// =============================================================================
// Braking Functions
// Written by Oliver Flaks and Kiran Chinnam
// 11.06.2024
// =============================================================================

// The Function of this code will be to check the status of the EStop Loop,
// and complete software disconnections

#include "braking_functions.h"

unsigned long hornInterlockTimer = 0;


void setBrakeRelays(BrakeDemand BrakeDemand, OutgoingValues *nextSend)
{
    switch (BrakeDemand)
    {
    case BrakeDemand::LVL_0:
        nextSend->br1 = false;
        nextSend->br2 = false;
        nextSend->br3 = false;
        nextSend->mbr = false;
        break;

    case BrakeDemand::LVL_1:
        nextSend->br1 = true;
        nextSend->br2 = false;
        nextSend->br3 = false;
        nextSend->mbr = true;
        break;

    case BrakeDemand::LVL_2:
        nextSend->br1 = true;
        nextSend->br2 = true;
        nextSend->br3 = false;
        nextSend->mbr = true;
        break;

    case BrakeDemand::LVL_3:
        nextSend->br1 = true;
        nextSend->br2 = true;
        nextSend->br3 = true;
        nextSend->mbr = true;
        break;

    default:
        // Something horribly wrong has happened if we are executing this block.
        // Full brake
        nextSend->br1 = true;
        nextSend->br2 = true;
        nextSend->br3 = true;
        nextSend->mbr = true;
        Serial.println("[FATAL] Attempt to set unknown brake level");
        break;
    }
}

void evaluateBrakesAndEmergencyStop(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, IncomingValues *lastReceived, EphemeralValues *ephemeralState)
{
    // First step is to evaluate the state of our e-stop.
    // Hardware e-stop is just a GPIO, so already updated in nextSend.
    //Any addition calls of s_estop would go here
    nextSend->s_estop = ephemeralState->e_stop_offline_condition;

    bool anyEStopIsActive = nextSend->s_estop || nextSend->h_estop;

    bool inhibitBrakeRelease = false;

#if defined(BRAKE_RELEASE_INHIBIT_BY_AUTOCOUPLER) && BRAKE_RELEASE_INHIBIT_BY_AUTOCOUPLER
    inhibitBrakeRelease = inhibitBrakeRelease || (nextSend->coupler_status == CouplerStatus::UNCOUPLING || nextSend->coupler_status == CouplerStatus::COUPLING);
#endif

    // Handle the horn timer
    // Horn timer is reset whilst the locomotive is not stationary, or the horn is sounded.
    if (currentReceive->horn_btn || nextSend->speed > STATIONARY_THRESHOLD_KMH)
    {
        hornInterlockTimer = millis();
    }

    // Check if there are any brake hold application requests
    bool applyOrMaintainBrakeHold = ephemeralState->autostop_requests_brake_hold || anyEStopIsActive || millis() - hornInterlockTimer > BRAKE_RELOCK_TIMER_MS;

    // Handle release of brake hold.
    // Conditions are:
    //  - Not attempting to apply brake hold
    //  - Not inhibited by any inhibitors
    //  - Driver is hold service brake, ready to takeover
    //  - Horn button was recently released
    if (!applyOrMaintainBrakeHold && currentReceive->brake_demand != LVL_0 && !inhibitBrakeRelease && currentReceive->horn_btn == false && lastReceived->horn_btn == true)
    {
        nextSend->brake_hold = false;
    }
    else if (applyOrMaintainBrakeHold)
    {
        nextSend->brake_hold = true;
    }
    // Otherwise brake hold retains its previous value.

    // Set the appropriate brake output
    if (nextSend->brake_hold)
    {
        setBrakeRelays(BRAKE_HOLD_LEVEL, nextSend);
    }
    else
    {
        setBrakeRelays(currentReceive->brake_demand, nextSend);
    }

    /*// transition from local brake to nextRecieve.brake demand - requires brake held, then the horn button to be pressed, also checks that we're currently in brake hold
    if (currentReceive.brake_demand != LVL_0 && lastSent.brake_hold == true && nextSend.h_estop == false && nextSend.s_estop == false &&
        currentReceive.horn_btn == false && lastReceived.horn_btn == true)
    {
        nextSend.brake_hold = false;
        setBrakeRelays(currentReceive.brake_demand, &nextSend);
        // TODO: CR0603 RR - Remove this block once done. I think a brake released push message is not a good idea because of how often it'll be displayed.
        // If you disagree, please add a variable to the ephemeral struct, as with the other push messages.
        nextSend.msg_push = MessagePush ::BRAKE_RELEASED;
        nextSend.ksr = true; // allow traction again.
    }*/

    // Check for brake faults
    //  If any brakes are not measured to be equal to their commanded ouput, we have a potential brake fault.
    //  Additional check for single brake applied in the BR3 pair. Either both should be on or both off. Never only 1.
    if (ephemeralState->measured_br1 != nextSend->br1 || ephemeralState->measured_br2 != nextSend->br2 || ephemeralState->measured_br3 != nextSend->br3 || ephemeralState->measured_br3_ooc)
    {
        nextSend->brake_fault_detect = true;
        if (lastSent->brake_fault_detect == false)
        {
            nextSend->brake_fault_detect_time = millis();
        }
        else if (millis() - (nextSend->brake_fault_detect_time) > BRAKE_FAULT_TIMER_MS)
        {
            nextSend->brake_fault = true;
        }
    }
    else
    {
        nextSend->brake_fault_detect = false;
        nextSend->brake_fault = false;
    }
}