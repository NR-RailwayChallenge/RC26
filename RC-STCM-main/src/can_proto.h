#ifndef CAN_PROTO_H
#define CAN_PROTO_H

#include "can_transport.h"
#include "rx_vars.h"
#include "implementation_defs.h"

bool compose_and_send(CanTransport *can, Can_Aid aid, OutgoingValues *state);
void parse_frame(CAN_message_t *frame, IncomingValues *state);

#endif