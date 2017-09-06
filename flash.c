#include "flash.h"

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

