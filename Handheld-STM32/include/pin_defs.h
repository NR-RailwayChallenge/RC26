// Pin Definitions file for Nucleo F401RE

#ifndef pindefs_h
#define pindefs_h

#define PIN_BRAKE PC2
#define PIN_CHANGE_DIRECTION PA4
#define PIN_COUPLE_BTN PC0
#define PIN_HORN PA1
#define PIN_AUTOSTOP PB0
#define PIN_REGEN PC1
#define PIN_DSD_DETECT PA0
#define PIN_JS_SIG PC3

// Not ever used, noted here only for reference.
#define PIN_CAN_TD PA12
#define PIN_CAN_RD PA11

#define PIN_CAN_PORT CAN1
#define PIN_CAN_PINS CAN_PINS::DEF

#define PIN_PRIMARY_TASK_TRACE PB4
#define PIN_SECONDARY_TASK_TRACE PB5

// Tx on the Nucleo, which connects to Rx on the Pico
#define PIN_PICO_TX PC10
// Rx on the Nucleo, which connects to Tx on the Pico
#define PIN_PICO_RX PC11

#define PIN_HB_LED PA5

#endif