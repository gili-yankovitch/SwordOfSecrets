#ifndef __RDPROT__
#define __RDPROT__

#define FLASH_OBKEYR ((volatile uint32_t *)0x40022008)
#define FLASH_KEYR ((volatile uint32_t *)0x40022004)
#define FLASH_CTLR ((volatile uint32_t *)0x40022010)
#define FLASH_STATR ((volatile uint32_t *)0x4002200C)
#define RDPRT_KEY 0x000000A5
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xCDEF89AB
#define FLASH_ADDR ((volatile uint32_t *)0x40022014)
#define FLASH_OBR ((volatile uint32_t *)0x4002201C)
#define FLASH_RDPR (*FLASH_OBR & (1 << 1))
#define RDPR ((volatile uint16_t *)0x1FFFF800)
#define RDPR_ON 0x1331
#define RDPR_OFF 0x5AA5

void handleFlashRDPROT();
void userSelectProg();
void lockFlash();
void unlockFlash();


#endif // __RDPROT__
