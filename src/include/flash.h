#ifndef __FLASH_H__
#define __FLASH_H__
#include <stdbool.h>

#define EXT_CMDS_ADDR 0x70000

#ifndef READ_CMD
#define READ_CMD 0x03
#endif

#ifndef WRITE_CMD
#define WRITE_CMD 0x02
#endif

#ifndef ERASE_CMD
#define ERASE_CMD 0x52
#endif

struct _ext_cmds_s
{
    uint8_t read;
    uint8_t write;
    uint8_t erase;
};

uint32_t flash_capacity(const uint8_t *id);

 bool flash_init(int cs_pin);

void flash_read(uint32_t addr, void * buf, size_t len);

void flash_write(uint32_t addr, void * buf, size_t len);

void flash_erase_block(uint32_t addr);

void flash_read_ext(uint32_t addr, void * buf, size_t len);

void flash_write_ext(uint32_t addr, void * buf, size_t len);

void flash_erase_block_ext(uint32_t addr);

void flash_load_ext_cmds();

#endif // __FLASH_H__
