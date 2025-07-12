#ifndef CRC32_H
#define CRC32_H

#include "ota_update.h"

uint32_t get_crc(uint8_t *buff, uint32_t len);
uint8_t word_to_byte(uint32_t addr, int index);

#endif