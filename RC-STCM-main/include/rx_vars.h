#ifndef RX_VARS_H
#define RX_VARS_H

#include <cstdint>
#include "hh_enumerations.h"

struct IncomingValues{

    BrakeDemand brake_demand;
    bool horn_btn;
    bool direction_btn; //Request to toggle direction
    bool auto_coupler_btn; //State of Autocoupler Button
    DriveModeEnum mode_req; //Mode requested by the handheld
    uint16_t driver_throttle_position;
    bool contactor1Status; 
    // The STCM should use this to deny positive tractive effort if the regen energy is below a
    // a threshold and we're in regen mode. Braking throttle commands should always be allowed.
    // Allowing braking commands means even though we momentarily fail this check when we enter
    // regen mode, allowing braking allows regen and this value to rise, unlocking +ve 
    // acceleration, as we start regen mode whilst we're on the move.
    float cap_energy_rel_joules; 

    //bool VESCSTM32_Online;

//============================================================================
// ECM Incoming Values parameters
//============================================================================
    bool handheld_online;
    DriveModeEnum drive_mode;


};


#endif