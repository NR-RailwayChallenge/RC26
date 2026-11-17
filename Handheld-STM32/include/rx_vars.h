#ifndef RX_VARS_H
#define RX_VARS_H

#include <cstdint>

enum coupler_status_t{
    Uncoupling = 0,
    Uncoupled = 1,
    Coupling = 2,
    Coupled = 3
};

enum drive_mode_t{
    Shunting = 0,
    Standard = 1,
    Regen = 2,
    Autostop = 3,
    Neutral = 4
};

struct incoming_vars{
    // LINK REPORT
    bool self_online;
    bool ecm_online;
    bool lam_online;
    bool rdt_online;
    bool vgm_online;
    //Traction State
    float trac_velocity;
    uint8_t speed_limit;
    float autostop_distance_remain;
    float trip_counter;
    //Aux State
    bool brake_fault;
    bool master_brake_relay;
    bool brake_3;
    bool brake_2;
    bool brake_1;
    bool autostop_marker_found;
    bool horn_interlock;
    bool direction_flag;
    bool s_e_stop;
    bool h_e_stop;
    bool fire_alarm;
    bool stationary_lock;
    coupler_status_t coupler_status;
    drive_mode_t drive_mode;
    //STCM Throttle State
    // ATO and Output throttle positions not read.
    bool kill_switch_relay;
    bool auto_throttle_en;
    bool driver_throttle_en;
    //ECM Cap energy
    // Handheld will only read the rel value. Abs to be shown via RDT.
    float super_cap_energy;
    uint16_t cap_volts;
    //ECM Vesc Power
    float trac_current;
    uint16_t bus_vesc;
    float vesc_inst_eff;
    //ECM Power Supply
    uint16_t bus36v;
    uint16_t bus24v;
    uint16_t bus12v;
    bool contactor3;
    bool contactor2;
    bool contactor1;
    bool b12OK;
    bool b5OK;
    //STCM Message Push
    uint8_t msg_push;
};

#endif