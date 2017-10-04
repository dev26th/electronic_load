#include "encoder.h"

#include <stdbool.h>

#include "stm8.h"
#include "systemtimer.h"

static int8_t encoderValue;

void ENCODER_init(void) {
    GPIOB->CR1 |= GPIO_CR1_5 | GPIO_CR1_4;  // pull-up
    GPIOB->CR2 |= GPIO_CR2_4; // interrupt

    EXTI->CR1 |= EXTI_CR1_PBIS_FALLING;
}

void ENCODER_process(void) {
    int8_t value;
    disable_irq();
    value = encoderValue;
    encoderValue = 0;
    enable_irq();

    if(value != 0)
        ENCODER_onChange(value);
}

void ENCODERBUTTON_exti(void) __interrupt(IRQN_EXTI1) {
    encoderValue += (GPIOB->IDR & GPIO_IDR_5) ? 1 : -1;
}

