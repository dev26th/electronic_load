#include "uart.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "stm8.h"
#include "settings.h"
#include "strings.h"

void UART_write(const char *str) {
    for(; *str; ++str)
        UART_send((uint8_t)*str);
}

void UART_writeHexU8(uint8_t v) {
    UART_send(HEX_DIGITS[v >> 4]);
    UART_send(HEX_DIGITS[v & 0x0F]);
}

void UART_writeHexU16(uint16_t v) {
    UART_writeHexU8(v >> 8);
    UART_writeHexU8(v & 0xFF);
}

void UART_writeHexU32(uint32_t v) {
    UART_writeHexU8(v >> 24);
    UART_writeHexU8(v >> 16);
    UART_writeHexU8(v >> 8);
    UART_writeHexU8(v & 0xFF);
}

void UART_writeDecU32(uint32_t v) {
    char buf[11];
    uint8_t i;
    for(i = 0; i < 10; ++i) buf[i] = ' ';
    buf[10] = '\0';

    i = 10;
    do {
        uint8_t d = v % 10;
        v /= 10;
        buf[--i] = '0' + d;
    }
    while(v != 0);

    UART_write(buf);
}
