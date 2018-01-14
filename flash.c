#include "flash.h"

void FLASH_unlockProg(void) {
    FLASH->PUKR = FLASH_KEY1;
    FLASH->PUKR = FLASH_KEY2;
    while(!(FLASH->IAPSR & FLASH_IAPSR_PUL));
}

void FLASH_lockProg(void) {
    FLASH->IAPSR &= FLASH_IAPSR_PUL;
}

void FLASH_unlockData(void) {
    FLASH->DUKR = FLASH_KEY2;
    FLASH->DUKR = FLASH_KEY1;
    while(!(FLASH->IAPSR & FLASH_IAPSR_DUL));

    // FIXME without this dummy delay first 4 bytes of EEPROM are written as zero sometimes
    {
        volatile uint16_t i;
        for(i = 0; i < 10000; ++i) {}
    }
}

void FLASH_waitData(void) {
    while(!(FLASH->IAPSR & FLASH_IAPSR_EOP));
}

void FLASH_lockData(void) {
    FLASH->IAPSR &= FLASH_IAPSR_DUL;
}

void FLASH_waitOpt(void) {
    FLASH_waitData();
}

void FLASH_unlockOpt(void) {
    FLASH_unlockData();
    FLASH->CR2  |= FLASH_CR2_OPT;
    FLASH->NCR2 &= ~FLASH_NCR2_NOPT;
}

void FLASH_lockOpt(void) {
    FLASH->CR2  &= ~FLASH_CR2_OPT;
    FLASH->NCR2 |= FLASH_NCR2_NOPT;
    FLASH_lockData();
}

