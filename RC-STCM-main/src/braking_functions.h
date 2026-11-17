//Header File to define the EStopFunction
//By Oliver Flaks
//28/03/2024

#ifndef BRAKING_FUNCTIONS_H
#define BRAKING_FUNCTIONS_H
#include "Arduino.h"
#include "implementation_defs.h"
#include "rx_vars.h"
#include "pin_defs.h"

//The Function of this code will be to check the status of the EStop Loop, 
//and complete software disconnections

extern unsigned long hornInterlockTimer;

void setBrakeRelays(BrakeDemand BrakeDemand, OutgoingValues* nextSend);
void evaluateBrakesAndEmergencyStop(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, IncomingValues *lastReceived, EphemeralValues *ephemeralState);

#endif

