#include "button.h"

#include "stm8.h"
#include "systemtimer.h"

#define BUTTON_BOUNCE_TIME_MS 20
#define BUTTON_LONG_TIME_MS 1000

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

void BUTTON_init(void) {
    GPIOC->CR1 |= GPIO_CR1_4;  // pull-up
}

// ~3.8 us
void BUTTON_cycle(void) {
    bool p;

    if(pressValue != PressValue_None) return;
    if((SYSTEMTIMER_ms - lastCheck) < BUTTON_BOUNCE_TIME_MS) return;

    p = BUTTON_isPressed();
    if(p && pressed && (SYSTEMTIMER_ms - pressTime) >= BUTTON_LONG_TIME_MS) {
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

void BUTTON_process(void) {
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
        case PressValue_Short: BUTTON_onRelease(false); break;
        case PressValue_Long:  BUTTON_onRelease(true);  break;
    }
}

