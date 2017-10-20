#ifndef _FLASH_H_
#define _FLASH_H_

#include "stm8.h"

void FLASH_unlockProg(void);
void FLASH_lockProg(void);

void FLASH_unlockData(void);
void FLASH_waitData(void);
void FLASH_lockData(void);

void FLASH_unlockOpt(void);
void FLASH_waitOpt(void);
void FLASH_lockOpt();

#endif // _FLASH_
