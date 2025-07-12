#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include "ota_update.h"
#include "crc32.h"

#define UART_PORT_NUM                   UART_NUM_1
#define UART_BAUD_RATE                  115200
#define UART_TX_PIN                     GPIO_NUM_17
#define UART_RX_PIN                     GPIO_NUM_16
#define UART_BUF_SIZE                   1024


void uart_init(void);
void uart_write_byte(uint8_t data);
int uart_read_bytes_timeout(uint8_t *data, size_t length, int timeout_ms);
void uart_flush_rx_buffer(void);
esp_err_t read_bootloader_reply(uint8_t command_code, uint8_t *response_data, size_t *response_len);
esp_err_t send_bootloader_packet(uint8_t *packet, size_t total_len);

#endif