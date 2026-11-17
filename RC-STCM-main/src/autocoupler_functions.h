#ifndef AUTOCOUPLER_H
#define AUTOCOUPLER_H

#include "Arduino.h"
#include "implementation_defs.h"
#include "rx_vars.h"
#include "pin_defs.h"
#include <cstdint>

void processAutocouplerStates(OutgoingValues* nextSend, OutgoingValues* lastSent, IncomingValues* nextRecieve, IncomingValues* lastRecieved);

#endif