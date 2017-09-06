#include "system.h"

#include "stm8.h"

void SYSTEM_toHseClock(void) {
    CLK->SWCR |= CLK_SWCR_SWEN;
    CLK->SWR = CLK_SWR_SWI_HSE;
    while(CLK->SWCR & CLK_SWCR_SWBSY);
}

void SYSTEM_reset(void) {
    // use the WWDG for reset
    WWDG->CR = WWDG_CR_WDGA | 0;
}

