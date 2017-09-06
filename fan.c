#include "fan.h"

void FAN_init(void) {
    TIM3->PSCR  = 1;
    TIM3->ARRH  = 0;
    TIM3->ARRL  = 99;
    TIM3->CCR2H = 0;
    TIM3->CCR2L = 0;
    TIM3->CCMR2 = TIM3_CCMR2_OC2M_PWM1 | TIM3_CCMR2_OC2PE;
    TIM3->CCER1 = TIM3_CCER1_CC2E;
    TIM3->CR1  |= TIM3_CR1_CEN;
}

