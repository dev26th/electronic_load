#include "button.h"

#include "stm8.h"
#include "systemtimer.h"

#define BUTTON_BOUNCE_TIME_MS 20
#define BUTTON_LONG_TIME_MS 1000

static volatile bool longPress;
static volatile bool pending;
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

    if(pending) return;
    if((SYSTEMTIMER_ms - lastCheck) < BUTTON_BOUNCE_TIME_MS) return;

    p = BUTTON_isPressed();
    if(p && pressed && (SYSTEMTIMER_ms - pressTime) >= BUTTON_LONG_TIME_MS) {
        if(!pressProcessed) {
            longPress = true;
            pending = true;
            pressProcessed = true;
        }
    }
    else if(p != pressed) {
        if(p) {
            pressTime = SYSTEMTIMER_ms;
        }
        else if(!pressProcessed) {
            longPress = false;
            pending = true;
        }
        pressed = p;
        lastCheck = SYSTEMTIMER_ms;
        pressProcessed = false;
    }
}

void BUTTON_process(void) {
    int8_t l;
    if(!pending) return;

    disable_irq();
    l = longPress;
    pending = false;
    enable_irq();

    BUTTON_onRelease(l);
}

