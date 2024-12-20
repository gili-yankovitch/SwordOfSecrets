#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <SerialFlash.h>
#include "aes.hpp"
#include "rdprot.h"
#include "armory.h"
#include "keys.h"

#define PIN_FLASH_CS PD4
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

    Serial.println("Started");

    if (!flash.begin(PIN_FLASH_CS))
    {
        // Serial.println(F("SPI Flash not detected."));

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

    plunderLoad();

    Serial.println(INTRO_MESSAGE);
#endif

    blink(1);
}

void loop()
{
#ifdef SETUP
    blink(1);
#else
    int err;

    if (palisade() < 0)
    {
        Serial.println("This is not the right header...");

        goto done;
    }

    if (parapet() < 0)
    {
        Serial.println("Invalid Header");

        goto done;
    }

    err = postern();

    if (err < 0)
    {
        Serial.println("Error in response.");
        goto done;
    }
    else if (err == 1)
    {
        Serial.println("Invalid padding");

        goto done;
    }

    digForTreasure();

    if (treasuryVisit() < 0)
    {
        Serial.println("The secret is still safe!");
    }
    else
    {
        Serial.println("THE SECRET IS REVEALED!");
    }

    blink(4);

done:
#endif
    delay(1000);
}
