#ifndef HTTP_H
#define HTTP_H

#include "ota_update.h"
#include "mqtt.h"

#define HTTP_BUFFER_SIZE                2048
#define FIRMWARE_CHUNK_SIZE             128   // Size for each write command
#define DOWNLOAD_BUFFER_SIZE            32768 // 32KB buffer for larger files
#define MAX_FIRMWARE_SIZE               (512 * 1024)  // 512KB max

extern uint8_t *download_buffer;
extern size_t total_firmware_size;
extern size_t bytes_downloaded;
extern bool download_complete;
extern size_t allocated_buffer_size;


esp_err_t http_event_handler(esp_http_client_event_t *evt);
esp_err_t download_firmware(const char *url);

#endif