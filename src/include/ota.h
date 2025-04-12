#ifndef __OTA_H__
#define __OTA_H__

#include <stdint.h>
#include <aes.h>

#define OTA_MAGIC 0x1337
#define PAGE_SIZE 64

// AES Blocks are 16 bytes. Then data should be what remains of such block size
struct chunk_header_s
{
    uint16_t magic;
    uint16_t addr;
    uint16_t total_size;
    uint16_t cksum;
};

struct chunk_s
{
    struct chunk_header_s header;
    uint8_t data[PAGE_SIZE];
    uint8_t _pad[AES_BLOCKLEN - (sizeof(struct chunk_header_s) + PAGE_SIZE) % AES_BLOCKLEN];
};

void updateInit();

void recvChunk();

#endif // __OTA_H__
