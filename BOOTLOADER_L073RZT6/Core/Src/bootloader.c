/*
 * bootloader.c
 *
 *  Created on: July 12, 2025
 *      Author: kjeyabalan
 */

#include "bootloader.h"

uint8_t bl_rx_buffer[BL_RX_LEN];

void debug_puts(char *s)
{
	while(*s)
	{
		HAL_UART_Transmit(&huart2, (uint8_t *)s++, 1, HAL_MAX_DELAY);
	}
	HAL_UART_Transmit(&huart2, (uint8_t *)"\n", 1, HAL_MAX_DELAY);
}

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
            	debug_puts("BL_MSG: Invalid command");
                break;
		}
	}
}

void bootloader_jump_to_user_app(void)
{
	void (*app_reset_handler)(void);

	debug_puts("BL_MSG: Jumping to user app");

	uint32_t msp_value = *(volatile uint32_t *)FLASH_SECTOR2_BASE_ADDRESS;
	uint32_t reset_handler_address = *(volatile uint32_t *)(FLASH_SECTOR2_BASE_ADDRESS + 4);

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
	debug_puts("BL_MSG: Reset command received");
    bootloader_send_ack(pBuffer[0], 0);
    uint8_t resp = ADDR_VALID;
    bootloader_uart_write_data(&resp, 1);
    HAL_Delay(1000);
    NVIC_SystemReset();
}

void bootloader_handle_getcid_cmd(uint8_t *pBuffer)
{
    uint16_t bl_cid_num = get_mcu_chip_id();
    uint8_t tx_buf[4] = { BL_ACK, 2, (uint8_t)(bl_cid_num & 0xFF), (uint8_t)((bl_cid_num >> 8) & 0xFF) };
    HAL_UART_Transmit(C_UART, tx_buf, sizeof(tx_buf), HAL_MAX_DELAY);
    debug_puts("BL_MSG: Sent chip ID");
}

void bootloader_handle_flash_erase_cmd(uint8_t *pBuffer)
{
    debug_puts("BL_MSG: Flash erase command");
    uint8_t erase_status;
    uint8_t initial_page = pBuffer[2];
    uint16_t num_pages = pBuffer[3] | (pBuffer[4] << 8);

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);
    erase_status = execute_flash_erase(initial_page, num_pages);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0);

    bootloader_send_ack(pBuffer[0], 1);
    bootloader_uart_write_data(&erase_status, 1);
}

void bootloader_handle_mem_write_cmd(uint8_t *pBuffer)
{
    debug_puts("BL_MSG: Memory write command");
    uint8_t len = pBuffer[0];
    if (len < 7) return;

    uint8_t payload_len = pBuffer[6];
    uint32_t mem_address;
    memcpy(&mem_address, &pBuffer[2], 4);

    bootloader_send_ack(pBuffer[0], 1);

    uint8_t write_status = (verify_address(mem_address) == ADDR_VALID)
        ? execute_mem_write(&pBuffer[7], mem_address, payload_len)
        : ADDR_INVALID;

    bootloader_uart_write_data(&write_status, 1);
}

void bootloader_send_ack(uint8_t command_code, uint8_t follow_len)
{
    uint8_t ack_buf[2] = { BL_ACK, follow_len };
    HAL_UART_Transmit(C_UART, ack_buf, 2, HAL_MAX_DELAY);
}

void bootloader_send_nack(void)
{
    uint8_t nack = BL_NACK;
    HAL_UART_Transmit(C_UART, &nack, 1, HAL_MAX_DELAY);
}

void bootloader_uart_write_data(uint8_t *pBuffer, uint32_t len)
{
    HAL_UART_Transmit(C_UART, pBuffer, len, HAL_MAX_DELAY);
}

uint16_t get_mcu_chip_id(void)
{
    return (uint16_t)(DBGMCU->IDCODE) & 0x0FFF;
}


uint8_t verify_address(uint32_t go_address)
{
    return ((go_address >= SRAM1_BASE && go_address < SRAM1_END) ||
            (go_address >= FLASH_BASE && go_address < FLASH_END))
            ? ADDR_VALID : ADDR_INVALID;
}

uint8_t execute_flash_erase(uint8_t page_number, uint16_t number_of_pages)
{
    FLASH_EraseInitTypeDef flashErase_handle;
    uint32_t pageError;

    if (number_of_pages > 512) return INVALID_SECTOR;

    flashErase_handle.TypeErase = FLASH_TYPEERASE_PAGES;
    flashErase_handle.PageAddress = FLASH_BASE + ((uint32_t)page_number * 128);
    flashErase_handle.NbPages = number_of_pages;

    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&flashErase_handle, &pageError);
    HAL_FLASH_Lock();

    return HAL_OK;
}

uint8_t execute_mem_write(uint8_t *pBuffer, uint32_t mem_address, uint32_t len)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t padded_len = (len + 3) & ~0x03;

    HAL_FLASH_Unlock();
    for (uint32_t i = 0; i < padded_len; i += 4)
    {
        uint32_t word = 0xFFFFFFFF;
        word  = (i + 0 < len ? pBuffer[i + 0] : 0xFF);
        word |= (i + 1 < len ? pBuffer[i + 1] : 0xFF) << 8;
        word |= (i + 2 < len ? pBuffer[i + 2] : 0xFF) << 16;
        word |= (i + 3 < len ? pBuffer[i + 3] : 0xFF) << 24;

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, mem_address + i, word);
        if (status != HAL_OK) break;
    }
    HAL_FLASH_Lock();
    return status;
}





