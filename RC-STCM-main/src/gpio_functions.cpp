// =============================================================================
// GPIO FUNCTIONALITY
// Written by Oliver Flaks and Kiran Chinnam
// 11.06.2024
// =============================================================================

#include "gpio_functions.h"

// =============================================================================
// Pulls input data from relevant input pin, doesn't pull CAN data
// =============================================================================
void readGpioInputs(OutgoingValues* nextSend, OutgoingValues* lastSent, EphemeralValues* ephemeralState)
{
    nextSend->h_estop = digitalRead(PIN_E_STOP_HEALTH);
    if (!digitalRead(PIN_AUTOSTOP))
    {
        //TODO: CR0603 RR - Switched to ephemeral value which is copied to AS_MD by autostop code for seperation of concerns.
        nextSend->autostop_marker_found = true;
    }
    // TODO: CR0602 Kiran Is this an outstanding problem? ANS: Yes.
    // This breaks the INPUT-EVALUATE-OUTPUT principle, since you're evaluating whether there is a brake fault here. 
    // TODO: CR0603 AR - Here, check only for whether the brakes are applied or not. BR3 is a special case.
 
    // TODO: TUNE brake fault sensors, there is a high threshhold that may be used if we have 2 brakes on 1 sensor
    ephemeralState->measured_br1 = (analogRead(PIN_BRAKE_FAULT_1)/ADC_MAX_FLOAT * ADC_SYS_VOLTS) >= BRAKE_FAULT_THRESHOLD;
    ephemeralState->measured_br2 = (analogRead(PIN_BRAKE_FAULT_2)/ADC_MAX_FLOAT * ADC_SYS_VOLTS) >= BRAKE_FAULT_THRESHOLD;
    ephemeralState->measured_br3 = (analogRead(PIN_BRAKE_FAULT_3)/ADC_MAX_FLOAT * ADC_SYS_VOLTS) >= BRAKE_FAULT_THRESHOLD_HI;
    // Check if exactly one of the brakes on BR3 is active. This should never happen and indicates a definite brake fault.
    ephemeralState->measured_br3_ooc = (analogRead(PIN_BRAKE_FAULT_3)/ADC_MAX_FLOAT * ADC_SYS_VOLTS) >= BRAKE_FAULT_THRESHOLD
     && (analogRead(PIN_BRAKE_FAULT_3)/ADC_MAX_FLOAT * ADC_SYS_VOLTS) < BRAKE_FAULT_THRESHOLD_HI;
    
    nextSend->fire = !digitalRead(PIN_SMOKE_DETECT) || !digitalRead(PIN_HEAT_DETECT);

    //TODO: Refactor MSGpush to external func.
    if(digitalRead(PIN_SMOKE_DETECT)==0)
    {
        ephemeralState->msg_smoke = true;
    }
    if(digitalRead(PIN_HEAT_DETECT)==0)
    {
        ephemeralState->msg_heat = true;
    }

    nextSend->autocoupler_couple_detect = digitalRead(PIN_AUTO_COUPLER_DETECT);
    nextSend->autocoupler_current_sensor_volts = (analogRead(PIN_COUPLING_DIR_DETECT) / ADC_MAX_FLOAT) * ADC_SYS_VOLTS;
}



// =============================================================================
// Writes pins based on the outgoing values states, doesn't do control.
// =============================================================================
void writeGpioOutputs(OutgoingValues* nextSend, IncomingValues* currentReceive)
{
    //TODO: CR0603 RR - Throttle selection is handled by selectThrottleAndApplySpeedLimit().
    // this will write out relevent nextSend values to the digital pins:
    //if (nextSend->driver_throttle_enabled == true && nextSend->ato_throttle_enabled == false)
    //{
    //    analogWrite(PIN_THROTTLE_OUT, nextSend->out_throttle);
    //}
    //if (nextSend->driver_throttle_enabled == false && nextSend->ato_throttle_enabled == true)
    //{
    //    // TODO: CR0602 - Kiran we discussed difference between ATO throttle and out throttle, and their respective enable. Delete if happy
    //    analogWrite(PIN_THROTTLE_OUT, nextSend->ato_throttle);
    //}

    //TODO: CR0603 RR - out_throttle should always reflect what is sent to the VESC.
    analogWrite(PIN_THROTTLE_OUT, nextSend->out_throttle);

    digitalWrite(PIN_KSR, !nextSend->ksr);//This is consistent with CAN proto, and KSR low for no drive.
    digitalWrite(PIN_LR, nextSend->current_direction_is_reverse); // Low for reverse, high for normal, always sent. 

    // activates brakes based on the brake demand defined elsewhere
    digitalWrite(PIN_MBR, nextSend->mbr);//These Inversions are now correct as I understand (OF)
    digitalWrite(PIN_BR1, nextSend->br1);
    digitalWrite(PIN_BR2, nextSend->br2);
    digitalWrite(PIN_BR3, nextSend->br3);

    // horn sounding
    digitalWrite(PIN_HR, !currentReceive->horn_btn && nextSend->hh_online);

    // Autocoupler State Settings
    digitalWrite(PIN_ACR, nextSend->ACR);
    digitalWrite(PIN_ACDR, nextSend->ACD);
}
