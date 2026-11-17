#ifndef constants_h
#define constants_h

// You should not need to modify these. We don't expect 
// these to change, but putting these value here alleviates
// magic numbers in your code.

#define COOP_OS_MAINTASK_PERIOD 10000
#define DISABLE_SOFTWARE_SID_FILTER false
#define CAN_BAUDRATE 500000
#define CAN_MASK 0x02
#define CAN_FILTER 0x00
#define KEEPALIVE_MS 100
#define TX_KEEPALIVE_MS 50
#define VESC_COUNT 4
#define VESC_TIMEOUT_MS 1000
#define CAPACITANCE_F 166.0F

#define MEASURE_BY_VOLTS 1

//TODO: Set to 5000 to enable the reset protection feature.
#define RESET_PROTECTION_MS 0

//Resistor divider ratios
/*
 ------------- BUS BAR
    |
    ▯ R2
    |
    ---------- To Nucleo
    |
    ▯ R1 (aka Sampling Resistor)
    |
-------------- GND
*/

#define R1_12V 1800
#define R2_12V 10000
#define R1_24V 1000
#define R2_24V 10000
#define R1_36V 1000
#define R2_36V 15000
#define R1_SUPERCAP 1000
#define R2_SUPERCAP 15000

#define WHEEL_RADIUS_M 0.1f
#define DRIVE_REDUCTION_GEAR_RATIO 7
#define MOTOR_POLE_PAIRS 4

#endif
