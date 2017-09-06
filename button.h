#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>

#include "stm8.h"

void BUTTON_onRelease(bool longpress);
void BUTTON_init(void);
void BUTTON_process(void);
inline bool BUTTON_isPressed(void) { return !(GPIOC->IDR & GPIO_IDR_4); }

#endif // _BUTTON_H_
