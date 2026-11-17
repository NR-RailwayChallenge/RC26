// Pin Definitions file for Nucleo F401RE

#ifndef pindefs_h
#define pindefs_h

#include "STM32_CAN.h"

//Define your pins like this. Make sure to prefix them with PIN_
#define PIN_BUS_12 PA0
#define PIN_BUS_24 PA1
#define PIN_BUS_36 PB0
#define PIN_BUS_SUPERCAP PC1
#define PIN_C1 PC5
#define PIN_C2 PC6
#define PIN_C3 PC8
#define PIN_RCR PC9
#define PIN_B5_OK PB8
#define PIN_B12_OK PB9
 
// Not ever used, noted here only for reference.
#define PIN_CAN_TD PA12
#define PIN_CAN_RD PA11
#define PIN_CAN_V_TD PB13
#define PIN_CAN_V_RD PB12

#define PIN_CAN_PORT CAN1
#define PIN_CAN_PINS DEF

#define PIN_CAN_VESC_PORT CAN2
#define PIN_CAN_VESC_PINS DEF

// These pins are to allow for a digital logic analyser to
// time your loop execution times without spamming Serial.Print
#define PIN_PRIMARY_TASK_TRACE PB4
#define PIN_SECONDARY_TASK_TRACE PB5

#define PIN_HEARTBEAT_LED LED_BUILTIN

#endif
