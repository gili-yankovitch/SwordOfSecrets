#include <ch32v003fun.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <uart.h>
#include <ota.h>
#include <prot.h>
#include <otakey.h>

#define RESET_MAX_JIFFIES 10000000
#define FLASH_ADDR 0x08000000
#define FLASH_SIZE 0x00004000
#define OTA_START_ADDR 0x1000
#define OTA_END_ADDR   0x4000

static uint8_t __attribute__(( section(".bootloader.data") )) iv[AES_BLOCKLEN] = { 0 };

static struct AES_ctx __attribute__(( section(".bootloader.data") )) ctx;
static const uint8_t __attribute__(( used, section(".topflash.rodata") )) status[] = "VMCSE";

void __attribute__(( section(".topflash.text") )) updateInit()
{
    memset(iv, 0, AES_BLOCKLEN);

    AES_init_ctx_iv(&ctx, (const uint8_t *)&k, (const uint8_t *)&iv);
}

uint16_t __attribute__(( section(".topflash.text") )) cksum16(uint8_t * buf, size_t s)
{
    uint32_t sum = 0;
    int i;

    for (i = 0; i < s; i += 2)
    {
        sum += *((uint16_t *)(buf + i));
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    return sum;
}

void __attribute__((noinline, used, section(".topflash.text") )) recvChunk()
{
    struct chunk_s chunk;
    uint16_t cksum;

    read((uint8_t *)&chunk, sizeof(struct chunk_s));

    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)&chunk, sizeof(struct chunk_s));

    // NULL checksum
    cksum = chunk.header.cksum;
    chunk.header.cksum = 0;

    // Verify:
    // 1. Magic for sanity
    // 2. Checksum for correctness
    // 3. Chunk address does not overwrite OTA code or ISRVec
    if (chunk.header.magic != OTA_MAGIC)
    {
        _write(0, (const char *)&status[1], 1);

        return;
    }

    if (cksum != cksum16((uint8_t *)&chunk, sizeof(struct chunk_s)))
    {
        _write(0, (const char *)&status[2], 1);

        return;
    }

    if (chunk.header.addr < OTA_START_ADDR)
    {
        _write(0, (const char *)&status[3], 1);

        return;
    }

    if (chunk.header.addr >= OTA_END_ADDR)
    {
        _write(0, (const char *)&status[4], 1);

        return;
    }

    // Write!
    flashPageErase(FLASH_ADDR + chunk.header.addr);
    _flashWrite(FLASH_ADDR + chunk.header.addr, (uint32_t *)chunk.data);

    // Tell other side that this chunk was written successfully.
    _write(0, (const char *)&status[0], 1);
}

bool __attribute__(( noinline, used, section(".topflash.text") )) update_wait()
{
    uint32_t initial_jiffies = SysTick->CNT;

    while (SysTick->CNT - initial_jiffies < RESET_MAX_JIFFIES)
    {
        if (uartAvailable())
        {
            return true;
        }
    }

    return false;
}

void __attribute__(( noinline, used, section(".topflash.text") )) ota()
{
    // No UART on boot
    if (!update_wait())
    {
        return;
    }

    // flashReadProtect();

    updateInit();

    for (;;)
    {
        // Wait for data
        if (uartAvailable())
        {
            recvChunk();
        }

        // Finish update if uart hangs
        if (!update_wait())
        {
            return;
        }
    }
}

void __attribute__((used)) _startup();

void __attribute__(( section(".topflash.text") )) boot()
{
    // Initialize everything
    SystemInit();

    uartInit();

    ota();

    // Call main()
    _startup();
}
