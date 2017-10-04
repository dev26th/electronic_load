#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <stdint.h>
#include "stm8.h"

void ENCODER_onChange(int8_t d);
void ENCODER_init(void);
void ENCODER_process(void);

void ENCODERBUTTON_exti(void) __interrupt(IRQN_EXTI1);

#endif // _ENCODER_H_
