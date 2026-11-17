#ifndef GPIO_FUNC_H
#define GPIO_FUNC_H

#include <Arduino.h>
#include "pin_defs.h"
#include "constants.h"
#include "implementation_defs.h"
#include "rx_vars.h"

// =============================================================================
// Pulls input data from relevant input pin, doesn't pull CAN data
// =============================================================================
void readGpioInputs(OutgoingValues* nextSend, OutgoingValues* lastSent, EphemeralValues* ephemeralState);

// =============================================================================
// Writes pins based on the outgoing values states, doesn't do control.
// =============================================================================
void writeGpioOutputs(OutgoingValues* nextSend, IncomingValues* currentReceive);


#endif