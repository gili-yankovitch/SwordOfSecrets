#include <stdio.h>
#include <stdint.h>
#include "ch32v003fun/ch32v003fun.h"
#include "aes.h"
#include "rdprot.h"
#include "armory.h"
#include "keys.h"
#include "flash.h"

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

	funPinMode( PC3, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );

	SetupUART(UART_BRR);

    if (flash_init(PIN_FLASH_CS))
    {
        printf("Flash initialization success\r\n");

        // blink(2);
    }
    else
    {
        printf("Flash initialization failed\r\n");

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

        goto error;
    }

// #ifdef SETUP
// #else
#ifdef SOLVE
    solve();
#else
    // Protect internal flash from dumping the firmware
    // Didn't think you'd get the keys THAT easily eh? >:)
    // handleFlashRDPROT();

    plunderLoad();

    printf(INTRO_MESSAGE "\r\n");
#endif
// #endif

    err = 0;
error:
    return err;
}

void loop()
{
    handleChallengeStatus();

// #ifdef SETUP
    blink(1);
// #else
#ifdef SOLVE
    printf("Done solving.\r\n");
#else
    int err;

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

    err = postern();

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

    blink(4);

done:
    Delay_Ms(1000);
#endif
// #endif
}

int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();

    if (setup() < 0)
    {
        while (1);
    }

    while (1)
    {
        loop();
    }
}
