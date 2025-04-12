#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include <stdbool.h>

void uartInit();

bool uartAvailable();

uint8_t _gets();

ssize_t read(void * buf, size_t len);

#endif
