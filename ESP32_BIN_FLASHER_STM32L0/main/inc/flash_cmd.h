#ifndef FLASH_CMD_H
#define FLASH_CMD_H

#include "ota_update.h"
#include "http.h"
#include "crc32.h"
#include "uart_config.h"

esp_err_t send_sync_command(void);
esp_err_t send_get_cid_command(void);
esp_err_t send_flash_erase_command(uint8_t sector, uint8_t num_sectors);
esp_err_t send_mem_write_command(uint32_t base_address, uint8_t *data, uint8_t length);
esp_err_t send_go_reset();
esp_err_t flash_downloaded_firmware(void);


#endif