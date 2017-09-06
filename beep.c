#include "beep.h"

#include "stm8.h"
#include "system.h"
#include "systemtimer.h"
#include "flash.h"
#include "uart.h"

// ====================================================================================================================

static uint32_t start;
static uint8_t duration;

inline void on(void) {
    BEEP->CSR |= BEEP_CSR_BEEPEN;
}

inline void off(void) {
    BEEP->CSR &= ~BEEP_CSR_BEEPEN;
}

void BEEP_init(void) {
#define BEEP_CALIBRATION 14
    if(!(OPT->AFR & OPT_AFR_D4_BEEP)) {
        UART_write("fix AFR7\r\n");
        FLASH_unlockOpt();
        OPT->AFR  |=  OPT_AFR_D4_BEEP;
        OPT->NAFR &= ~OPT_AFR_D4_BEEP;
        FLASH_waitOpt();
        FLASH_lockOpt();
        SYSTEM_reset();
    }
    BEEP->CSR = BEEP_CALIBRATION;
}

void BEEP_process(void) {
    if(duration && (SYSTEMTIMER_ms - start > duration)) {
        duration = 0;
        off();
    }
}

void BEEP_beep(enum BEEP_freq f, uint8_t ms) {
    if(f == BEEP_freq_None) {
        off();
        duration = 0;
    }
    else {
        BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_BEEPSEL) | f;
        on();
        start    = SYSTEMTIMER_ms;
        duration = ms;
    }
}

