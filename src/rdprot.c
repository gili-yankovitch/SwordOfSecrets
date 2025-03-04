#include <stdio.h>
#include "rdprot.h"

void unlockFlashCTLR()
{
    *FLASH_KEYR = FLASH_KEY1;
    *FLASH_KEYR = FLASH_KEY2;
}

void unlockFlashOBKEYR()
{
    *FLASH_OBKEYR = FLASH_KEY1;
    *FLASH_OBKEYR = FLASH_KEY2;
}

void userSelectProg()
{
    // char s[16];
    unlockFlashCTLR();
    unlockFlashOBKEYR();

    while (*FLASH_STATR != 0x8000) ;

    // Set OBG
    *FLASH_CTLR |= 1 << 4;
}

void lockFlash()
{
    *FLASH_KEYR = RDPRT_KEY;
    *RDPR = RDPR_ON;
}

void unlockFlash()
{
    *RDPR = RDPR_OFF;
}

void handleFlashRDPROT()
{
    // char s[64];

    // sprintf(s, "RDPR = %4x LOCK = %d OBWRE = %d\r\n",
    //     *RDPR,
    //     (*FLASH_CTLR & (1 << 7)) != 0,
    //     (*FLASH_CTLR & (1 << 9)) != 0);
    // Serial.print(s);

    // sprintf(s, "Read protection: %d\r\n", (*FLASH_OBR & (1 << 1)) != 0);
    // Serial.print(s);

    if (!FLASH_RDPR)
    {
        userSelectProg();
        lockFlash();
    }
    else
    {
        printf("Internal flash RDProt ENABLED\r\n");
    }
}
