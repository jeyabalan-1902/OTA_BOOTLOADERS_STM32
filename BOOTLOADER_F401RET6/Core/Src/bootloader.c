/*
 * bootloader.c
 *
 *  Created on: May 21, 2025
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

	    SCB->VTOR = FLASH_SECTOR2_BASE_ADDRESS;
		__DSB();
		__ISB();
	    __set_MSP(msp_value);

	    app_reset_handler = (void *)reset_handler_address;
	    app_reset_handler();

}

void bootloader_go_reset_cmd(uint8_t *pBuffer)
{
	printf("BL_MSG:bootloader_go_reset_cmd\n");
	uint32_t command_packet_len = bl_rx_buffer[0]+1 ;
	uint32_t host_crc = *((uint32_t * ) (bl_rx_buffer+command_packet_len - 4) ) ;

	if (! bootloader_verify_crc(&bl_rx_buffer[0],command_packet_len-4,host_crc))
	{
		 printf("BL_MSG:checksum success !!\n");
		 bootloader_send_ack(pBuffer[0],0);
		 bootloader_uart_write_data(ADDR_VALID, 1);
		 printf("BL_MSG:Going to reset... !!\n");
		 HAL_Delay(1000);
		 NVIC_SystemReset();
	}
	else
	{
		 printf("BL_MSG:checksum fail !!\n");
		 bootloader_send_nack();
	}
}

 void bootloader_handle_getcid_cmd(uint8_t *pBuffer)
 {
 	uint16_t bl_cid_num = 0;
 	printf("BL_MSG:bootloader_handle_getcid_cmd\n");

     //Total length of the command packet
 	uint32_t command_packet_len = bl_rx_buffer[0]+1 ;

 	//extract the CRC32 sent by the Host
 	uint32_t host_crc = *((uint32_t * ) (bl_rx_buffer+command_packet_len - 4) ) ;

 	if (! bootloader_verify_crc(&bl_rx_buffer[0],command_packet_len-4,host_crc))
 	{
         printf("BL_MSG:checksum success !!\n");
         bootloader_send_ack(pBuffer[0],2);
         bl_cid_num = get_mcu_chip_id();
         printf("BL_MSG:MCU id : %d %#x !!\n",bl_cid_num, bl_cid_num);
         bootloader_uart_write_data((uint8_t *)&bl_cid_num,2);

 	}
 	else
 	{
         printf("BL_MSG:checksum fail !!\n");
         bootloader_send_nack();
 	}
 }

 void bootloader_handle_flash_erase_cmd(uint8_t *pBuffer)
 {
     uint8_t erase_status = 0x00;
     printf("BL_MSG:bootloader_handle_flash_erase_cmd\n");

     //Total length of the command packet
 	uint32_t command_packet_len = bl_rx_buffer[0]+1 ;

 	//extract the CRC32 sent by the Host
 	uint32_t host_crc = *((uint32_t * ) (bl_rx_buffer+command_packet_len - 4) ) ;

 	if (! bootloader_verify_crc(&bl_rx_buffer[0],command_packet_len-4,host_crc))
 	{
         printf("BL_MSG:checksum success !!\n");
         bootloader_send_ack(pBuffer[0],1);
         printf("BL_MSG:initial_sector : %d  no_ofsectors: %d\n",pBuffer[2],pBuffer[3]);

         HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,1);
         erase_status = execute_flash_erase(pBuffer[2] , pBuffer[3]);
         HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,0);

         printf("BL_MSG: flash erase status: %#x\n",erase_status);

         bootloader_uart_write_data(&erase_status,1);

 	}else
 	{
         printf("BL_MSG:checksum fail !!\n");
         bootloader_send_nack();
 	}
 }


 void bootloader_handle_mem_write_cmd(uint8_t *pBuffer)
 {
 	uint8_t addr_valid = ADDR_VALID;
 	uint8_t write_status = 0x00;
 	uint8_t chksum =0, len=0;
 	len = pBuffer[0];
 	uint8_t payload_len = pBuffer[6];

 	uint32_t mem_address = *((uint32_t *) ( &pBuffer[2]) );

 	chksum = pBuffer[len];

     printf("BL_MSG:bootloader_handle_mem_write_cmd\n");

     //Total length of the command packet
 	uint32_t command_packet_len = bl_rx_buffer[0]+1 ;

 	//extract the CRC32 sent by the Host
 	uint32_t host_crc = *((uint32_t * ) (bl_rx_buffer+command_packet_len - 4) ) ;


 	if (! bootloader_verify_crc(&bl_rx_buffer[0],command_packet_len-4,host_crc))
 	{
         printf("BL_MSG:checksum success !!\n");

         bootloader_send_ack(pBuffer[0],1);

         printf("BL_MSG: write addr:%#lx\n", mem_address);

 		if( verify_address(mem_address) == ADDR_VALID )
 		{

             printf("BL_MSG: valid mem write address\n");

             //glow the led to indicate bootloader is currently writing to memory
             HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

             //execute mem write
             write_status = execute_mem_write(&pBuffer[7],mem_address, payload_len);

             //turn off the led to indicate memory write is over
             HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

             //inform host about the status
             bootloader_uart_write_data(&write_status,1);

 		}
 		else
 		{
             printf("BL_MSG: invalid mem write address\n");
             write_status = ADDR_INVALID;
             //inform host that address is invalid
             bootloader_uart_write_data(&write_status,1);
 		}
 	}
 	else
 	{
         printf("BL_MSG:checksum fail !!\n");
         bootloader_send_nack();
 	}
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

 //This verifies the CRC of the given buffer in pData .
 uint8_t bootloader_verify_crc (uint8_t *pData, uint32_t len, uint32_t crc_host)
 {
     uint32_t uwCRCValue=0xff;

     for (uint32_t i=0 ; i < len ; i++)
 	{
         uint32_t i_data = pData[i];
         uwCRCValue = HAL_CRC_Accumulate(&hcrc, &i_data, 1);
 	}

 	 /* Reset CRC Calculation Unit */
   __HAL_CRC_DR_RESET(&hcrc);

 	if( uwCRCValue == crc_host)
 	{
 		return VERIFY_CRC_SUCCESS;
 	}

 	return VERIFY_CRC_FAIL;
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
 	if ( go_address >= SRAM1_BASE && go_address <= SRAM1_END)
 	{
 		return ADDR_VALID;
 	}
// 	else if ( go_address >= SRAM2_BASE && go_address <= SRAM2_END)
// 	{
// 		return ADDR_VALID;
// 	}
 	else if ( go_address >= FLASH_BASE && go_address <= FLASH_END)
 	{
 		return ADDR_VALID;
 	}
// 	else if ( go_address >= BKPSRAM_BASE && go_address <= BKPSRAM_END)
// 	{
// 		return ADDR_VALID;
// 	}
 	else
 		return ADDR_INVALID;
 }


uint8_t execute_flash_erase(uint8_t sector_number , uint8_t number_of_sector)
 {
 	FLASH_EraseInitTypeDef flashErase_handle;
 	uint32_t sectorError;
 	HAL_StatusTypeDef status;


 	if( number_of_sector > 8 )
 		return INVALID_SECTOR;

 	if( (sector_number == 0xff ) || (sector_number <= 7) )
 	{
 		if(sector_number == (uint8_t) 0xff)
 		{
 			flashErase_handle.TypeErase = FLASH_TYPEERASE_MASSERASE;
 		}else
 		{
 		    /*Here we are just calculating how many sectors needs to erased */
 			uint8_t remanining_sector = 8 - sector_number;
             if( number_of_sector > remanining_sector)
             {
             	number_of_sector = remanining_sector;
             }
 			flashErase_handle.TypeErase = FLASH_TYPEERASE_SECTORS;
 			flashErase_handle.Sector = sector_number; // this is the initial sector
 			flashErase_handle.NbSectors = number_of_sector;
 		}
 		flashErase_handle.Banks = FLASH_BANK_1;

 		/*Get access to touch the flash registers */
 		HAL_FLASH_Unlock();
 		flashErase_handle.VoltageRange = FLASH_VOLTAGE_RANGE_3;  // our mcu will work on this voltage range
 		status = (uint8_t) HAL_FLASHEx_Erase(&flashErase_handle, &sectorError);
 		HAL_FLASH_Lock();

 		return status;
 	}


 	return INVALID_SECTOR;
 }


 uint8_t execute_mem_write(uint8_t *pBuffer, uint32_t mem_address, uint32_t len)
 {
     uint8_t status=HAL_OK;

     //We have to unlock flash module to get control of registers
     HAL_FLASH_Unlock();

     for(uint32_t i = 0 ; i <len ; i++)
     {
         //Here we program the flash byte by byte
         status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,mem_address+i,pBuffer[i] );
     }

     HAL_FLASH_Lock();

     return status;
 }
