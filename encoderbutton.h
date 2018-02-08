#ifndef _ENCODERBUTTON_H_
#define _ENCODERBUTTON_H_

#include <stdbool.h>

void ENCODERBUTTON_init(void);
void ENCODERBUTTON_onRelease(bool longpress);
void ENCODERBUTTON_cycle(void);
void ENCODERBUTTON_process(void);

#endif // _ENCODERBUTTON_H_
