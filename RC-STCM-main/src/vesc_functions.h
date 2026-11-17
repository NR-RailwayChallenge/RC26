#ifndef VESC_FUNC_H
#define VESC_FUNC_H

#include <cstdint>
#include "STM32_CAN.h"
#include "constants.h"
#include "timing.h"
#include "vesc_proto_defs.h"

int32_t decodeBigEndianInt32(uint8_t *buffer);
int16_t decodeBigEndianInt16(uint8_t *buffer);
void receive_and_parse_vesc(STM32_CAN* canIfV, VescStatus* currentVescState, VescStatus* previousVescState);

#endif