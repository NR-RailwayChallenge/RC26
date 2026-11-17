#ifndef IMPLEMENTATION_DEFS_H
#define IMPLEMENTATION_DEFS_H
#include <cstdint>
#include "rx_vars.h"

enum message_type_indexer{
    MTSF_HH_THROTTLE,
    MTSF_HH_BTN_STATE,
};

const int tx_msg_type_count = 2;

#define STALE false
#define FRESH true

struct tracked_outgoing{
    bool change_direction;
    bool horn;
    bool couple;
    uint8_t local_brake_demand;
    drive_mode_t mode_rq;
    uint16_t throttle_pos;
    //None outgoing, tracked only
    bool autostop_btn;
    bool regen_btn;
};

//JS appears centered around 1970 rather than 2047

#define THROTTLE_DEADBAND_LOWER 1800 
#define THROTTLE_DEADBAND_UPPER 2150
#define THROTTLE_NEUTRAL 2047

#define MODE_CHANGE_SHORT_PRESS_MS 250
#define MODE_CHANGE_LONG_PRESS_MS 1000
#define MODE_CHANGE_PRESS_LIMIT_MS 5000
#define MODE_CHANGE_TIMEOUT_MS 500

#endif