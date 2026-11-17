#include "can_proto.h"
#include <iostream>

bool compose_and_send(CanTransport *can, Can_Aid aid, tracked_outgoing *state)
{
  CAN_message_t frame;
  memset(frame.buf,0,8);
  switch (aid)
  {
  case HH_BTN_STATE:
  {
    frame.id = CAN_HH_BTN_STATE;
    frame.len = CAN_DLC_HH_BTN_STATE;
    frame.buf[CAN_HH_BTN_STATE_MODE_RQ_BYTE] |= ((state->mode_rq) & CAN_HH_BTN_STATE_MODE_RQ_MASK) << CAN_HH_BTN_STATE_MODE_RQ_SHIFT;
    frame.buf[CAN_HH_BTN_STATE_BRAKE_BYTE] |= ((state->local_brake_demand) & CAN_HH_BTN_STATE_BRAKE_MASK) << CAN_HH_BTN_STATE_BRAKE_SHIFT;
    frame.buf[CAN_HH_BTN_STATE_COUPLE_BYTE] |= ((state->couple) & CAN_HH_BTN_STATE_COUPLE_MASK) << CAN_HH_BTN_STATE_COUPLE_SHIFT;
    frame.buf[CAN_HH_BTN_STATE_HORN_BYTE] |= ((state->horn) & CAN_HH_BTN_STATE_HORN_MASK) << CAN_HH_BTN_STATE_HORN_SHIFT;
    frame.buf[CAN_HH_BTN_STATE_DIR_BYTE] |= ((state->change_direction) & CAN_HH_BTN_STATE_DIR_MASK) << CAN_HH_BTN_STATE_DIR_SHIFT;
  }
  break;
  case HH_THROTTLE:
  {
    frame.id = CAN_HH_THROTTLE;
    frame.len = CAN_DLC_HH_THROTTLE;
    CAN_HH_THROTTLE_THROTTLE_TYPE throttle = state->throttle_pos;
    memcpy(&frame.buf[CAN_HH_THROTTLE_THROTTLE_OFFSET], &throttle, sizeof(throttle));
  }
  break;
  default:
    return false;
  }

  return can->transmit(&frame);
}

void parse_frame(CAN_message_t *frame, incoming_vars *state)
{
  // Mask the aid with 7ff to ignore anything past the 11 lsb.
  // RTR and other similar flags set bits in the can_id field.
  Can_Aid incomingAid = (Can_Aid)(frame->id & 0x7ff);
  //Serial.printf("fpsr 0x%03x\n",incomingAid);
  switch (incomingAid)
  {
  case STCM_LINKREPORT:
    state->self_online = (frame->buf[CAN_STCM_LINKREPORT_HH_BYTE] >> CAN_STCM_LINKREPORT_HH_SHIFT) & CAN_STCM_LINKREPORT_HH_MASK;
    state->ecm_online = (frame->buf[CAN_STCM_LINKREPORT_ECM_BYTE] >> CAN_STCM_LINKREPORT_ECM_SHIFT) & CAN_STCM_LINKREPORT_ECM_MASK;
    state->lam_online = (frame->buf[CAN_STCM_LINKREPORT_LAM_BYTE] >> CAN_STCM_LINKREPORT_LAM_SHIFT) & CAN_STCM_LINKREPORT_LAM_MASK;
    state->rdt_online = (frame->buf[CAN_STCM_LINKREPORT_RDT_BYTE] >> CAN_STCM_LINKREPORT_RDT_SHIFT) & CAN_STCM_LINKREPORT_RDT_MASK;
    state->vgm_online = (frame->buf[CAN_STCM_LINKREPORT_VGM_BYTE] >> CAN_STCM_LINKREPORT_VGM_SHIFT) & CAN_STCM_LINKREPORT_VGM_MASK;
    break;
  case STCM_TRACTION_STATE:
  {
    state->trac_velocity = (frame->buf[CAN_STCM_TRACTION_STATE_SPEED_OFFSET])*0.1f;
    state->speed_limit = (frame->buf[CAN_STCM_TRACTION_STATE_SPEED_LIM_OFFSET]);
    state->autostop_distance_remain = (*((CAN_STCM_TRACTION_STATE_AUTOSTOP_DIST_TYPE*)&frame->buf[CAN_STCM_TRACTION_STATE_AUTOSTOP_DIST_OFFSET]))*0.01f;
    state->trip_counter = (*((CAN_STCM_TRACTION_STATE_TRIP_DIST_TYPE*)&frame->buf[CAN_STCM_TRACTION_STATE_TRIP_DIST_OFFSET]))*0.01f;
  }
  break;
  case STCM_AUX_STATE:
    state->brake_fault = (frame->buf[CAN_STCM_AUX_STATE_BF_BYTE] >> CAN_STCM_AUX_STATE_BF_SHIFT) & CAN_STCM_AUX_STATE_BF_MASK;
    state->brake_1 = (frame->buf[CAN_STCM_AUX_STATE_BR1_BYTE] >> CAN_STCM_AUX_STATE_BR1_SHIFT) & CAN_STCM_AUX_STATE_BR1_MASK;
    state->brake_2 = (frame->buf[CAN_STCM_AUX_STATE_BR2_BYTE] >> CAN_STCM_AUX_STATE_BR2_SHIFT) & CAN_STCM_AUX_STATE_BR2_MASK;
    state->brake_3 = (frame->buf[CAN_STCM_AUX_STATE_BR3_BYTE] >> CAN_STCM_AUX_STATE_BR3_SHIFT) & CAN_STCM_AUX_STATE_BR3_MASK;
    state->autostop_marker_found = (frame->buf[CAN_STCM_AUX_STATE_AS_MD_BYTE] >> CAN_STCM_AUX_STATE_AS_MD_SHIFT) & CAN_STCM_AUX_STATE_AS_MD_MASK;
    state->horn_interlock=(frame->buf[CAN_STCM_AUX_STATE_HL_BYTE] >> CAN_STCM_AUX_STATE_HL_SHIFT) & CAN_STCM_AUX_STATE_HL_MASK;
    state->direction_flag=(frame->buf[CAN_STCM_AUX_STATE_DIR_BYTE] >> CAN_STCM_AUX_STATE_DIR_SHIFT) & CAN_STCM_AUX_STATE_DIR_MASK;
    state->s_e_stop = (frame->buf[CAN_STCM_AUX_STATE_SESTOP_BYTE] >> CAN_STCM_AUX_STATE_SESTOP_SHIFT) & CAN_STCM_AUX_STATE_SESTOP_MASK;
    state->h_e_stop = (frame->buf[CAN_STCM_AUX_STATE_HESTOP_BYTE] >> CAN_STCM_AUX_STATE_HESTOP_SHIFT) & CAN_STCM_AUX_STATE_HESTOP_MASK;
    state->fire_alarm = (frame->buf[CAN_STCM_AUX_STATE_FIRE_BYTE] >> CAN_STCM_AUX_STATE_FIRE_SHIFT) & CAN_STCM_AUX_STATE_FIRE_MASK;
    state->stationary_lock = (frame->buf[CAN_STCM_AUX_STATE_BH_BYTE] >> CAN_STCM_AUX_STATE_BH_SHIFT) & CAN_STCM_AUX_STATE_BH_MASK;
    state->coupler_status = (coupler_status_t)((frame->buf[CAN_STCM_AUX_STATE_COUPLER_BYTE] >> CAN_STCM_AUX_STATE_COUPLER_SHIFT) & CAN_STCM_AUX_STATE_COUPLER_MASK);
    state->drive_mode = (drive_mode_t)((frame->buf[CAN_STCM_AUX_STATE_DRIVE_MODE_BYTE] >> CAN_STCM_AUX_STATE_DRIVE_MODE_SHIFT) & CAN_STCM_AUX_STATE_DRIVE_MODE_MASK);
    break;
  case ECM_CAP_ENERGY:
  {
    state->super_cap_energy_joules = ((*((uint16_t*)&frame->buf[CAN_ECM_CAP_ENERGY_ENERGY_REL_OFFSET]))*0.1f) * 0.000278; //converts to WH, naming convention is no longer accurate as it is NOT Joules
    state->cap_volts = *((uint16_t*)&frame->buf[CAN_ECM_CAP_ENERGY_CAP_VOLTAGE_OFFSET]);
  }
  break;
  case ECM_VESC_POWER:
  {
    state->bus_vesc = *((uint16_t *)&frame->buf[CAN_ECM_VESC_POWER_VESC_VOLTAGE_OFFSET]);
    state->trac_current = (*((int16_t *)&frame->buf[CAN_ECM_VESC_POWER_VESC_CURRENT_OFFSET])) * 0.1f;
    state->vesc_inst_eff = (*((int16_t *)&frame->buf[CAN_ECM_VESC_POWER_VESC_INST_EFF_OFFSET])) * 0.1f;
  }
  break;
  case ECM_POWER_SUPPLY:
  {
    state->bus36v = *((uint16_t*)&frame->buf[CAN_ECM_POWER_SUPPLY_36V_BUS_OFFSET]);
    state->bus24v = *((uint16_t*)&frame->buf[CAN_ECM_POWER_SUPPLY_24V_BUS_OFFSET]);
    state->bus12v = *((uint16_t*)&frame->buf[CAN_ECM_POWER_SUPPLY_12V_BUS_OFFSET]);
    state->contactor3 = (frame->buf[CAN_ECM_POWER_SUPPLY_C3_S_BYTE] >> CAN_ECM_POWER_SUPPLY_C3_S_SHIFT) & CAN_ECM_POWER_SUPPLY_C3_S_MASK;
    state->contactor2 = (frame->buf[CAN_ECM_POWER_SUPPLY_C2_S_BYTE] >> CAN_ECM_POWER_SUPPLY_C2_S_SHIFT) & CAN_ECM_POWER_SUPPLY_C2_S_MASK;
    state->contactor1 = (frame->buf[CAN_ECM_POWER_SUPPLY_C1_S_BYTE] >> CAN_ECM_POWER_SUPPLY_C1_S_SHIFT) & CAN_ECM_POWER_SUPPLY_C1_S_MASK;
    state->b12OK = (frame->buf[CAN_ECM_POWER_SUPPLY_B12_OK_BYTE] >> CAN_ECM_POWER_SUPPLY_B12_OK_SHIFT) & CAN_ECM_POWER_SUPPLY_B12_OK_MASK;
    state->b5OK = (frame->buf[CAN_ECM_POWER_SUPPLY_B5_OK_BYTE] >> CAN_ECM_POWER_SUPPLY_B5_OK_SHIFT) & CAN_ECM_POWER_SUPPLY_B5_OK_MASK;
  }
  break;
  case STCM_THROTTLE_STAT:
    state->kill_switch_relay = (frame->buf[CAN_STCM_THROTTLE_STAT_KSR_BYTE] >> CAN_STCM_THROTTLE_STAT_KSR_SHIFT) & CAN_STCM_THROTTLE_STAT_KSR_MASK;
    state->auto_throttle_en = (frame->buf[CAN_STCM_THROTTLE_STAT_ATE_BYTE] >> CAN_STCM_THROTTLE_STAT_ATE_SHIFT) & CAN_STCM_THROTTLE_STAT_ATE_MASK;
    state->driver_throttle_en = (frame->buf[CAN_STCM_THROTTLE_STAT_DTE_BYTE] >> CAN_STCM_THROTTLE_STAT_DTE_SHIFT) & CAN_STCM_THROTTLE_STAT_DTE_MASK;
    break;
  case STCM_MSG_PUSH:
  {
    state->msg_push = *((uint8_t*)&frame->buf[CAN_STCM_MSG_PUSH_MSG_ID_OFFSET]);
  }
  break;
  default:
    break;
  }
}