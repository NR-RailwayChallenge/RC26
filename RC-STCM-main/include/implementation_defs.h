// =============================================================================
// Implementation Definitions
// Written by Oliver Flaks and Kiran Chinnam
// 11.06.2024
// =============================================================================
#ifndef IMPLEMENTATION_DEFS_H
#define IMPLEMENTATION_DEFS_H
#include <cstdint>
#include "hh_enumerations.h"
#include "constants.h"
#include "rc_can_defs.h"

enum message_type_indexer{//aliasing the characters within the zrray with variable names
    MTSF_STCM_LINKREPORT,
    MTSF_STCM_TRACTION_STATE,
    MTSF_STCM_AUX_STATE,
    MTSF_STCM_THROTTLE_STATE,
    MTSF_STCM_MESSAGE_PUSH,
    MTSF_ECM_CAP_ENERGY,
    MTSF_ECM_VESC_POWER,
    MTSF_ECM_POWER_SUPPLY
};

const int tx_msg_type_count = 8; //Number of message types we Tx via CAN.

#define STALE false //STALE=messages will be updated when we send messages
#define FRESH true //Don't need to resend, ready to go

struct OutgoingValues{
    bool hh_online;
    bool ecm_online;
    bool lam_online;
    bool rdt_online;
    bool vgm_online;
    
    float speed; //current speed of the loco
    uint8_t speed_lim; //Current speed limiter setting
    float autostop_distance_remaining_m; //Distance left to the end of the autostop challenge
    // TODO: CR0603 RR - This was erroneously a uint16_t instead of a float.
    float autostop_start_dist_m;//for marking where the autostop started
    float trip_dist_m; //Odometer Value
    uint16_t ato_throttle;
    uint16_t out_throttle;
    bool ato_throttle_enabled; //true=auto throttle enabled
    bool driver_throttle_enabled; //true = driver throttle enabled
    bool brake_fault_detect;
    float brake_fault_detect_time;
    bool brake_fault; 
    bool current_direction_is_reverse = false; // TODO: CR0603 RR - Renamed this for clarity.
    bool horn_interlock;//true = Driver must press horn //TODO: CR0603 AR - We don't use this anywhere, delete?
    //If we are deleting horn_interlock, then can_proto needs to be updated - please let me know.
    bool mbr;//true = brake applied
    bool br1;//true = brake applied
    bool br2;
    bool br3;
    bool h_estop;//false=healthy
    bool s_estop;//false=healthy
    bool ksr;
    bool brake_hold;
    DriveModeEnum drive_mode = STANDARD; 
    bool autostop_marker_found;//this lies within autostop message push
    bool fire;//true = fire present
    CouplerStatus coupler_status = UNCOUPLED;
    bool ACR = false;
    bool ACD = false;
    float autocoupler_current_sensor_volts = 0.0f;
    bool autocoupler_couple_detect;
    bool stationaryFlag = true;
    float acceleration_mss;
    //float autoStopKi = 0.0f; // TODO: CR0603 RR - Ki is a constant, so should not go here.
    CAN_STCM_MSG_PUSH_MSG_ID_ENUM msg_push = NULL_MESSAGE;
    //============================================================
    //ECM Outgoing_values parameters
    //============================================================
    // Store watt-hour values so conversion error doesn't accumulate.
    float rel_wh = 0.0f;
    float rel_j = 0.0f;
    float abs_wh = 0.0f;
    float abs_j = 0.0f;
    float rel_j_offset = 0.0f;
    uint16_t cap_mv = 0;
    uint16_t vesc_mv = 0;
    int16_t vesc_current_scaled = 0;
    int16_t vesc_inst_eff = 0;
    uint16_t bus_36 = 0;
    uint16_t bus_24 = 0;
    uint16_t bus_12 = 0;
    bool contactor_3 = false;
    bool contactor_2 = false;
    bool contactor_1 = false;
    bool dc_dc_12_ok = false;
    bool dc_dc_5_ok = false;
    bool rcr_state = false;
};

// TODO: CR0603 RR - The Outgoing Values struct retains the old value, unless you overwrite it. 
// Use the ephemeral values struct for temporary vars you use to calculate other things,
// and only where you won't need to compare against the previous value of that variable.
struct EphemeralValues{
    bool measured_br1 = false;
    bool measured_br2 = false;
    bool measured_br3 = false;
    bool measured_br3_ooc = false; // Variable set to true if the 2 brakes on BR3 are out of correspondance with each other.
    bool autostop_requests_brake_hold = false;
    bool msg_mode_change_failed = false; // Set to true to trigger a message push for failed drive mode change.
    bool retroreflector_seen = false;
    bool msg_regen_protection_exit = false; //Set to true to indicate that the regen protection function revert mode to standard.
    bool msg_regen_finished = false;
    bool msg_autostop_started = false;
    bool msg_smoke = false;
    bool msg_heat = false;
    bool msg_direction_change_blocked = false;
    uint16_t remapped_driver_throttle = NEUTRAL_THROTTLE;
    bool autostop_requests_throttle_control = false;
    bool new_acceleration_value_available = false;
    bool msg_autostop_braking_initiated = false;
    bool e_stop_offline_condition = false;
};


#endif