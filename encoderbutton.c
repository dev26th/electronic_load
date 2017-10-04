#include "encoderbutton.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stm8.h"
#include "systemtimer.h"

#define BUTTON_BOUNCE_TIME_MS 20

static volatile bool pending;

void ENCODERBUTTON_init(void) {
    GPIOC->CR1 |= GPIO_CR1_3;  // pull-up
}

// ~3.4 us
void ENCODERBUTTON_cycle(void) {
    static bool pressed;
    static uint32_t lastCheck;
    bool p;

    if(pending) return;
    if(SYSTEMTIMER_ms - lastCheck < BUTTON_BOUNCE_TIME_MS) return;

    p = !(GPIOC->IDR & GPIO_CR1_3);
    if(p != pressed) {
        if(!p) pending = true;
        pressed   = p;
        lastCheck = SYSTEMTIMER_ms;
    }
}

void ENCODERBUTTON_process(void) {
    if(!pending) return;

    pending = false;

    ENCODERBUTTON_onRelease();
}

