#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <SerialFlash.h>
#include "aes.hpp"
#include "rdprot.h"
#include "secboot.h"

#define DATA PC0
#define CLK PD0
#define LOAD PD3

#define PIN_FLASH_CS PC4
SerialFlashChip flash;

void blink(int n)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        digitalWrite(PC3, HIGH);
        delay(100);
        digitalWrite(PC3, LOW);
        delay(100);
    }
}

void setup()
{
    pinMode(PC3, OUTPUT);

    Serial.begin(115200);

    if (!flash.begin(PIN_FLASH_CS))
    {
        Serial.println(F("SPI Flash not detected."));

        blink(3);

        while (1)
            ;
    }

#ifdef SETUP
    setupQuest();
#else
    // Protect internal flash from dumping the firmware
    // Didn't think you'd get the keys THAT easily eh? >:)
    handleFlashRDPROT();
#endif

    blink(1);
}

void loop()
{
#ifdef SETUP
    blink(1);
#else
    int err;

    if (stage1() < 0)
    {
        Serial.print("This is not the right header...\r\n");

        goto done;
    }

    if (stage2() < 0)
    {
        Serial.print("Invalid Header\r\n");

        goto done;
    }

    err = stage3();

    if (err < 0)
    {
        goto done;
    }
    else if (err == 1)
    {
        Serial.print("Invalid padding\r\n");

        goto done;
    }

    Serial.println("Secret unlocked!\r\n");

    blink(4);

done:
#endif
    delay(1000);
}
