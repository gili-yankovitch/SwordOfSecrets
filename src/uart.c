#include <ch32v003fun.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

void __attribute__(( noinline, used, section(".topflash.text") )) uartInit()
{
	SetupUART(UART_BRR);
}

bool __attribute__(( noinline, used, section(".topflash.text") )) uartAvailable()
{
    return USART1->STATR & USART_FLAG_RXNE;
}

uint8_t __attribute__(( noinline, used, section(".topflash.text") )) _gets()
{
    while (!(USART1->STATR & USART_FLAG_RXNE));
    uint8_t c = (uint8_t)(USART1->DATAR & (uint8_t)0x00FF);
    return c;
}

ssize_t __attribute__(( noinline, used, section(".topflash.text") )) read(void * buf, size_t len)
{
    size_t i;

    for (i = 0; i < len; ++i)
    {
        *((uint8_t *)buf + i) = (uint8_t)(_gets() & 0xff);
    }

    return len;
}
