#include "button.h"

#include "stm8.h"
#include "systemtimer.h"

#define BUTTON_BOUNCE_TIME_MS 20
#define BUTTON_LONG_TIME_MS 1000

struct ButtonState {
    volatile bool pressed;
    volatile bool pressProcessed;
    volatile uint32_t lastCheck;
    volatile uint32_t pressTime;
};
static struct ButtonState buttonState;

void BUTTON_init(void) {
    GPIOC->CR1 |= GPIO_CR1_4;  // pull-up
}

void BUTTON_process(void) {
    bool pressed;

    if(SYSTEMTIMER_ms - buttonState.lastCheck < BUTTON_BOUNCE_TIME_MS) return;

    pressed = BUTTON_isPressed();
    if(pressed && buttonState.pressed && (SYSTEMTIMER_ms - buttonState.pressTime) >= BUTTON_LONG_TIME_MS) {
        if(!buttonState.pressProcessed) {
            BUTTON_onRelease(true);
            buttonState.pressProcessed = true;
        }
    }
    else if(pressed != buttonState.pressed) {
        if(pressed)
            buttonState.pressTime = SYSTEMTIMER_ms;
        else if(!buttonState.pressProcessed)
            BUTTON_onRelease(false);
        buttonState.pressed = pressed;
        buttonState.lastCheck = SYSTEMTIMER_ms;
        buttonState.pressProcessed = false;
    }
}

