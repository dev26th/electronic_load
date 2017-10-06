#include "adc.h"

#include "stm8.h"

static uint16_t res[4];
static uint8_t nCount;

void ADC_init(void) {
    ADC1->CR1 |= ADC1_CR1_SPSEL_6;
    ADC1->CR1 |= ADC1_CR1_ADON;  // wake up
    ADC1->CR2 |= ADC1_CR2_ALIGN; // right-align
}

void ADC_startScanCont(uint8_t n) {
    nCount = n;
    res[0] = 0;
    res[1] = 0;
    res[2] = 0;
    res[3] = 0;

    ADC1->CR3 &= ~ADC1_CR3_OVR;
    ADC1->CSR  = ADC1_CSR_EOCIE | 3;
    ADC1->CR1 |= ADC1_CR1_CONT;
    ADC1->CR3 |= ADC1_CR3_DBUF;
    ADC1->CR2 |= ADC1_CR2_SCAN;
    ADC1->CR1 |= ADC1_CR1_ADON;
}

void ADC_ADC1_eoc(void) __interrupt(IRQN_ADC1_EOC) {
    ADC1->CSR = ADC1_CSR_EOCIE | 3;

    res[0] += ADC1->DB[0].L | (((uint16_t)ADC1->DB[0].H) << 8);
    res[1] += ADC1->DB[1].L | (((uint16_t)ADC1->DB[1].H) << 8);
    res[2] += ADC1->DB[2].L | (((uint16_t)ADC1->DB[2].H) << 8);
    res[3] += ADC1->DB[3].L | (((uint16_t)ADC1->DB[3].H) << 8);

    --nCount;
    if(nCount == 1) ADC1->CR1 &= ~ADC1_CR1_CONT;
    if(nCount == 0) ADC_onResult(res);
}

