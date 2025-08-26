#include <stdio.h>
#include <stdint.h>
#include <ch32v003fun.h>
#include <build-mode.h>
#include <aes.h>
#include <armory.h>
#include <keys.h>
#include <uart.h>
#include <flash.h>
#include <cli.h>

#define CH32V003_SPI_SPEED_HZ (100000000/3)
#define CH32V003_SPI_DIRECTION_2LINE_TXRX
#define CH32V003_SPI_CLK_MODE_POL0_PHA0
#define CH32V003_SPI_NSS_SOFTWARE_ANY_MANUAL // #define CH32V003_SPI_NSS_HARDWARE_PC0
#define CH32V003_SPI_IMPLEMENTATION
#include <ch32v003_SPI.h>

#ifdef SOLVE
#include "solve.h"
#endif
#define PIN_FLASH_CS PC4

#define CHALLENGE_STATUS_ADDR 0x900000
#define RESET_MAX_JIFFIES 1000000

uint32_t initial_jiffies = 0;

struct challenge_status_s
{
    uint32_t jiffies;
    uint32_t count;
} status;

void blink(int n)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        funDigitalWrite(PC3, FUN_HIGH);
        Delay_Ms(100);
        funDigitalWrite(PC3, FUN_LOW);
        Delay_Ms(100);
    }
}

void handleChallengeStatus()
{
    flash_read(CHALLENGE_STATUS_ADDR, &status, sizeof(struct challenge_status_s));

    // First time!
    if ((status.jiffies == 0xffffffff) && (status.count == 0xffffffff))
    {
        status.count = 0;

    }
    // New Reset
    else if (((SysTick->CNT - initial_jiffies) <= status.jiffies) && (status.jiffies < RESET_MAX_JIFFIES))
    {
        // Increase reset count
        status.count++;
    }

    status.jiffies = SysTick->CNT - initial_jiffies;

    flash_erase_block(CHALLENGE_STATUS_ADDR);
    flash_write(CHALLENGE_STATUS_ADDR, &status, sizeof(struct challenge_status_s));
}

void resetChallengeStatus()
{
    flash_erase_block(CHALLENGE_STATUS_ADDR);
}

int setup()
{
    int err = -1;

    initial_jiffies = SysTick->CNT;

    if (flash_init(PIN_FLASH_CS))
    {
        blink(1);
        printf("Flash initialization success\r\n");

        blink(2);

        flash_load_ext_cmds();
    }
    else
    {
        printf("Flash initialization failed\r\n");

        blink(4);

        funDigitalWrite(PC3, FUN_HIGH);

        goto error;
    }

    handleChallengeStatus();

	printf("Initializing (Reset count: %ld)...\r\n", status.count);

    blink(1);

    // blink(status.count);

    if (status.count >= 3)
    {

        printf("Resetting challenge...\r\n");
        resetChallengeStatus();

        printf("Resetting quest...\r\n");
        setupQuest();

        printf("Done.\r\n");

        funDigitalWrite(PC3, FUN_HIGH);

        // goto error;
    }

#ifdef SOLVE
    solve();
#else
#ifndef GOLD_CHALLENGE
    plunderLoad();
#endif

    printf(INTRO_MESSAGE "\r\n");
#endif

    err = 0;
error:
    return err;
}

void challenges(char * challenge)
{
    handleChallengeStatus();
    // blink(1);

#ifdef SOLVE
    printf("Done solving.\r\n");
#else

#ifdef GOLD_CHALLENGE
    attempts(challenge);

    if (persuasion() < 0)
    {
        printf("Probably not...\r\n");

        goto done;
    }

#else
    if (palisade() < 0)
    {
        printf("This is not the right header..." "\r\n");

        goto done;
    }

    if (parapet() < 0)
    {
        printf("Invalid Header" "\r\n");

        goto done;
    }

    int err = postern();

    if (err < 0)
    {
        printf("Error in response." "\r\n");
        goto done;
    }
    else if (err == 1)
    {
        printf("Invalid padding" "\r\n");

        goto done;
    }

    digForTreasure();

    if (treasuryVisit() < 0)
    {
        printf("The secret is still safe!" "\r\n");
    }
    else
    {
        printf("THE SECRET IS REVEALED!" "\r\n");
    }
#endif
    blink(1);

done:
    // Delay_Ms(1000);
    return;
#endif
}

unsigned int xtoi(char c)
{
    switch (c)
    {
        case 'F':
        case 'E':
        case 'D':
        case 'C':
        case 'B':
        case 'A':
            return 10 + (c - 'A');
        case 'f':
        case 'e':
        case 'd':
        case 'c':
        case 'b':
        case 'a':
            return 10 + (c - 'a');
        case '9':
        case '8':
        case '7':
        case '6':
        case '5':
        case '4':
        case '3':
        case '2':
        case '1':
        case '0':
            return c - '0';
        default:
            return 0xff;
    }

    return 0xff;
}

static uint32_t atox(char * c)
{
    int v;
    uint32_t total = 0;

    while ((v = xtoi(*c)) != 0xff)
    {
        total = total * 16 + v;

        c++;
    }

    return total;
}

int strcmp(const char *l, const char *r);
int memcmp(const void *vl, const void *vr, size_t n);

static void parseCmd(char * data, size_t len)
{
    size_t i;

    if (!memcmp(CMD_SOLVE, data, sizeof(CMD_SOLVE) - 1))
    {
        challenges(data + sizeof(CMD_SOLVE));
    }
    if (!strcmp(CMD_ASSERT, data))
    {
        funDigitalWrite(PIN_FLASH_CS, FUN_LOW);
    }
    else if (!strcmp(CMD_RELEASE, data))
    {
        funDigitalWrite(PIN_FLASH_CS, FUN_HIGH);
    }
    else if (!strcmp(CMD_BEGIN, data))
    {
        SPI_init();
        SPI_begin_8();
    }
    else if (!strcmp(CMD_END, data))
    {
        SPI_end();
    }
    else if (!strcmp(CMD_RESET, data))
    {
        resetChallengeStatus();
        setupQuest();
    }
    else if (!memcmp(CMD_DATA, data, 4))
    {
        if (len <= sizeof(CMD_DATA) - 1)
        {
            return;
        }

        // Data starts only after command
        i = 4;

        while (i < len)
        {
            if (xtoi(data[i]) == 0xff)
            {
                i++;
                continue;
            }

            uint8_t in = atox(&data[i]);

            uint8_t out = SPI_transfer_8(in);

            printf("%02x ", out);

            // Find the next non-number, which follows a number
            // Finish the current number
            while (xtoi(data[i]) != 0xff)
            {
                i++;
            }

            // Skip all the next non-numbers
            while (xtoi(data[i]) == 0xff)
            {
                i++;
            }
        }

        printf("\r\n");
    }
}

static void handleSerialRead()
{
    char data[128];
    size_t len = 0;

    printf(">> ");

    while (len < sizeof(data) - 1)
    {
        data[len++] = _gets();

        if ((data[len - 1] == '\r') || (data[len - 1] == '\n'))
        {
            printf("\r\n");

            break;
        }
        else
        {
            printf("%c", data[len - 1]);
        }
    }

    data[len - 1] = '\0';

    // Parse input
    parseCmd(data, len);

    printf("\r\n");
}

int main()
{
	// Enable GPIOs
	funGpioInitAll();

	funPinMode( PC3, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );

    // blink(3);

    if (setup() < 0)
    {
        while (1);
    }

    while (1)
    {
        handleSerialRead();
    }
}
