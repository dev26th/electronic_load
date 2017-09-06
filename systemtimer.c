#include "systemtimer.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "stm8.h"
#include "settings.h"

volatile uint32_t SYSTEMTIMER_ms = 0;

bool tickTack;

void SYSTEMTIMER_delayMs(uint32_t ms) {
    uint32_t b = SYSTEMTIMER_ms;
    while(SYSTEMTIMER_ms - b < ms);
}

void SYSTEMTIMER_waitMsOnStart(uint32_t ms) {
    while(SYSTEMTIMER_ms < ms);
}

void SYSTEMTIMER_init(void) {

    // Note: use TIM2 instead of TIM4 for get exact millisecond

#define SYSTEM_TIMER_PSC    5  // 2^n FIXME auto

#if (CPU_F / (1 << SYSTEM_TIMER_PSC) / SYSTEMTIMER_TICKS_PER_S - 1 >= 65536)
    #error "SystemTimer: CPU_F = " ##CPU_F "too big for this prescaler"
#endif
#if (CPU_F % ((1 << SYSTEM_TIMER_PSC) * SYSTEMTIMER_TICKS_PER_S) != 0)
    #warning "SystemTimer: cannot configure exact tick for this CPU_F"
#endif

    uint16_t arr = (CPU_F / (1 << SYSTEM_TIMER_PSC) / SYSTEMTIMER_TICKS_PER_S / 2 - 1);
    TIM2->PSCR = SYSTEM_TIMER_PSC;
    TIM2->ARRH = (uint8_t)(arr >> 8);
    TIM2->ARRL = (uint8_t)arr;
    TIM2->IER  = TIM2_IER_UIE;
    TIM2->CR1 |= TIM2_CR1_CEN;
    rim();
}

void SYSTEMTIMER_TIM2_overflow(void) __interrupt(IRQN_TIM2_UP) {
    TIM2->SR1 = (uint8_t)~TIM2_SR1_UIF;
    SYSTEMTIMER_ms += SYSTEMTIMER_MS_PER_TICK / 2;
    tickTack = !tickTack;
    if(tickTack)
        SYSTEMTIMER_onTick();
    else
        SYSTEMTIMER_onTack();
}

