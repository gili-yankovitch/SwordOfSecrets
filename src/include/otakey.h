#ifndef __OTA_KEY_H__
#define __OTA_KEY_H__

static const uint8_t __attribute__(( used, section(".topflash.rodata") )) k[AES_BLOCKLEN] = {
                0x0, 0x1, 0x2, 0x3,
                0x4, 0x5, 0x6, 0x7,
                0x8, 0x9, 0xa, 0xb,
                0xc, 0xd, 0xe, 0xf
            };

#endif // __OTA_KEY_H__
