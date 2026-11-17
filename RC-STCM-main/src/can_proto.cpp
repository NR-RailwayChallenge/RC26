#include "can_proto.h"

bool compose_and_send(CanTransport *can, Can_Aid aid, OutgoingValues *state)
{
  CAN_message_t frame;
  memset(frame.buf, 0, 8);
  switch (aid)
  {
  case STCM_LINKREPORT:
  {
    frame.id = CAN_STCM_LINKREPORT;
    frame.len = CAN_DLC_STCM_LINKREPORT;
    frame.buf[CAN_STCM_LINKREPORT_HH_BYTE] |= ((state->hh_online) & CAN_STCM_LINKREPORT_HH_MASK) << CAN_STCM_LINKREPORT_HH_SHIFT;
    frame.buf[CAN_STCM_LINKREPORT_ECM_BYTE] |= ((true) & CAN_STCM_LINKREPORT_ECM_MASK) << CAN_STCM_LINKREPORT_ECM_SHIFT; //ecm online always true because ECM no longer exists.
    frame.buf[CAN_STCM_LINKREPORT_LAM_BYTE] |= ((state->lam_online) & CAN_STCM_LINKREPORT_LAM_MASK) << CAN_STCM_LINKREPORT_LAM_SHIFT;
    frame.buf[CAN_STCM_LINKREPORT_RDT_BYTE] |= ((state->rdt_online) & CAN_STCM_LINKREPORT_RDT_MASK) << CAN_STCM_LINKREPORT_RDT_SHIFT;
    frame.buf[CAN_STCM_LINKREPORT_VGM_BYTE] |= ((state->vgm_online) & CAN_STCM_LINKREPORT_VGM_MASK) << CAN_STCM_LINKREPORT_VGM_SHIFT;
  }
  break;
  case STCM_TRACTION_STATE:
  {
    frame.id = CAN_STCM_TRACTION_STATE;
    frame.len = CAN_DLC_STCM_TRACTION_STATE;
    CAN_STCM_TRACTION_STATE_SPEED_TYPE tempSpeed = round(state->speed * 10.0f);
    CAN_STCM_TRACTION_STATE_AUTOSTOP_DIST_TYPE tempAutostopDist = round(state->autostop_distance_remaining_m * 100.0f);
    if (state->drive_mode != DriveModeEnum::AUTOSTOP)
    {
      tempAutostopDist = -30001;
    }
    else if (!state->autostop_marker_found)
    {
      tempAutostopDist = 30001;
    }
    CAN_STCM_TRACTION_STATE_AUTOSTOP_DIST_TYPE tempTripDist = round(state->trip_dist_m * 100.0f);
    memcpy(&frame.buf[CAN_STCM_TRACTION_STATE_SPEED_OFFSET], &tempSpeed, sizeof(tempSpeed));
    memcpy(&frame.buf[CAN_STCM_TRACTION_STATE_SPEED_LIM_OFFSET], &(state->speed_lim), sizeof(state->speed_lim));
    memcpy(&frame.buf[CAN_STCM_TRACTION_STATE_AUTOSTOP_DIST_OFFSET], &tempAutostopDist, sizeof(tempAutostopDist));
    memcpy(&frame.buf[CAN_STCM_TRACTION_STATE_TRIP_DIST_OFFSET], &tempTripDist, sizeof(tempTripDist));
  }
  break;
  case STCM_AUX_STATE:
  {
    frame.id = CAN_STCM_AUX_STATE;
    frame.len = CAN_DLC_STCM_AUX_STATE;
    frame.buf[CAN_STCM_AUX_STATE_BF_BYTE] |= ((state->brake_fault) & CAN_STCM_AUX_STATE_BF_MASK) << CAN_STCM_AUX_STATE_BF_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_MBR_BYTE] |= ((state->mbr) & CAN_STCM_AUX_STATE_MBR_MASK) << CAN_STCM_AUX_STATE_MBR_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_BR3_BYTE] |= ((state->br3) & CAN_STCM_AUX_STATE_BR3_MASK) << CAN_STCM_AUX_STATE_BR3_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_BR2_BYTE] |= ((state->br2) & CAN_STCM_AUX_STATE_BR2_MASK) << CAN_STCM_AUX_STATE_BR2_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_BR1_BYTE] |= ((state->br1) & CAN_STCM_AUX_STATE_BR1_MASK) << CAN_STCM_AUX_STATE_BR1_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_AS_MD_BYTE] |= ((state->autostop_marker_found) & CAN_STCM_AUX_STATE_AS_MD_MASK) << CAN_STCM_AUX_STATE_AS_MD_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_HL_BYTE] |= ((state->horn_interlock) & CAN_STCM_AUX_STATE_HL_MASK) << CAN_STCM_AUX_STATE_HL_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_DIR_BYTE] |= ((state->current_direction_is_reverse) & CAN_STCM_AUX_STATE_DIR_MASK) << CAN_STCM_AUX_STATE_DIR_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_SESTOP_BYTE] |= ((state->s_estop) & CAN_STCM_AUX_STATE_SESTOP_MASK) << CAN_STCM_AUX_STATE_SESTOP_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_HESTOP_BYTE] |= ((state->h_estop) & CAN_STCM_AUX_STATE_HESTOP_MASK) << CAN_STCM_AUX_STATE_HESTOP_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_FIRE_BYTE] |= ((state->fire) & CAN_STCM_AUX_STATE_FIRE_MASK) << CAN_STCM_AUX_STATE_FIRE_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_BH_BYTE] |= ((state->brake_hold) & CAN_STCM_AUX_STATE_BH_MASK) << CAN_STCM_AUX_STATE_BH_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_COUPLER_BYTE] |= ((state->coupler_status) & CAN_STCM_AUX_STATE_COUPLER_MASK) << CAN_STCM_AUX_STATE_COUPLER_SHIFT;
    frame.buf[CAN_STCM_AUX_STATE_DRIVE_MODE_BYTE] |= ((state->drive_mode) & CAN_STCM_AUX_STATE_DRIVE_MODE_MASK) << CAN_STCM_AUX_STATE_DRIVE_MODE_SHIFT;
  }
  break;
  case STCM_THROTTLE_STAT:
  {
    frame.id = CAN_STCM_THROTTLE_STAT;
    frame.len = CAN_DLC_STCM_THROTTLE_STAT;
    memcpy(&frame.buf[CAN_STCM_THROTTLE_STAT_ATO_THROTTLE_OFFSET], &(state->ato_throttle), sizeof(state->ato_throttle));
    memcpy(&frame.buf[CAN_STCM_THROTTLE_STAT_OUT_THROTTLE_OFFSET], &(state->out_throttle), sizeof(state->out_throttle));
    frame.buf[CAN_STCM_THROTTLE_STAT_KSR_BYTE] |= ((state->ksr) & CAN_STCM_THROTTLE_STAT_KSR_MASK) << CAN_STCM_THROTTLE_STAT_KSR_SHIFT;
    frame.buf[CAN_STCM_THROTTLE_STAT_ATE_BYTE] |= ((state->ato_throttle_enabled) & CAN_STCM_THROTTLE_STAT_ATE_MASK) << CAN_STCM_THROTTLE_STAT_ATE_SHIFT;
    frame.buf[CAN_STCM_THROTTLE_STAT_DTE_BYTE] |= ((state->driver_throttle_enabled) & CAN_STCM_THROTTLE_STAT_DTE_MASK) << CAN_STCM_THROTTLE_STAT_DTE_SHIFT;
  }
  break;
  case STCM_MSG_PUSH:
  {
    frame.id = CAN_STCM_MSG_PUSH;
    frame.len = CAN_DLC_STCM_MSG_PUSH;
    frame.buf[CAN_STCM_MSG_PUSH_MSG_ID_OFFSET] = state->msg_push;
  }
  break;

  //ECM Messages
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

  return can->transmit(&frame);
}

void parse_frame(CAN_message_t *frame, IncomingValues *state)
{
  // Mask the aid with 7ff to ignore anything past the 11 lsb.
  // RTR and other similar flags set bits in the can_id field.
  Can_Aid incomingAid = (Can_Aid)(frame->id & 0x7ff);
  Serial.printf("fpsr 0x%03x\n", incomingAid);
  switch (incomingAid)
  {
  case HH_THROTTLE:
  {
    memcpy(&(state->driver_throttle_position), &(frame->buf[CAN_HH_THROTTLE_THROTTLE_OFFSET]), sizeof(state->driver_throttle_position));
  }
  break;
  case HH_BTN_STATE:
  {
    state->mode_req = (DriveModeEnum)((frame->buf[CAN_HH_BTN_STATE_MODE_RQ_BYTE] >> CAN_HH_BTN_STATE_MODE_RQ_SHIFT) & CAN_HH_BTN_STATE_MODE_RQ_MASK);
    state->brake_demand = (BrakeDemand)((frame->buf[CAN_HH_BTN_STATE_BRAKE_BYTE] >> CAN_HH_BTN_STATE_BRAKE_SHIFT) & CAN_HH_BTN_STATE_BRAKE_MASK);
    state->auto_coupler_btn = (frame->buf[CAN_HH_BTN_STATE_COUPLE_BYTE] >> CAN_HH_BTN_STATE_COUPLE_SHIFT) & CAN_HH_BTN_STATE_COUPLE_MASK;
    state->horn_btn = (frame->buf[CAN_HH_BTN_STATE_HORN_BYTE] >> CAN_HH_BTN_STATE_HORN_SHIFT) & CAN_HH_BTN_STATE_HORN_MASK;
    state->direction_btn = (frame->buf[CAN_HH_BTN_STATE_DIR_BYTE] >> CAN_HH_BTN_STATE_DIR_SHIFT) & CAN_HH_BTN_STATE_DIR_MASK;
    Serial.print("Direction Btn: ");
    Serial.println(state->direction_btn);
  }
  break;
  case ECM_POWER_SUPPLY:
  {
    state->contactor1Status = (frame->buf[CAN_ECM_POWER_SUPPLY_C1_S_BYTE] >> CAN_ECM_POWER_SUPPLY_C1_S_SHIFT) & CAN_ECM_POWER_SUPPLY_C1_S_MASK;
  }
  case RDT_GNSS:
  {
    // The STCM doesn't care about the payload, the STCM is only interested
    // in this message so it knows that the RDT is online on the CAN bus.
    // Therefore, don't extract any contents from this message.
  }
  break;
  case LAM_ANNOUNCE:
  {
    // The STCM doesn't care about the payload, the STCM is only interested
    // in this message so it knows that the LAM is online on the CAN bus.
    // Therefore, don't extract any contents from this message.
  }
  break;
  // case STCM_THROTTLE_INPUT: //NEW!!!
  // {
  //   //We dont need to parse this as the STCM shouldn't be receiving this message. The STCM parses this throttle input message using information in the handheld.
  //   //Therefore, don't do anything if we somehow get this message.
  // }
  // break;

  default:
    break;
  }
}
