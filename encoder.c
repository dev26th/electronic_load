#include "encoder.h"

#include <stdbool.h>

#include "stm8.h"
#include "systemtimer.h"

#define ENCODER_BOUNCE_TIME_MS 4

struct EncoderState {
    volatile uint8_t idr;
    volatile uint32_t lastCheck;
};
static struct EncoderState encoderState;

void ENCODER_init(void) {
    GPIOB->CR1 |= GPIO_CR1_5 | GPIO_CR1_4;  // pull-up

    encoderState.idr = GPIOB->IDR;
}

void ENCODER_process(void) {
    uint8_t idr;

    //if(SYSTEMTIMER_ms - encoderState.lastCheck < ENCODER_BOUNCE_TIME_MS) return;

    idr = GPIOB->IDR;
    if(idr != encoderState.idr) {
        bool   a  = (idr & GPIO_IDR_5);
        bool   b  = (idr & GPIO_IDR_4);
        bool   a0 = (encoderState.idr & GPIO_IDR_5);
        bool   b0 = (encoderState.idr & GPIO_IDR_4);
        int8_t d;

        if(b) {
                if(a0) d = -1;
                else   d = +1;
        }
        else if(a && !b) {
                if(!b0) d = -1;
                else    d = +1;
        }
        else {
                if(!b0) d = +1;
                else    d = -1;
        }

        if(a && b) ENCODER_onChange(d);
        encoderState.idr = idr;
        encoderState.lastCheck = SYSTEMTIMER_ms;
    }
}

