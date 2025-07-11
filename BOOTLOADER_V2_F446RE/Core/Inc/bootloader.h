/*
 * bootloader.h
 *
 *  Created on: May 21, 2025
 *      Author: kjeyabalan
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "main.h"

#define D_UART    &huart2
#define BL_RX_LEN  200

#define UART_TIMEOUT_MS        3000
#define TOTAL_TIMEOUT_MS       3000
#define POLL_INTERVAL_MS       10
#define BOOT_CMD               0x50

#define FLASH_SECTOR2_BASE_ADDRESS 0x08008000U      //Application base address

#define BL_GET_CID				0x51
#define BL_GO_TO_RESET			0x52
#define BL_FLASH_ERASE          0x53
#define BL_MEM_WRITE			0x54

/* ACK and NACK bytes*/
#define BL_ACK   0XA5
#define BL_NACK  0X7F

/*CRC*/
#define VERIFY_CRC_FAIL    1
#define VERIFY_CRC_SUCCESS 0

#define ADDR_VALID     0x00
#define ADDR_INVALID   0x01

#define INVALID_SECTOR 0x04

/*Memories of STM32F446RET6 MCU */
#define SRAM1_SIZE            112*1024     // STM32F446RE has 112KB of SRAM1
#define SRAM1_END             (SRAM1_BASE + SRAM1_SIZE)
#define SRAM2_SIZE            16*1024     // STM32F446RE has 16KB of SRAM2
#define SRAM2_END             (SRAM2_BASE + SRAM2_SIZE)
#define FLASH_SIZE            512*1024     // STM32F446RE has 512KB of SRAM2
#define BKPSRAM_SIZE          4*1024     // STM32F446RE has 4KB of SRAM2
#define BKPSRAM_END           (BKPSRAM_BASE + BKPSRAM_SIZE)


extern uint8_t bl_rx_buffer[BL_RX_LEN];
extern UART_HandleTypeDef *C_UART;

extern CRC_HandleTypeDef hcrc;

extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

void  bootloader_uart_read_data(void);
void bootloader_jump_to_user_app(void);

void bootloader_handle_getcid_cmd(uint8_t *pBuffer);
void bootloader_go_reset_cmd(uint8_t *pBuffer);
void bootloader_handle_flash_erase_cmd(uint8_t *pBuffer);
void bootloader_handle_mem_write_cmd(uint8_t *pBuffer);

void bootloader_send_ack(uint8_t command_code, uint8_t follow_len);
void bootloader_send_nack(void);

uint8_t bootloader_verify_crc (uint8_t *pData, uint32_t len,uint32_t crc_host);
uint8_t get_bootloader_version(void);
void bootloader_uart_write_data(uint8_t *pBuffer,uint32_t len);

uint16_t get_mcu_chip_id(void);
uint8_t verify_address(uint32_t go_address);
uint8_t execute_flash_erase(uint8_t sector_number , uint8_t number_of_sector);
uint8_t execute_mem_write(uint8_t *pBuffer, uint32_t mem_address, uint32_t len);


#endif /* INC_BOOTLOADER_H_ */
