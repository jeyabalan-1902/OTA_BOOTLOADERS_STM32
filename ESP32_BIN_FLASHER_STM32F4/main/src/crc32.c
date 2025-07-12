#include "crc32.h"

uint8_t word_to_byte(uint32_t addr, int index) {
    return (uint8_t)((addr >> (8 * (index - 1))) & 0x000000FF);
}

uint32_t get_crc(uint8_t *buff, uint32_t len) {
    uint32_t Crc = 0xFFFFFFFF;
    
    for (uint32_t n = 0; n < len; n++) {
        uint32_t data = buff[n];
        Crc = Crc ^ data;
        for (int i = 0; i < 32; i++) {
            if (Crc & 0x80000000) {
                Crc = (Crc << 1) ^ 0x04C11DB7; // STM32 polynomial
            } else {
                Crc = (Crc << 1);
            }
        }
    }
    return Crc;
}