#include "utils.h"

int32_t decodeBigEndianInt32(uint8_t *buffer){
  int32_t res = buffer[3];
  res |= buffer[2]<<8;
  res |= buffer[1]<<16;
  res |= buffer[0]<<24;
  return res;
}

int16_t decodeBigEndianInt16(uint8_t *buffer){
  int16_t res = buffer[1];
  res |= buffer[0]<<8;
  return res;
}
