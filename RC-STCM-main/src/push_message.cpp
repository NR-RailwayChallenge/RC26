#include "push_message.h"

unsigned long pushMessagePostedTimer = 0;

int getMessageTimeoutMs(CAN_STCM_MSG_PUSH_MSG_ID_ENUM message){
    // TODO: Implement variable timeout?
    return 3000;
}

void setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM message, OutgoingValues *nextSend){
    if(nextSend->msg_push > message){
        nextSend->msg_push = message;
        pushMessagePostedTimer = millis();
    }
}

void evaluatePushMessages(OutgoingValues *nextSend, EphemeralValues *ephemeralState){
    //If there's an existing push message, check if it's ready to timeoff.
    if(nextSend->msg_push != CAN_STCM_MSG_PUSH_MSG_ID_ENUM::NULL_MESSAGE && (millis() - pushMessagePostedTimer> getMessageTimeoutMs(nextSend->msg_push))){
        //Message has timed out
        nextSend->msg_push = CAN_STCM_MSG_PUSH_MSG_ID_ENUM::NULL_MESSAGE;
    }

    //TODO: CR0603 Repeat for any other push messages. I've just done a few examples here.
    if(ephemeralState->msg_mode_change_failed) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::MODE_CHANGE_IMPOSS, nextSend);
    if(ephemeralState->msg_autostop_braking_initiated) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::AS_MARK_DETECT, nextSend);
    if(ephemeralState->msg_autostop_started) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::AS_STARTED, nextSend);
    if(ephemeralState->msg_regen_finished) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::REGEN_MODE_FINISH, nextSend);
    if(ephemeralState->msg_regen_protection_exit) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::REGEN_MODE_FINISH, nextSend);
    if(ephemeralState->msg_direction_change_blocked) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::DIR_CHANGE_IMPOSS, nextSend);
    if(ephemeralState->msg_smoke) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::SMOKE_DETECT, nextSend);
    if(ephemeralState->msg_heat) setPushMessage(CAN_STCM_MSG_PUSH_MSG_ID_ENUM::OVERHEAT_DETECT, nextSend);

}