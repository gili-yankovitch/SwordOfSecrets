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

    // Serial.begin(115200);

    if (!flash.begin(PIN_FLASH_CS))
    {
        // Serial.println(F("SPI Flash not detected."));
        // blink(3);
        while (1)
            ;
    }

    // Serial.print("Booting...\r\n");

    // initDisplay();

    blink(1);
#if 1
    if (!handleFlashRDPROT())
    {
        blink(1);

        setupQuest();
    }
    else
    {
        blink(2);
    }
#else
    setupQuest();
#endif
}

void loop()
{
#if 0
    drawDisplayBuffer();
#endif
    int err;

    if (stage1() < 0)
    {
        return;
    }

    if (stage2() < 0)
    {
        return;
    }

    err = stage3();

    if (err < 0)
    {
        return;
    }
    else if (err == 1)
    {
        Serial.print(".");

        return;
    }

    blink(4);
}
