#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

#include "stm8.h"

void ADC_onResult(const uint16_t* res);

void ADC_init(void);

void ADC_startScanCont(uint8_t n);

void ADC_ADC1_eoc(void) __interrupt(IRQN_ADC1_EOC);

#endif // _ADC_H_
