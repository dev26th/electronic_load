#include "crc.h"

char crc8(char crc, char b) {
    crc ^= b;
    for(char i = 0; i < 8; ++i)
        crc = crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1;
    return crc;
}
