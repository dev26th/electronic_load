#include "encoder.h"

#include <stdbool.h>

#include "stm8.h"
#include "systemtimer.h"

static volatile int8_t encoderValue;
static          int8_t encoderValueCopy;

void ENCODER_init(void) {
    GPIOB->CR1 |= GPIO_CR1_5 | GPIO_CR1_4;  // pull-up
    GPIOB->CR2 |= GPIO_CR2_4; // interrupt

    EXTI->CR1 |= EXTI_CR1_PBIS_FALLING;
}

void ENCODER_process(void) {
    // Atomic:
    //   encoderValueCopy = encoderValue;
    //   encoderValue = 0;
    __asm
    CLR     A
    EXG     A, _encoderValue
    LD      _encoderValueCopy, A
    __endasm;

    if(encoderValueCopy != 0)
        ENCODER_onChange(encoderValueCopy);
}

void ENCODERBUTTON_exti(void) __interrupt(IRQN_EXTI1) {
    encoderValue += (GPIOB->IDR & GPIO_IDR_5) ? 1 : -1;
}

