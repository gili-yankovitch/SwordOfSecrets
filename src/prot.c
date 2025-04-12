#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define FLASH_OBKEYR ((volatile uint32_t *)0x40022008)
#define FLASH_KEYR ((volatile uint32_t *)0x40022004)
#define FLASH_MODEKEYR ((volatile uint32_t *)0x40022024)
#define FLASH_CTLR ((volatile uint32_t *)0x40022010)
#define FLASH_STATR ((volatile uint32_t *)0x4002200C)
#define FLASH_OBR ((volatile uint32_t *)0x4002201C)
#define FLASH_WPR ((volatile uint32_t *)0x40022020)
#define RDPRT_KEY 0x000000A5
#ifndef FLASH_KEY1
#define FLASH_KEY1 0x45670123
#endif

#ifndef FLASH_KEY2
#define FLASH_KEY2 0xCDEF89AB
#endif

#define WRITE_BLOCK_SIZE 64
#define FLASH_ADDR ((volatile uint32_t *)0x40022014)
#define FLASH_OBR ((volatile uint32_t *)0x4002201C)
#define FLASH_RDPR (*FLASH_OBR & (1 << 1))
#define RDPR ((volatile uint16_t *)0x1FFFF800)
#define RDPR_ON 0x7777
#define RDPR_OFF 0x5AA5

#define FLASH_BUFRST_BIT     (1 << 19)
#define FLASH_BUFLOAD_BIT    (1 << 18)
#define FLASH_FTER_BIT       (1 << 17)
#define FLASH_FTPG_BIT       (1 << 16)
#define FLASH_FLOCK_BIT      (1 << 15)
#define FLASH_OBWRE_BIT      (1 << 9)
#define FLASH_OBER_BIT       (1 << 5)
#define FLASH_OBG_BIT        (1 << 4)
#define FLASH_STRT_BIT       (1 << 6)
#define FLASH_LOCK_BIT       (1 << 7) // LOCK bit
#define FLASH_PER_BIT        (1 << 1) // Page Erase bit
#define FLASH_PG_BIT         (1 << 0) // Program mode bit
#define FLASH_BSY_BIT        (1 << 0) // Busy bit
#define FLASH_EOP_BIT        (1 << 5) // End of programming bit


static inline void __attribute__(( section(".topflash.text") )) flashFlockUnlock()
{
    *FLASH_MODEKEYR = FLASH_KEY1;
    *FLASH_MODEKEYR = FLASH_KEY2;
}

static inline void __attribute__(( section(".topflash.text") )) flashFlockLock()
{
    *FLASH_CTLR |= FLASH_FLOCK_BIT;
}

static inline void __attribute__(( section(".topflash.text") )) flashUnlock()
{
    *FLASH_KEYR = FLASH_KEY1;
    *FLASH_KEYR = FLASH_KEY2;
}

static inline void __attribute__(( section(".topflash.text") )) flashLock()
{
    *FLASH_CTLR |= FLASH_LOCK_BIT;
}

static inline void __attribute__(( section(".topflash.text") )) flashUnlockOBKEYR()
{
    *FLASH_OBKEYR = FLASH_KEY1;
    *FLASH_OBKEYR = FLASH_KEY2;
}

static inline void __attribute__(( section(".topflash.text") )) enableFlashProgramming()
{
    *FLASH_CTLR |= FLASH_PG_BIT; // Set PG bit
}

static inline void __attribute__(( section(".topflash.text") )) disableFlashProgramming()
{
    *FLASH_CTLR &= ~FLASH_PG_BIT; // Set PG bit
}

void __attribute__(( section(".topflash.text") )) flashBusy()
{
    while (((*FLASH_STATR) & FLASH_BSY_BIT)) ;
}

static inline void __attribute__(( section(".topflash.text") )) flashEOP()
{
    while ((((*FLASH_STATR) & FLASH_BSY_BIT)) && (!((*FLASH_STATR) & FLASH_EOP_BIT))) ;
    *FLASH_STATR |= FLASH_EOP_BIT;
}

void __attribute__((section(".topflash.text"))) flashPageErase(uint32_t address)
{
    // #1
    flashUnlock();
    flashBusy();

    // #2
    flashFlockUnlock();

    // #3
    flashBusy();

    // #4
    *FLASH_CTLR |= FLASH_FTER_BIT;

    // #5
    *FLASH_ADDR = address;

    // #6
    *FLASH_CTLR |= FLASH_STRT_BIT;

    // #7
    flashEOP();

    *FLASH_CTLR &= ~FLASH_FTER_BIT;

    flashFlockLock();
    flashBusy();

    flashLock();
    flashBusy();
}

void __attribute__((section(".topflash.text"))) _flashPageErase(uint32_t address)
{
    flashUnlock();  // Unlock flash for writing
    flashBusy();

    flashBusy();  // Ensure flash is not busy

    *FLASH_CTLR |= FLASH_PER_BIT;  // Enable page erase mode
    *FLASH_ADDR = address;  // Set the address to erase
    *FLASH_CTLR |= (1 << 6); // Start the erase operation

    flashBusy();  // Wait for erase to complete

    // Clear EOP flag
    if (*FLASH_STATR & FLASH_EOP_BIT) {
        *FLASH_STATR |= FLASH_EOP_BIT;
    }

    *FLASH_CTLR &= ~FLASH_PER_BIT;  // Disable page erase mode
    flashLock();  // Lock flash again
}

void __attribute__((section(".topflash.text"))) flashRead(uint32_t addr, void * pdata, size_t len)
{
    memcpy(pdata, (void *)addr, len);
}

void __attribute__((section(".topflash.text"))) _flashWrite(uint32_t addr, uint32_t * data)
{
    int i;

    flashUnlock();
    flashBusy();

    flashFlockUnlock();
    flashBusy();

    *FLASH_CTLR |= FLASH_FTPG_BIT;
    *FLASH_CTLR |= FLASH_BUFRST_BIT;

    flashEOP();

    // Write data to address
    for (i = 0; i < 16; i++)
    {
        *((volatile uint32_t *)addr + i) = data[i];
        *FLASH_CTLR |= FLASH_BUFLOAD_BIT;

        flashEOP();
    }

    *FLASH_ADDR = addr;
    flashEOP();

    *FLASH_CTLR |= FLASH_STRT_BIT;
    flashEOP();

    *FLASH_CTLR &= ~FLASH_FTPG_BIT;

    flashFlockLock();
    flashBusy();

    flashLock();
    flashBusy();
}

void __attribute__((noinline, used, section(".topflash.text"))) flashWrite(uint32_t addr, void * pdata, size_t len)
{
    int i;
    uint8_t tmp[WRITE_BLOCK_SIZE] = { 0 };
    uint32_t unalignedBytes = addr & (WRITE_BLOCK_SIZE - 1);
    uint8_t * data = pdata;

#ifdef DEBUG
    printf("Writing address %lx nbytes %d\r\n", addr, len);
#endif

    // Is the address unaligned?
    if (unalignedBytes)
    {
#ifdef DEBUG
        printf("Writing address %lx nbytes %ld UNALIGNED BYTES\r\n", addr - unalignedBytes, WRITE_BLOCK_SIZE - unalignedBytes);
#endif

        // Read the original data
        flashRead(addr - unalignedBytes, &tmp, WRITE_BLOCK_SIZE);

        // Erase the page
        flashPageErase(addr - unalignedBytes);

        // Write unaligned bytes
        memcpy((uint8_t *)tmp + unalignedBytes, data, WRITE_BLOCK_SIZE - unalignedBytes);

        // Rewrite the data
        _flashWrite(addr - unalignedBytes, (uint32_t *)tmp);

        // Decrease the number of unaligned bytes
        len -= WRITE_BLOCK_SIZE - unalignedBytes;

        // Update write addr
        addr += WRITE_BLOCK_SIZE - unalignedBytes;

        // Update data pointer
        data += WRITE_BLOCK_SIZE - unalignedBytes;
    }

#ifdef DEBUG
    printf("Writing address %lx nbytes %d ALIGNED BYTES\r\n", addr, (len / WRITE_BLOCK_SIZE) * WRITE_BLOCK_SIZE);
#endif

    // Write the remaining aligned bytes
    for (i = 0; i < len / WRITE_BLOCK_SIZE; i++)
    {
        flashPageErase(addr);

        // Write 4 bytes at a time
        _flashWrite(addr, (uint32_t *)data);

#ifdef DEBUG
        printf("ALIGNED BYTES: 0x%lx\r\n", *((uint32_t *)data));
        printf("ALIGNED BYTES: 0x%lx\r\n", *((uint32_t *)addr));
#endif
        // Update addresses
        addr += WRITE_BLOCK_SIZE;
        data += WRITE_BLOCK_SIZE;
    }

    // Decrease the number of bytes from len
    len -= i * WRITE_BLOCK_SIZE;

    // Is there something left to write?
    if (len)
    {
#ifdef DEBUG
        printf("Writing address %lx nbytes %d REMAINING BYTES\r\n", addr, len);
#endif
        // Read remaining data
        flashRead(addr, &tmp, WRITE_BLOCK_SIZE);

        flashPageErase(addr);

        // Write the rest of the data
        memcpy(tmp, data, len);

        // Write back to flash
        _flashWrite(addr, (uint32_t *)tmp);
    }
}

static void __attribute__((section(".topflash.text"))) userSelectProg(volatile uint16_t * addr, uint16_t val)
{
    flashUnlock();
    flashUnlockOBKEYR();

    flashBusy();

    // Set OBG
    *FLASH_CTLR |= FLASH_OBG_BIT;
    *FLASH_CTLR |= FLASH_STRT_BIT;

    flashBusy();
    flashEOP();

    *addr = val;

    *FLASH_CTLR &= ~FLASH_OBG_BIT;
}

int __attribute__((section(".topflash.text"))) flashReadProtect()
{
    if (!FLASH_RDPR)
    {
        userSelectProg(RDPR, RDPR_ON);
    }

    return 0;
}

int __attribute__((section(".topflash.text"))) flashReadUnprotect()
{
    if (FLASH_RDPR)
    {
        // 1
        flashUnlock();

        // 2
        flashBusy();

        // 3
        if (!(*FLASH_CTLR & FLASH_OBWRE_BIT))
        {
            flashUnlockOBKEYR();
        }

        // 4
        *FLASH_CTLR |= FLASH_OBER_BIT;
        *FLASH_CTLR |= FLASH_STRT_BIT;

        // 5
        flashBusy();
        flashEOP();

        *RDPR = 1;

        *FLASH_OBR &= ~(0xff << 10);

        *FLASH_OBR &= ~(0xff << 18);

        // 7
        *FLASH_CTLR &= ~FLASH_OBER_BIT;

        userSelectProg(RDPR, RDPR_OFF);
    }

    return 0;
}
