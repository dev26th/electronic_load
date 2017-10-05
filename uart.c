#include "uart.h"

#include <stddef.h>
#include <stdint.h>

#include "stm8.h"
#include "settings.h"
#include "strings.h"
#include "ringbuffer.h"

enum RxState {
    RxState_Start,
    RxState_H,       // waiting for high byte half
    RxState_L,       // waiting low byte half
    RxState_Stop     // '\n' received, but buffer not read yet
};
static enum RxState rxState = RxState_Start;

static uint8_t rxBuf[UART_RXBUF_SIZE];
static uint8_t rxBufPos;
static bool hasChecksum;

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
    UART_writeDecU64(v, 10);
}

void UART_writeDecU64(uint64_t v, uint8_t n) {
    char buf[22];
    uint8_t i;
    for(i = 0; i < n; ++i) buf[i] = ' ';
    buf[n] = '\0';

    i = n;
    do {
        uint8_t d = v % 10;
        v /= 10;
        buf[--i] = '0' + d;
    }
    while(v != 0);

    UART_write(buf);
}

static inline void resetRx(void) {
    rxBufPos = 0;
    rxState  = RxState_Start;
}

const uint8_t* UART_getRx(uint8_t* size) {
    if(rxState == RxState_Stop) {
        *size = rxBufPos;
        return rxBuf;
    }
    else {
        return NULL;
    }
}

bool UART_hasChecksum(void) {
    return hasChecksum;
}

void UART_rxDone(void) {
    resetRx();
}

void UART_process(void) {
    uint8_t v;
    uint8_t b;
    if(!RINGBUFFER_takeIfNotEmpty(&b)) return;

    switch(rxState) {
        case RxState_Start:
            if(b == 'S') {            // start with checksum
                rxState = RxState_H;
                hasChecksum = true;
            }
            else if(b == 's') {       // start, no checksum
                rxState = RxState_H;
                hasChecksum = false;
            }

            break;

        case RxState_H:
        case RxState_L:
            if(b >= '0' && b <= '9') {
                v = b - '0';
            }
            else if(b >= 'A' && b <= 'F') {
                v = b - 'A' + 10;
            }
            else if(b == '\n') { // ignore
                break;
            }
            else if(b == '\r') { // stop
                if(rxState == RxState_L)   // unexpected, reset
                    resetRx();
                else
                    rxState = RxState_Stop;
                break;
            }
            else {    // unexpected symbol, reset
                resetRx();
                break;
            }

            if(rxState == RxState_H) {
                rxBuf[rxBufPos] = (v << 4);
                rxState = RxState_L;
            }
            else {
                rxBuf[rxBufPos] |= v;
                rxState = RxState_H;
                ++rxBufPos;
                if(rxBufPos >= UART_RXBUF_SIZE) resetRx();
            }

            break;

        case RxState_Stop:
            nop();  // who are the EVELYN and the DOG?
            break; // ignore all
    }
}

void UART_UART2_rx(void) __interrupt(IRQN_UART2_RX) {
    RINGBUFFER_addIfNotFull(UART2->DR);
    UART2->SR = (uint8_t)~UART_SR_RXNE; // reset RXNE
}

