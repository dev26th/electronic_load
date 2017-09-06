#ifndef _LOAD_H_
#define _LOAD_H_

#include <stdint.h>
#include <stdbool.h>

#include "stm8.h"

// PWM on PC1/TIM1_CH1
void LOAD_init(void);

void LOAD_set(uint16_t v);

inline void LOAD_start(void) {
    GPIOE->ODR &= ~GPIO_ODR_5;
}

inline void LOAD_stop(void) {
    GPIOE->ODR |= GPIO_ODR_5;
}

inline bool LOAD_isStable(void) {
    return (GPIOC->IDR & GPIO_IDR_2);
}

#endif // _LOAD_H_
