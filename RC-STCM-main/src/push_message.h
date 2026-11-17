#ifndef PUSH_MSG_H
#define PUSH_MSG_H

#include "implementation_defs.h"
#include "rc_can_defs.h"
#include <Arduino.h>

int getMessageTimeoutMs(CAN_STCM_MSG_PUSH_MSG_ID_ENUM message);
void setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM message, OutgoingValues *nextSend);
void evaluatePushMessages(OutgoingValues *nextSend, EphemeralValues *ephemeralState);

#endif