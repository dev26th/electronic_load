#include "adc.h"

#include "stm8.h"

static uint16_t res;
static uint8_t nCount;
static ADC_onResult_t _onResult;

void ADC_init(void) {
    ADC1->CR1 |= ADC1_CR1_SPSEL_6;
    ADC1->CR1 |= ADC1_CR1_ADON;  // wake up
    ADC1->CR2 |= ADC1_CR2_ALIGN; // right-align
}

void ADC_start(uint8_t ch, uint8_t n, ADC_onResult_t onResult) {
    nCount = n;
    res = 0;
    _onResult = onResult;

    ADC1->CR3 &= ~ADC1_CR3_OVR;
    ADC1->CSR  = ADC1_CSR_EOCIE | ch;
    if(n == 1)
        ADC1->CR1 &= ~ADC1_CR1_CONT;
    else
        ADC1->CR1 |= ADC1_CR1_CONT;
    ADC1->CR3 |= ADC1_CR3_DBUF;
    ADC1->CR1 |= ADC1_CR1_ADON;
}

void ADC_ADC1_eoc(void) __interrupt(IRQN_ADC1_EOC) {
    ADC1->CSR &= ~ADC1_CSR_EOC;

    res += ADC1->DRL | ((uint16_t)ADC1->DRH << 8);

    --nCount;
    if(nCount == 1) ADC1->CR1 &= ~ADC1_CR1_CONT;
    //if(nCount == 0) _onResult(res);
}

