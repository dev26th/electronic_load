#ifndef _SYSTEM_TIMER_H_
#define _SYSTEM_TIMER_H_

#include <stdint.h>

#include "stm8.h"

#define SYSTEMTIMER_MS_PER_TICK 2
#define SYSTEMTIMER_TICKS_PER_S (1000/SYSTEMTIMER_MS_PER_TICK)

extern volatile uint32_t SYSTEMTIMER_ms; // with overflow every ~50 days

void SYSTEMTIMER_onTick(void); // called every 10 ms
void SYSTEMTIMER_onTack(void); // called every 10 ms

void SYSTEMTIMER_delayMs(uint32_t ms);

void SYSTEMTIMER_waitMsOnStart(uint32_t ms);

void SYSTEMTIMER_init(void); // 1 kHz

void SYSTEMTIMER_TIM2_overflow(void) __interrupt(IRQN_TIM2_UP);

#endif // _SYSTEM_TIMER_H_
