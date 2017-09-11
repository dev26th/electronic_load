#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>

bool RINGBUFFER_addIfNotFull(uint8_t v);
bool RINGBUFFER_takeIfNotEmpty(uint8_t* v);

#endif // _RINGBUFFER_H_
