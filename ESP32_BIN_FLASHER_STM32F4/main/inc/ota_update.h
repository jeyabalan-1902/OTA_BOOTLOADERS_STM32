#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include <sys/socket.h>


#define FLASH_BASE_ADDRESS              0x08008000

#define FLASH_ERASE_BASE_SECTOR         2
#define FLASH_ERASE_SECTORS             3

#define Flash_HAL_OK                    0x00
#define Flash_HAL_ERROR                 0x01
#define Flash_HAL_BUSY                  0x02
#define Flash_HAL_TIMEOUT               0x03
#define Flash_HAL_INV_ADDR              0x04

// BL Commands
#define COMMAND_BL_GET_CID              0x51
#define COMMAND_BL_GO_TO_RESET          0x52
#define COMMAND_BL_FLASH_ERASE          0x53
#define COMMAND_BL_MEM_WRITE            0x54

// Command Lengths
#define COMMAND_BL_GET_CID_LEN          6
#define COMMAND_BL_GO_TO_RESET_LEN      6
#define COMMAND_BL_FLASH_ERASE_LEN      8
#define COMMAND_BL_MEM_WRITE_BASE_LEN   7 

#define BL_ACK                          0xA5
#define BL_NACK                         0x7F


#endif