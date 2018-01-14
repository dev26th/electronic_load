#include "adc.h"

#include "stm8.h"

static ADC_onResult_t _onResult;
static uint8_t  _counts[ADC_COUNTS_SIZE];
static uint8_t  _ch;
static uint8_t  _n;
static uint8_t  _countMax;
static uint16_t _countValue;

void ADC_init(void) {
    ADC1->CR1 |= ADC1_CR1_SPSEL_6;
    ADC1->CR1 |= ADC1_CR1_ADON;  // wake up
    ADC1->CR2 |= ADC1_CR2_ALIGN; // right-align
}

void ADC_start(uint8_t ch, uint8_t n, ADC_onResult_t onResult) {
    uint16_t i;
    _onResult = onResult;
    _ch = ch;
    _n = n;
    _countMax = 0;
    _countValue = 0;
    for(i = 0; i < ADC_COUNTS_SIZE; ++i) _counts[i] = 0;

    ADC1->CR3 &= ~ADC1_CR3_OVR;
    ADC1->CSR  = ADC1_CSR_EOCIE | _ch;
    ADC1->CR1 |= ADC1_CR1_CONT;
    ADC1->CR3 |= ADC1_CR3_DBUF;
    ADC1->CR1 |= ADC1_CR1_ADON;
}

// ~7 us for non-last one
void ADC_ADC1_eoc(void) __interrupt(IRQN_ADC1_EOC) {
    uint16_t v, c;
    __asm BSET 0x500F, #7 __endasm;
    ADC1->CSR = ADC1_CSR_EOCIE | _ch;

    v = ADC1->DRL | ((uint16_t)ADC1->DRH << 8);
    c = _counts[v] + 1;
    _counts[v] = c;
    if(c > _countMax) {
        _countMax = c;
        _countValue = v;
    }

    --_n;
    if(_n == 1) ADC1->CR1 &= ~ADC1_CR1_CONT;
    if(_n == 0) _onResult(_counts, _countMax, _countValue);
    __asm BRES 0x500F, #7 __endasm;
}

