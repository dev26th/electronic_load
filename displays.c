#include "displays.h"

#include <stdbool.h>

#include "stm8.h"
#include "systemtimer.h"

inline void clock0(void) {
    GPIOC->ODR &= ~GPIO_ODR_5;
}

inline void clock1(void) {
    GPIOC->ODR |= GPIO_ODR_5;
}

inline void dataToInput(void) {
    GPIOC->DDR &= ~GPIO_DDR_6;
    GPIOC->DDR &= ~GPIO_DDR_7;
}

inline void dataToOutput(void) {
    GPIOC->DDR |= GPIO_DDR_6;
    GPIOC->DDR |= GPIO_DDR_7;
}

inline void dataA0(void) {
    GPIOC->ODR &= ~GPIO_ODR_6;
}

inline void dataA1(void) {
    GPIOC->ODR |= GPIO_ODR_6;
}

inline void dataB0(void) {
    GPIOC->ODR &= ~GPIO_ODR_7;
}

inline void dataB1(void) {
    GPIOC->ODR |= GPIO_ODR_7;
}

// Note: in this device SCL is always controlled by the master
static void initI2c(void) {
    // SCL
    GPIOC->DDR |= GPIO_DDR_5; // output
    GPIOC->CR1 |= GPIO_CR1_5; // push-pull
    GPIOC->CR2 &= ~GPIO_CR2_5; // low-speed

    // SDA-A
    GPIOC->CR1 &= ~GPIO_CR1_6; // open-drain
    GPIOC->CR2 &= ~GPIO_CR2_6; // low-speed, no interrupt

    // SDA-B
    GPIOC->CR1 &= ~GPIO_CR1_7; // open-drain
    GPIOC->CR2 &= ~GPIO_CR2_7; // low-speed, no interrupt
}

// ~1/5 of clock-cycle
inline void delay(void) {
    // According to TM1650 datasheet, minimal signal duration is 100 ns,
    // which is unreachable with STM8@16MHz. So, don't add artifical delay.
}

inline void waitBusy(void) {
    // Note: in this device SCL is always controlled by the master
}

static void clockBegin(void) {
    waitBusy();
    clock0();
    delay();
}

static void clockEnd(void) {
    clock1();
    delay();
    delay();
}

static void writeBits(bool bitA, bool bitB) {
    clockBegin();
    dataToOutput();

    if(bitA) dataA1();
    else     dataA0();

    if(bitB) dataB1();
    else     dataB0();

    delay();
    clockEnd();
}

// we are not interested in result, just generate a clock for the slaves
static void readBits(void) {
    clockBegin();
    dataToInput();
    delay();
    clockEnd();
}

static void begin(void) {
    dataToOutput();

    dataA1();
    dataB1();
    delay();
    clock1();
    delay();
    delay();

    waitBusy();

    dataA0();
    dataB0();
    delay();
    delay();
    // SCL will be set to 0 during next clock
}

static void end(void) {
    waitBusy();
    dataToOutput();

    // ensure SDA is low
    clock0();
    delay();
    dataA0();
    dataB0();
    delay();

    // SCL to 1, then SDA to 1
    clock1();
    delay();
    delay();
    dataA1();
    dataB1();
}

static void writeBytes(uint8_t a, uint8_t b) {
    uint8_t i;
    for(i = 8; i > 0; --i) {
        writeBits(a & 0x80, b & 0x80);
        a <<= 1;
        b <<= 1;
    }

    readBits(); // ACK
}

static void sendBytes(uint8_t addr, uint8_t a, uint8_t b) {
    begin();
    writeBytes(addr, addr);
    writeBytes(a, b);
    end();
}

void DISPLAYS_init(void) {
    initI2c();
}

void DISPLAYS_start(void) {
    SYSTEMTIMER_waitMsOnStart(50); // let the TM1650s time to start
    sendBytes(0x48, 0x11, 0x11); // min brightness; dot; on
}

void DISPLAYS_display(const uint8_t* a, const uint8_t* b) {
    uint8_t addr;
    for(addr = 0x68; addr <= 0x6E; addr += 2, ++a, ++b)
        sendBytes(addr, *a, *b);
}

const uint8_t DISPLAYS_DIGITS[] = {
    DISPLAYS_SYM_0,
    DISPLAYS_SYM_1,
    DISPLAYS_SYM_2,
    DISPLAYS_SYM_3,
    DISPLAYS_SYM_4,
    DISPLAYS_SYM_5,
    DISPLAYS_SYM_6,
    DISPLAYS_SYM_7,
    DISPLAYS_SYM_8,
    DISPLAYS_SYM_9,
    DISPLAYS_SYM_A,
    DISPLAYS_SYM_b,
    DISPLAYS_SYM_C,
    DISPLAYS_SYM_d,
    DISPLAYS_SYM_E,
    DISPLAYS_SYM_F
};

