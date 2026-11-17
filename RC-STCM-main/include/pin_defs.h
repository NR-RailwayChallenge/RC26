// Pin Definitions file for Nucleo F401RE

#ifndef pindefs_h
#define pindefs_h

// Not ever used, noted here only for reference.
#define PIN_CAN_TD PA12
#define PIN_CAN_RD PA11
#define PIN_CAN_V_TD PB13
#define PIN_CAN_V_RD PB12

#define PIN_CAN_PORT CAN1
#define PIN_CAN_PINS CAN_PINS::DEF
#define PIN_CAN_VESC_PORT CAN2
#define PIN_CAN_VESC_PINS CAN_PINS::DEF

//Pins that need to be assigned:


//Inputs:
//CAN PINS selection:
#define LOCOCAN CAN1
#define VESCCAN CAN2


//Digital
//smoke and fire
#define PIN_SMOKE_DETECT PA8 
#define PIN_HEAT_DETECT PB10
//brake fault sensors
#define PIN_BRAKE_FAULT_1 PB0
#define PIN_BRAKE_FAULT_2 PC1
#define PIN_BRAKE_FAULT_3 PC0
#define PIN_E_STOP_HEALTH PB5
//autocoupler detection
#define PIN_AUTO_COUPLER_DETECT PB2
#define PIN_COUPLING_DIR_DETECT PA7
// #define PIN_DRIVE_DIR_DETECT 

#define PIN_AUTOSTOP PB1

//Outputs:
//Digital:
//Relays:
#define PIN_MBR PA0 //
#define PIN_BR1 PC11
#define PIN_BR2 PD2
#define PIN_BR3 PA1
#define PIN_LR PB15
#define PIN_HR PB14
#define PIN_KSR PC7
#define PIN_ACR PA9//Power for autocoupler
#define PIN_ACDR PB4//Relay controlling direction autocoupler
//Analogue:
#define PIN_THROTTLE_OUT PA6//  this function is not needed for now

//ECM pindefs
//Define your pins like this. Make sure to prefix them with PIN_
#define PIN_BUS_12 PC3
// #define PIN_BUS_12_ALIAS PA3
#define PIN_BUS_24 PC2
// #define PIN_BUS_24_ALIAS PA2
#define PIN_BUS_36 PC4
// #define PIN_BUS_36_ALIAS PA10
#define PIN_BUS_SUPERCAP PA4 
// #define PIN_BUS_SUPERCAP_ALIAS PB3 // All ALIAS pins are the incorrectly wired on the PCB. Define these are high impedence to not affect busbar readings.
#define PIN_C1 PC5
#define PIN_C2 PC6
#define PIN_C3 PC8
#define PIN_RCR PC9
#define PIN_B5_OK PB8
#define PIN_B12_OK PB9

// These pins are to allow for a digital logic analyser to
// time your loop execution times without spamming Serial.Print
#define PIN_PRIMARY_TASK_TRACE PC10
#define PIN_SECONDARY_TASK_TRACE PC12

#define PIN_HB_LED LED_BUILTIN

#endif