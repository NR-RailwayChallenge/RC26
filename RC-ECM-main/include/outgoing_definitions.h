#ifndef OUTGOING_DEFINITIONS_H
#define OUTGOING_DEFINITIONS_H
#include <cstdint>

// This is part of the STALE/FRESH SYSTEM.
// Replace these with one entry for each message type your module will SEND.
enum message_type_indexer{
    MTSF_ECM_CAP_ENERGY,
    MTSF_ECM_VESC_POWER,
    MTSF_ECM_POWER_SUPPLY
};

// Set this to the number of entries you have above.
const int tx_msg_type_count = 3;

// DO NOT edit these
#define STALE false
#define FRESH true

// Replace these 'variables' with everything you expect to send on to the CAN bus.
// This is the example for the handheld. If you're unsure about what datatype to use,
// refer to the CAN protocol spec, or ask the Electrical Team Lead.
struct outgoing_values{
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

#endif