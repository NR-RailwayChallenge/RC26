#include "vesc_functions.h"

void receive_and_parse_vesc(STM32_CAN* canIfV, VescStatus* currentVescState, VescStatus* previousVescState)
{
  CAN_message_t rxMsg;
  if (canIfV->read(rxMsg))
  {
    // Received a message from a VESC
    // Extract the VESC ID and see if it is from a VESC that we know
    int vescIndex = (rxMsg.id >> 4) - 1;
    // Discard the packet if it doesn't match a pre-defined index.
    if (vescIndex >= VESC_COUNT)
      return;
    switch (rxMsg.id & 0x0F)
    {
    case 1:
      currentVescState[vescIndex].rpm = decodeBigEndianInt32(rxMsg.buf);
      currentVescState[vescIndex].current = decodeBigEndianInt16(&rxMsg.buf[4]) / 10.0f;
      currentVescState[vescIndex].dutyCycle = decodeBigEndianInt16(&rxMsg.buf[6]) / 1000.0f;
      break;
    case 2:
      currentVescState[vescIndex].ahConsumed = decodeBigEndianInt32(rxMsg.buf) / 10000.0f;
      currentVescState[vescIndex].ahCharged = decodeBigEndianInt32(&rxMsg.buf[4]) / 10000.0f;
      break;
    case 3:
      currentVescState[vescIndex].whConsumed = decodeBigEndianInt32(rxMsg.buf) / 10000.0f;
      currentVescState[vescIndex].whCharged = decodeBigEndianInt32(&rxMsg.buf[4]) / 10000.0f;
      break;
    case 4:
      currentVescState[vescIndex].mosfetTemp = decodeBigEndianInt16(rxMsg.buf) / 10.0f;
      currentVescState[vescIndex].motorTemp = decodeBigEndianInt16(&rxMsg.buf[2]) / 10.0f;
      currentVescState[vescIndex].inputCurrent = decodeBigEndianInt16(&rxMsg.buf[4]) / 10.0f;
      currentVescState[vescIndex].pidPosition = decodeBigEndianInt16(&rxMsg.buf[6]) / 50.0f;
      break;
    case 5:
      currentVescState[vescIndex].tachoRel = decodeBigEndianInt32(rxMsg.buf);
      currentVescState[vescIndex].voltageIn = decodeBigEndianInt16(&rxMsg.buf[4]) / 10.0f;
      break;
    case 6:
      currentVescState[vescIndex].adc1millivolts = decodeBigEndianInt16(rxMsg.buf);
      currentVescState[vescIndex].adc2millivolts = decodeBigEndianInt16(&rxMsg.buf[2]);
      currentVescState[vescIndex].adc3millivolts = decodeBigEndianInt16(&rxMsg.buf[4]);
      currentVescState[vescIndex].servoPosition = decodeBigEndianInt16(&rxMsg.buf[6]);
      break;
    default:
      // Unrecognised message somehow got through the filters - ignore it.
      return;
    }
    currentVescState[vescIndex].seenBefore = true;
    currentVescState[vescIndex].lastContactMillis = ms();
  }
}

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
