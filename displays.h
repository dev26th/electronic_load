#ifndef _DISPLAYS_H_
#define _DISPLAYS_H_

#include "stm8.h"
#include "settings.h"

#define DISPLAYS_DOT       0x80
#define DISPLAYS_SYM_SPACE 0x00
#define DISPLAYS_SYM_DASH  0x40
#define DISPLAYS_SYM_J     0x0E
#define DISPLAYS_SYM_n     0x54
#define DISPLAYS_SYM_o     0x5c
#define DISPLAYS_SYM_P     0x73
#define DISPLAYS_SYM_r     0x50
#define DISPLAYS_SYM_u     0x1c
#define DISPLAYS_SYM_0     0x3f
#define DISPLAYS_SYM_1     0x06
#define DISPLAYS_SYM_2     0x5b
#define DISPLAYS_SYM_3     0x4f
#define DISPLAYS_SYM_4     0x66
#define DISPLAYS_SYM_5     0x6d
#define DISPLAYS_SYM_6     0x7d
#define DISPLAYS_SYM_7     0x07
#define DISPLAYS_SYM_8     0x7f
#define DISPLAYS_SYM_9     0x6f
#define DISPLAYS_SYM_A     0x77
#define DISPLAYS_SYM_b     0x7c
#define DISPLAYS_SYM_C     0x39
#define DISPLAYS_SYM_d     0x5e
#define DISPLAYS_SYM_E     0x79
#define DISPLAYS_SYM_F     0x71

#define DISPLAYS_CHAR_E    "\0x79"

extern const uint8_t DISPLAYS_DIGITS[];

void DISPLAYS_init(void);
void DISPLAYS_start(void);
void DISPLAYS_display(const uint8_t* a, const uint8_t* b);

#endif // _DISPLAYS_H_
