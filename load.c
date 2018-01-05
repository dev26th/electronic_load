#include "load.h"

// use PC1/TIM1_CH1, PE5, PC2
void LOAD_init(void) {
    TIM1->PSCRH = 0;
    TIM1->PSCRL = 0;
    TIM1->ARRH  = 0x7F;
    TIM1->ARRL  = 0xFF;
    TIM1->BKR   = TIM1_BKR_MOE;
    TIM1->CCMR1 = TIM1_CCMR1_OC1M_PWM1 | TIM1_CCMR1_OC1PE;
    TIM1->CCER1 = TIM1_CCER1_CC1E;
    TIM1->CCR1H = 0;
    TIM1->CCR1L = 0;
    TIM1->CR2  |= TIM1_CR2_CCPC;
    TIM1->CR1  |= TIM1_CR1_CEN;

    LOAD_stop();
    GPIOE->DDR |= GPIO_DDR_5;  // output
    GPIOE->CR1 |= GPIO_CR1_5;  // pull-push

    GPIOC->CR1 |= GPIO_CR1_2;  // pull-up
}

void LOAD_set(uint16_t v) {
    v >>= 1; // FIXME move to the coef
    TIM1->CCR1H = v >> 8;
    TIM1->CCR1L = v & 0xFF;
    TIM1->EGR  |= TIM1_EGR_COMG;
}

