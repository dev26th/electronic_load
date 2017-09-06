#ifndef _BEEP_H_
#define _BEEP_H_

#include "stm8.h"

enum BEEP_freq {
    BEEP_freq_None = 0x0F,
    BEEP_freq_1k   = BEEP_CSR_BEEPSEL_1KHZ,
    BEEP_freq_2k   = BEEP_CSR_BEEPSEL_2KHZ,
    BEEP_freq_4k   = BEEP_CSR_BEEPSEL_4KHZ
};

// Beeper on PD4
void BEEP_init(void);

void BEEP_process(void);

void BEEP_beep(enum BEEP_freq f, uint8_t ms);

#endif // _BEEP_H_
