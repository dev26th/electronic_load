#include "ringbuffer.h"

#include <stddef.h>

#define SIZE 32

static volatile uint8_t begin;
static volatile uint8_t end;
static uint8_t buf[SIZE];

bool RINGBUFFER_addIfNotFull(uint8_t v) {
    uint8_t b = begin;
    uint8_t e = end;

    if((e - b) >= SIZE) return false;

    buf[e % SIZE] = v;
    end = e + 1;

    return true;
}

bool RINGBUFFER_takeIfNotEmpty(uint8_t* v) {
    uint8_t b = begin;
    uint8_t e = end;

    if(b == e) return false;

    *v = buf[b % SIZE];
    begin = b + 1;

    return true;
}

