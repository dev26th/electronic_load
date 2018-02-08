#include "encoderbutton.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stm8.h"
#include "systemtimer.h"

#define ENCODERBUTTON_BOUNCE_TIME_MS 20
#define ENCODERBUTTON_LONG_TIME_MS 1000

// FIXME merge with button.c

enum PressValue {
    PressValue_None,
    PressValue_Short,
    PressValue_Long
};
static volatile enum PressValue pressValue;
static          enum PressValue pressValueCopy;

static volatile bool pressed;
static volatile bool pressProcessed;
static volatile uint32_t lastCheck;
static volatile uint32_t pressTime;

void ENCODERBUTTON_init(void) {
    GPIOC->CR1 |= GPIO_CR1_3;  // pull-up
}

// ~3.8 us
void ENCODERBUTTON_cycle(void) {
    bool p;

    if(pressValue != PressValue_None) return;
    if((SYSTEMTIMER_ms - lastCheck) < ENCODERBUTTON_BOUNCE_TIME_MS) return;

    p = !(GPIOC->IDR & GPIO_CR1_3);
    if(p && pressed && (SYSTEMTIMER_ms - pressTime) >= ENCODERBUTTON_LONG_TIME_MS) {
        if(!pressProcessed) {
            pressValue = PressValue_Long;
            pressProcessed = true;
        }
    }
    else if(p != pressed) {
        if(p) {
            pressTime = SYSTEMTIMER_ms;
        }
        else if(!pressProcessed) {
            pressValue = PressValue_Short;
        }
        pressed = p;
        lastCheck = SYSTEMTIMER_ms;
        pressProcessed = false;
    }
}

void ENCODERBUTTON_process(void) {
    // Atomic:
    //   pressValueCopy = pressValue;
    //   pressValue = 0;
    __asm
    CLR     A
    EXG     A, _pressValue
    LD      _pressValueCopy, A
    __endasm;

    switch(pressValueCopy) {
        case PressValue_None:  break;
        case PressValue_Short: ENCODERBUTTON_onRelease(false); break;
        case PressValue_Long:  ENCODERBUTTON_onRelease(true);  break;
    }
}

