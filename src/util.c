#include "util.h"

uint16_t combine_bytes(uint8_t lsb, uint8_t msb) {
    return ((uint16_t) lsb) | (((uint16_t) msb) << 8);
}

