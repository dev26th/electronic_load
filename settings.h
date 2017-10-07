#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#define STM8S105
#define CPU_F    12000000ul
#define BAUD     19200     // FIXME 115200 doesn't work for long commands, e.g. Command_WriteCal
                           //       looks like it interferences with ADC-interrupts

#endif // _SETTINGS_H_
