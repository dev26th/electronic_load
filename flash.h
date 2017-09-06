#ifndef _FLASH_H_
#define _FLASH_H_

#include "stm8.h"

inline void FLASH_unlockProg(void) {
    FLASH->PUKR = FLASH_KEY1;
    FLASH->PUKR = FLASH_KEY2;
}

inline void FLASH_lockProg(void) {
    FLASH->IAPSR &= FLASH_IAPSR_PUL;
}

inline void FLASH_unlockData(void) {
    FLASH->DUKR = FLASH_KEY2;
    FLASH->DUKR = FLASH_KEY1;
}

inline void FLASH_lockData(void) {
    FLASH->IAPSR &= FLASH_IAPSR_DUL;
}

void FLASH_unlockOpt(void);

inline void FLASH_waitOpt(void) {
    while(!(FLASH->IAPSR & FLASH_IAPSR_EOP));
}

void FLASH_lockOpt();

#endif // _FLASH_
