#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

#include "stm8.h"

typedef void (*ADC_onResult_t)(uint16_t res);

void ADC_init(void);

void ADC_start(uint8_t ch, uint8_t n, ADC_onResult_t onResult);

void ADC_ADC1_eoc(void) __interrupt(IRQN_ADC1_EOC);

#endif // _ADC_H_
