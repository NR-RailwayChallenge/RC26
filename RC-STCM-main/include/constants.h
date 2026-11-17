#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BRAKE_RELOCK_TIMER_MS 1000000
#define BRAKE_FAULT_THRESHOLD 2.55f
#define BRAKE_FAULT_THRESHOLD_HI 2.65f
#define STATIONARY_THRESHOLD_KMH 0.05f
#define SHUNTLIMIT_KMH 3.00f
#define STANDARDLIMIT_KMH 15.00f
#define AUTOSTOPLIMIT_KMH 11.00f
#define OVERSPEED_BRAKE_FACTOR 3.0f
#define COOP_OS_MAINTASK_PERIOD 10000
#define KEEPALIVE_MS 100
#define TX_KEEPALIVE_MS 50
#define CAN_BAUDRATE 500000
#define VESC_COUNT 4
#define WHEEL_DIAMETER_m 0.2f
#define TACHO_RATIO 6 //2 per pole pair, of 3 pulses
#define WHEEL_RADIUS_M 0.1f//TODO:Tidy and check these constants
#define DRIVE_REDUCTION_GEAR_RATIO 7
#define MOTOR_POLE_PAIRS 4
#define AUTOSTOP_KP 0.1f
#define AUTOSTOP_KI 1.0f
#define BRAKE_BASELINE 1500
#define AUTOSTOP_DISTANCE_m 25.0f
#define CAN_MASK 0x08
#define CAN_FILTER 0x00
#define AUTOSTOP_DIVIDER 25 //Controls the frequency at which autostop control signals are evaluated and acceleration is calculated.
#define AUTOSTOP_SERVICE_BRAKE_DISTANCE 0.1f
#define ADC_MAX_FLOAT 4095.0f
#define ADC_SYS_VOLTS 3.3f
#define BRAKE_HOLD_LEVEL BrakeDemand::LVL_3
#define NEUTRAL_THROTTLE 2047
#define SHUNTMODE_THROTTLE_DIV 4
#define REGEN_TRACTIVE_MIN_ENERGY_JOULES 100 // TODO: Review this threshold.
//#define BRAKE_RELEASE_INHIBIT_BY_AUTOCOUPLER 1 // Comment this to disable autocoupler checks during brake release
#define BRAKE_FAULT_TIMER_MS 500
#define AUTOCOUPLER_RETRACT_VOLTS_THRESHOLD 2.45f // TODO: Check that retract is less than extend. If not, signs need to be changed in autocoupler_functions.cpp
#define AUTOCOUPLER_EXTEND_VOLTS_THRESHOLD 2.55f // TODO: Tune these parameters.

//ECM constants
#define DISABLE_SOFTWARE_SID_FILTER false
#define VESC_TIMEOUT_MS 1000
#define CAPACITANCE_F 166.0F

#define MEASURE_BY_VOLTS 1 //If defined, calculate energy using 1/2 cv^2 else if undefined use watt hours

//TODO: Set to 5000 to enable the reset protection feature.
#define RESET_PROTECTION_MS 0
#define LIMPERCENT 0.8 //percent of speed limit to start limiting throttle output. Input range 0-1. Default is now 80%
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

#define R1_12V 110
#define R2_12V 1000
#define R1_24V 68
#define R2_24V 1000
#define R1_36V 60
#define R2_36V 1500
#define R1_SUPERCAP 60
#define R2_SUPERCAP 1500

#define WHEEL_RADIUS_M 0.1f
#define DRIVE_REDUCTION_GEAR_RATIO 7
#define MOTOR_POLE_PAIRS 4
#endif
