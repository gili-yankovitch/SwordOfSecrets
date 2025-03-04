#ifndef __FLASH_H__
#define __FLASH_H__
#include <stdbool.h>

uint32_t flash_capacity(const uint8_t *id);

 bool flash_init(int cs_pin);

void flash_read(uint32_t addr, void * buf, size_t len);

void flash_write(uint32_t addr, void * buf, size_t len);

void flash_erase_block(uint32_t addr);

#endif // __FLASH_H__
