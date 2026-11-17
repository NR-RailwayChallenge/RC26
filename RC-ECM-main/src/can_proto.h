#ifndef CAN_PROTO_H
#define CAN_PROTO_H

#include "can_transport.h"
#include "incoming_struct.h"
#include "outgoing_definitions.h"

//See the CPP file for comments on these functions.
bool compose_and_send(CanTransport *can, Can_Aid aid, outgoing_values *state);
void parse_frame(CAN_message_t *frame, incoming_values *state);

#endif