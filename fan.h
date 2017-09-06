#ifndef _FAN_H_
#define _FAN_H_

#include <stdint.h>

#include "stm8.h"

// PWM on PD0/TIM3_CH2
void FAN_init(void);

inline void FAN_set(uint8_t v) {
    TIM3->CCR2L = v;
}

#endif // _FAN_H_
