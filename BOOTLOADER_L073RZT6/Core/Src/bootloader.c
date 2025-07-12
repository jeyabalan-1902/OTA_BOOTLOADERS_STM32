/*
 * bootloader.c
 *
 *  Created on: July 12, 2025
 *      Author: kjeyabalan
 */

#include "bootloader.h"

uint8_t bl_rx_buffer[BL_RX_LEN];
UART_HandleTypeDef *C_UART = NULL;

void  bootloader_uart_read_data(void)
{
    uint8_t rcv_len=0;

	while(1)
	{
		memset(bl_rx_buffer,0,200);
        HAL_UART_Receive(C_UART,bl_rx_buffer,1,HAL_MAX_DELAY);
		rcv_len= bl_rx_buffer[0];
		HAL_UART_Receive(C_UART,&bl_rx_buffer[1],rcv_len,HAL_MAX_DELAY);
		printf("BL_MSG: UART RX => LEN: %d CMD: 0x%02X\n", bl_rx_buffer[0], bl_rx_buffer[1]);
		switch(bl_rx_buffer[1])
		{
            case BL_GET_CID:
                bootloader_handle_getcid_cmd(bl_rx_buffer);
                break;
            case BL_GO_TO_RESET:
            	bootloader_go_reset_cmd(bl_rx_buffer);
                break;
            case BL_FLASH_ERASE:
                bootloader_handle_flash_erase_cmd(bl_rx_buffer);
                break;
            case BL_MEM_WRITE:
                bootloader_handle_mem_write_cmd(bl_rx_buffer);
                break;
            default:
                printf("BL_MSG:Invalid command code received from host \n");
                break;
		}
	}
}

void bootloader_jump_to_user_app(void)
{
	 void (*app_reset_handler)(void);

	    printf("BL_MSG: bootloader_jump_to_user_app\n");

	    uint32_t msp_value = *(volatile uint32_t *)FLASH_SECTOR2_BASE_ADDRESS;
	    uint32_t reset_handler_address = *(volatile uint32_t *)(FLASH_SECTOR2_BASE_ADDRESS + 4);

	    char msg[23];
		snprintf(msg, sizeof(msg), "MSP: 0x%08lX", msp_value);
		printf("%s",msg);

		snprintf(msg, sizeof(msg), "Reset: 0x%08lX", reset_handler_address);
		printf("%s",msg);

		HAL_Delay(1000);

	    HAL_RCC_DeInit();
		HAL_DeInit();

		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL = 0;

		for (int i = 0; i < 8; i++) {
			NVIC->ICER[i] = 0xFFFFFFFF;
			NVIC->ICPR[i] = 0xFFFFFFFF;
		}
		__DSB();
		__ISB();
	    __set_MSP(msp_value);

	    app_reset_handler = (void *)reset_handler_address;
	    app_reset_handler();

}

void bootloader_go_reset_cmd(uint8_t *pBuffer)
{
    printf("BL_MSG:bootloader_go_reset_cmd\n");

    // Send ACK (no follow-up data expected)
    bootloader_send_ack(pBuffer[0], 0);

    // Optionally send a confirmation byte (e.g., ADDR_VALID)
    uint8_t resp = ADDR_VALID;
    bootloader_uart_write_data(&resp, 1);

    printf("BL_MSG:Going to reset... !!\n");
    HAL_Delay(1000);  // Optional delay before reset

    NVIC_SystemReset();  // Perform system reset
}

void bootloader_handle_getcid_cmd(uint8_t *pBuffer)
{
	uint16_t bl_cid_num = get_mcu_chip_id();
    uint8_t tx_buf[4];

    printf("BL_MSG:bootloader_handle_getcid_cmd\n");



    tx_buf[0] = BL_ACK;
    tx_buf[1] = 2;
    tx_buf[2] = (uint8_t)(bl_cid_num & 0xFF);
    tx_buf[3] = (uint8_t)((bl_cid_num >> 8) & 0xFF);

    HAL_UART_Transmit(C_UART, tx_buf, sizeof(tx_buf), HAL_MAX_DELAY);

    printf("BL_MSG:Sent ACK + CID: 0x%04X\n", bl_cid_num);
}



void bootloader_handle_flash_erase_cmd(uint8_t *pBuffer)
{
    uint8_t erase_status = 0x00;
    printf("BL_MSG: bootloader_handle_flash_erase_cmd\n");

    uint8_t initial_page = pBuffer[2];
    uint16_t num_pages = pBuffer[3] | (pBuffer[4] << 8);

    printf("BL_MSG: initial_page: %d, number_of_pages: %d\n", initial_page, num_pages);

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);

    erase_status = execute_flash_erase(initial_page, num_pages);

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0);

    bootloader_send_ack(pBuffer[0], 1);

    printf("BL_MSG: flash erase status: %d\n", erase_status);
    bootloader_uart_write_data(&erase_status, 1);
}



void bootloader_handle_mem_write_cmd(uint8_t *pBuffer)
{
    printf("BL_MSG: >>> ENTERED bootloader_handle_mem_write_cmd <<<\n");

    uint8_t len = pBuffer[0];
    if (len < 7) {
        printf("BL_MSG: ERROR - Packet too short\n");
        return;
    }

    printf("BL_MSG: Packet length = %d\n", len);
    printf("BL_MSG: DUMP: ");
    for (int i = 0; i < (len < 16 ? len : 16); i++) {
        printf("%02X ", pBuffer[i]);
    }
    printf("\n");

    uint8_t payload_len = pBuffer[6];
    uint32_t mem_address;
    memcpy(&mem_address, &pBuffer[2], 4);  // Safe on Cortex-M0+

    printf("BL_MSG: write addr = 0x%08lX, data length = %d\n", mem_address, payload_len);

    bootloader_send_ack(pBuffer[0], 1);
    printf("BL_MSG: ACK sent\n");

    uint8_t write_status;

    if (verify_address(mem_address) == ADDR_VALID)
    {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

        write_status = execute_mem_write(&pBuffer[7], mem_address, payload_len);

        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        printf("BL_MSG: memory write status = %d\n", write_status);
    }
    else
    {
        printf("BL_MSG: invalid memory address\n");
        write_status = ADDR_INVALID;
    }

    printf("BL_MSG: writing status byte: 0x%02X\n", write_status);
    bootloader_uart_write_data(&write_status, 1);
    printf("BL_MSG: write_status sent: %d\n", write_status);
}


 void bootloader_send_ack(uint8_t command_code, uint8_t follow_len)
 {
 	 //here we send 2 byte.. first byte is ack and the second byte is len value
 	uint8_t ack_buf[2];
 	ack_buf[0] = BL_ACK;
 	ack_buf[1] = follow_len;
 	HAL_UART_Transmit(C_UART,ack_buf,2,HAL_MAX_DELAY);

 }

 /*This function sends NACK */
 void bootloader_send_nack(void)
 {
 	uint8_t nack = BL_NACK;
 	HAL_UART_Transmit(C_UART,&nack,1,HAL_MAX_DELAY);
 }


 uint8_t bootloader_verify_crc (uint8_t *pData, uint32_t len, uint32_t crc_host)
 {
     uint32_t uwCRCValue = 0xFFFFFFFF;

     for (uint32_t i = 0; i < len; i++) {
         uint32_t i_data = pData[i];
         uwCRCValue = HAL_CRC_Accumulate(&hcrc, &i_data, 1);
     }

     __HAL_CRC_DR_RESET(&hcrc);

     return (uwCRCValue == crc_host) ? VERIFY_CRC_SUCCESS : VERIFY_CRC_FAIL;
 }


 void bootloader_uart_write_data(uint8_t *pBuffer,uint32_t len)
 {
     /*you can replace the below ST's USART driver API call with your MCUs driver API call */
 	HAL_UART_Transmit(C_UART,pBuffer,len,HAL_MAX_DELAY);

 }

 uint16_t get_mcu_chip_id(void)
 {
 	uint16_t cid;
 	cid = (uint16_t)(DBGMCU->IDCODE) & 0x0FFF;
 	return  cid;
 }


 uint8_t verify_address(uint32_t go_address)
 {
     if ((go_address >= SRAM1_BASE) && (go_address < SRAM1_END))
         return ADDR_VALID;
     else if ((go_address >= FLASH_BASE) && (go_address < FLASH_END))
         return ADDR_VALID;
     return ADDR_INVALID;
 }


 uint8_t execute_flash_erase(uint8_t page_number, uint16_t number_of_pages)
 {
     FLASH_EraseInitTypeDef flashErase_handle;
     HAL_StatusTypeDef status;
     uint32_t pageError;

     if (number_of_pages > 512) // L073 has 1536 pages, stay safe
         return INVALID_SECTOR;

     flashErase_handle.TypeErase   = FLASH_TYPEERASE_PAGES;
     flashErase_handle.PageAddress = FLASH_BASE + ((uint32_t)page_number * 128); // 128 bytes/page
     flashErase_handle.NbPages     = number_of_pages;

     HAL_FLASH_Unlock();
     status = HAL_FLASHEx_Erase(&flashErase_handle, &pageError);
     HAL_FLASH_Lock();

     return status;
 }



 uint8_t execute_mem_write(uint8_t *pBuffer, uint32_t mem_address, uint32_t len)
 {
     HAL_StatusTypeDef status = HAL_OK;

     printf("BL_MSG: inside execute_mem_write len = %lu\n", len);

     // Align length to 4 bytes
     uint32_t padded_len = (len + 3) & ~0x03;

     HAL_FLASH_Unlock();

     for (uint32_t i = 0; i < padded_len; i += 4)
     {
         uint32_t word = 0xFFFFFFFF;  // default fill

         // Copy safely (pad if buffer shorter)
         word  = (i + 0 < len ? pBuffer[i + 0] : 0xFF);
         word |= (i + 1 < len ? pBuffer[i + 1] : 0xFF) << 8;
         word |= (i + 2 < len ? pBuffer[i + 2] : 0xFF) << 16;
         word |= (i + 3 < len ? pBuffer[i + 3] : 0xFF) << 24;

         // Write 32-bit word
         status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, mem_address + i, word);

         if (status != HAL_OK)
         {
             printf("BL_MSG: FLASH PROGRAM FAIL at 0x%08lX (status: %d)\n", mem_address + i, status);
             break;
         }
     }

     HAL_FLASH_Lock();

     printf("BL_MSG: FLASH write complete with status = %d\n", status);
     return status;
 }





