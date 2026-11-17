#ifndef UTILS_H
#define UTILS_H

#include <cstdint>

int32_t decodeBigEndianInt32(uint8_t *buffer);
int16_t decodeBigEndianInt16(uint8_t *buffer);

#endif
