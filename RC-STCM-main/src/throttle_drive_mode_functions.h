// V.01 of the throttle code.
// Author: Ross Forbes
// last updated: 17.04.2024:

#ifndef THROTTLE_DRIVEMODE_H
#define THROTTLE_DRIVEMODE_H

#include <Arduino.h>
#include <cstdint>
#include "hh_enumerations.h"
#include "implementation_defs.h"
#include "constants.h"
#include "pin_defs.h"
#include "rx_vars.h"
#include "braking_functions.h"
#include "pi_controller.h"

void remapDriverThrottle(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, EphemeralValues *ephemeralState);
void processDriveModeSwitching(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, IncomingValues *lastReceived, EphemeralValues *ephemeralState);
void calculateVirtualThrottle(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, EphemeralValues *ephemeralState, PiController *controller);
void selectThrottleAndApplySpeedLimit(OutgoingValues *nextSend, EphemeralValues *ephemeralState);
#endif

// the function of this code is to drive the throttle.
// it will look at the throttle value, throttle mode selected
// look at whether the horn and brakes have been pressed to allow movement
// and look at the health of the E-stop.
// the function of it is to calculate the output current demand
// and ultimately send the correct amount of current to the VESC to move the loco.
