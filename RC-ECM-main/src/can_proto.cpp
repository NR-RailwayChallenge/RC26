#include "can_proto.h"

// This function is responsible for forming and sending your CAN messages.
bool compose_and_send(CanTransport *can, Can_Aid aid, outgoing_values *state)
{
  // Create a instance of message to prep for sending
  CAN_message_t frame;
  // This line forces the data buffer to all zeros to make sure it is properly initialised.
  memset(frame.buf, 0, 8);
  switch (aid)
  {
  case ECM_CAP_ENERGY:
  {
    frame.id = CAN_ECM_CAP_ENERGY;
    frame.len = CAN_DLC_ECM_CAP_ENERGY;
#ifdef MEASURE_BY_VOLTS
    CAN_ECM_CAP_ENERGY_ENERGY_REL_TYPE tempRel = state->rel_j / 10;
    CAN_ECM_CAP_ENERGY_ENERGY_ABS_TYPE tempAbs = state->abs_j / 10;
#else
    CAN_ECM_CAP_ENERGY_ENERGY_REL_TYPE tempRel = state->rel_wh * 3600 / 10;
    CAN_ECM_CAP_ENERGY_ENERGY_ABS_TYPE tempAbs = state->abs_wh * 3600 / 10;
#endif
    memcpy(&frame.buf[CAN_ECM_CAP_ENERGY_ENERGY_REL_OFFSET], &tempRel, sizeof(tempRel));
    memcpy(&frame.buf[CAN_ECM_CAP_ENERGY_ENERGY_ABS_OFFSET], &tempAbs, sizeof(tempAbs));
    memcpy(&frame.buf[CAN_ECM_CAP_ENERGY_CAP_VOLTAGE_OFFSET], &(state->cap_mv), sizeof(state->cap_mv));
  }
  break;
  case ECM_POWER_SUPPLY:
  {
    frame.id = CAN_ECM_POWER_SUPPLY;
    frame.len = CAN_DLC_ECM_POWER_SUPPLY;
    memcpy(&frame.buf[CAN_ECM_POWER_SUPPLY_36V_BUS_OFFSET], &(state->bus_36), sizeof(state->bus_36));
    memcpy(&frame.buf[CAN_ECM_POWER_SUPPLY_24V_BUS_OFFSET], &(state->bus_24), sizeof(state->bus_24));
    memcpy(&frame.buf[CAN_ECM_POWER_SUPPLY_12V_BUS_OFFSET], &(state->bus_12), sizeof(state->bus_12));
    frame.buf[CAN_ECM_POWER_SUPPLY_C3_S_BYTE] |= ((state->contactor_3) & CAN_ECM_POWER_SUPPLY_C3_S_MASK) << CAN_ECM_POWER_SUPPLY_C3_S_SHIFT;
    frame.buf[CAN_ECM_POWER_SUPPLY_C2_S_BYTE] |= ((state->contactor_2) & CAN_ECM_POWER_SUPPLY_C2_S_MASK) << CAN_ECM_POWER_SUPPLY_C2_S_SHIFT;
    frame.buf[CAN_ECM_POWER_SUPPLY_C1_S_BYTE] |= ((state->contactor_1) & CAN_ECM_POWER_SUPPLY_C1_S_MASK) << CAN_ECM_POWER_SUPPLY_C1_S_SHIFT;
    frame.buf[CAN_ECM_POWER_SUPPLY_B12_OK_BYTE] |= ((state->dc_dc_12_ok) & CAN_ECM_POWER_SUPPLY_B12_OK_MASK) << CAN_ECM_POWER_SUPPLY_B12_OK_SHIFT;
    frame.buf[CAN_ECM_POWER_SUPPLY_B5_OK_BYTE] |= ((state->dc_dc_5_ok) & CAN_ECM_POWER_SUPPLY_B5_OK_MASK) << CAN_ECM_POWER_SUPPLY_B5_OK_SHIFT;
  }
  break;
  case ECM_VESC_POWER:
  {
    frame.id = CAN_ECM_VESC_POWER;
    frame.len = CAN_DLC_ECM_VESC_POWER;
    memcpy(&frame.buf[CAN_ECM_VESC_POWER_VESC_VOLTAGE_OFFSET], &(state->vesc_mv), sizeof(state->vesc_mv));
    memcpy(&frame.buf[CAN_ECM_VESC_POWER_VESC_CURRENT_OFFSET], &(state->vesc_current_scaled), sizeof(state->vesc_current_scaled));
    memcpy(&frame.buf[CAN_ECM_VESC_POWER_VESC_INST_EFF_OFFSET], &(state->vesc_inst_eff), sizeof(state->vesc_inst_eff));
  }
  break;
  default:
    return false;
  }
  // Finally transmit the message, and return a true value if the message was queued or sent, and false otherwise.
  return can->transmit(&frame);
}

// This function is responsible for reading CAN messages and intepreting them.
void parse_frame(CAN_message_t *frame, incoming_values *state)
{
  // Mask the aid with 7ff to ignore anything past the 11 lsb.
  // RTR and other similar flags set bits in the can_id field.
  Can_Aid incomingAid = (Can_Aid)(frame->id & 0x7ff);

  // This serial print statement is useful during debugging. Uncomment it if required.
  // Serial.printf("fpsr 0x%03x\n",incomingAid);
  switch (incomingAid)
  {
    // Add a case statement for each message type you expect to receive. Each Case starts with the word case and goes upto and including break;
  case STCM_LINKREPORT:
    state->self_online = (frame->buf[CAN_STCM_LINKREPORT_ECM_BYTE] >> CAN_STCM_LINKREPORT_ECM_SHIFT) & CAN_STCM_LINKREPORT_ECM_MASK;
    state->handheld_online = (frame->buf[CAN_STCM_LINKREPORT_HH_BYTE] >> CAN_STCM_LINKREPORT_HH_SHIFT) & CAN_STCM_LINKREPORT_HH_MASK;
    break;
  case STCM_AUX_STATE:
    state->drive_mode = (drive_mode_t)((frame->buf[CAN_STCM_AUX_STATE_DRIVE_MODE_BYTE] >> CAN_STCM_AUX_STATE_DRIVE_MODE_SHIFT) & CAN_STCM_AUX_STATE_DRIVE_MODE_MASK);
    break;
  default:
    break;
  }
}
