#include "encoderbutton.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stm8.h"
#include "systemtimer.h"

#define BUTTON_BOUNCE_TIME_MS 20

struct ButtonState {
    volatile bool pressed;
    volatile uint32_t lastCheck;
};
static struct ButtonState encoderButtonState;

void ENCODERBUTTON_init(void) {
    GPIOC->CR1 |= GPIO_CR1_3;  // pull-up
}

void ENCODERBUTTON_process(void) {
    bool pressed;

    if(SYSTEMTIMER_ms - encoderButtonState.lastCheck < BUTTON_BOUNCE_TIME_MS) return;

    pressed = !(GPIOC->IDR & GPIO_CR1_3);
    if(pressed != encoderButtonState.pressed) {
        if(!pressed) ENCODERBUTTON_onRelease();
        encoderButtonState.pressed = pressed;
        encoderButtonState.lastCheck = SYSTEMTIMER_ms;
    }
}

