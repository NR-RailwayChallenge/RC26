// Throttle Control
// Author: Ross Forbes, with additions from Oliver Flaks
// Last Updated: 11.06.2024

#include "throttle_drive_mode_functions.h"

void remapDriverThrottle(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, EphemeralValues *ephemeralState)
{
    // Emergency code in RC24 to bypass drive modes. Break Glass Code.
    // //TEMP START
    // ephemeralState->remapped_driver_throttle = currentReceive->driver_throttle_position;
    // nextSend->driver_throttle_enabled = true;
    // return;


    // //TEMP END
    
    if (nextSend->drive_mode == DriveModeEnum::SHUNTING)
    {
        // TODO: CR0603 RR - KSR, DTE should not be written here. This function should only deal with the driver throttle and propose
        //  KSR to be disabled, if required by the final output code.
        //  nextSend->ksr = true;
        // TODO: CR0603 RR - SPEEDLIMITS should be the reponsibility of the final throttle output code.
        // nextSend->speed_lim = SHUNTLIMIT_KMH;
        int32_t throttle_value_intermediate = currentReceive->driver_throttle_position - NEUTRAL_THROTTLE;
        throttle_value_intermediate /= SHUNTMODE_THROTTLE_DIV;
        throttle_value_intermediate += NEUTRAL_THROTTLE;
        ephemeralState->remapped_driver_throttle = throttle_value_intermediate;
        // The final throttle output will override this if autostop marker is found.
        nextSend->driver_throttle_enabled = true;
        // Block +ve tractive effort in regen mode if we're below the regenerated energy threshold.
    }
    else if (nextSend->drive_mode == DriveModeEnum::REGEN && currentReceive->cap_energy_rel_joules < REGEN_TRACTIVE_MIN_ENERGY_JOULES && currentReceive->driver_throttle_position > NEUTRAL_THROTTLE)
    {

        ephemeralState->remapped_driver_throttle = NEUTRAL_THROTTLE;
        nextSend->driver_throttle_enabled = false;
    }
    else
    {
        // In all other states, the driver_throttle is enabled. The output function will disable driver_throttle if requested by autostop.
        ephemeralState->remapped_driver_throttle = currentReceive->driver_throttle_position;
        nextSend->driver_throttle_enabled = true;
    }

    // if (nextSend->drive_mode == DriveModeEnum::STANDARD || nextSend->drive_mode == DriveModeEnum::REGEN)
    //{
    //  TODO: CR0603 RR - This would continually msgpush, even when in standard mode.
    // nextSend->msg_push = MessagePush::REGEN_MODE_START;
    // nextSend->ksr = true;
    // nextSend->driver_throttle_enabled = true;
    // nextSend->speed_lim = STANDARDLIMIT_KMH;
    // nextSend->out_throttle = currentReceive->driver_throttle_position;
    //}
}

void processDriveModeSwitching(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, IncomingValues *lastReceived, EphemeralValues *ephemeralState)
{
    // TODO: CR0603 RR - The previous mode switching code did not correctly prevent transition from shunt to regen.

    // Check that there actually is a request to change mode
    if (currentReceive->mode_req != DriveModeEnum::NEUTRAL && currentReceive->mode_req != lastReceived->mode_req && currentReceive->mode_req != lastSent->drive_mode)
    {
        switch (currentReceive->mode_req)
        {
        case DriveModeEnum::SHUNTING:
            if (lastSent->drive_mode == DriveModeEnum::STANDARD && currentReceive->driver_throttle_position == 2047)
            {
                nextSend->drive_mode == DriveModeEnum::SHUNTING;
            }
            else
            {
                ephemeralState->msg_mode_change_failed = true;
            }
            break;
        case DriveModeEnum::STANDARD:
            if ((lastSent->drive_mode == DriveModeEnum::SHUNTING && currentReceive->driver_throttle_position == 2047) || lastSent->drive_mode != DriveModeEnum::SHUNTING)
            {
                nextSend->drive_mode = DriveModeEnum::STANDARD;
            }
            else
            {
                ephemeralState->msg_mode_change_failed = true;
            }
            break;
        case DriveModeEnum::REGEN:
            if (lastSent->drive_mode == DriveModeEnum::STANDARD)
            {
                nextSend->drive_mode = DriveModeEnum::REGEN;
            }
            else
            {
                ephemeralState->msg_mode_change_failed = true;
            }
            break;
        case DriveModeEnum::AUTOSTOP:
            if (lastSent->drive_mode == DriveModeEnum::STANDARD)
            {
                nextSend->drive_mode = DriveModeEnum::AUTOSTOP;
            }
            else
            {
                ephemeralState->msg_mode_change_failed = true;
            }
            break;
        default:
            break;
        }
    }

    // TODO: CR0603 RR - The distance remaining magic values are handled in can proto.
    /*if (currentReceive->mode_req != DriveModeEnum::AUTOSTOP && currentReceive->mode_req != DriveModeEnum::NEUTRAL)
    {
        nextSend->autostop_marker_found = false;           // clears the marker detected stick when change out of autostop mode
        nextSend->autostop_distance_remaining_m = -301.0f; // Inform RDT to hide distance counter
    }*/
    nextSend->autostop_marker_found = nextSend->drive_mode == DriveModeEnum::AUTOSTOP && (lastSent->autostop_marker_found || ephemeralState->retroreflector_seen);

    // Protective function to end REGEN early if the ECM detected cap voltage < battery voltage.
    // Detected by contactor 1 being open, and then being closed despite not having exited regen mode.
    if (nextSend->drive_mode == REGEN && lastReceived->contactor1Status == false && currentReceive->contactor1Status == true)
    {
        nextSend->drive_mode = STANDARD;
        ephemeralState->msg_regen_protection_exit = true;
    } // else if used here as the protective function should NOT display a standard 'regen finished' message.
    else if (lastSent->drive_mode == REGEN && nextSend->drive_mode != REGEN)
    {
        ephemeralState->msg_regen_finished = true;
    }
}

void calculateVirtualThrottle(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, EphemeralValues *ephemeralState, PiController *controller)
{

    if (nextSend->drive_mode == AUTOSTOP)
    {
        // TODO: CR0603 RR The following 2 statements were outside the braces. This meant only the first line was conditional.
        //  The rest of the code would have executed regardless of drive mode.
        // This comment won't make much sense now due to the refactoring. See link for Tranche 1 context https://github.com/NR-Colas-Railway-Challenge/RC-STCM/blob/30942a91a26f687d768361c75820e23dcbbbf1c9/src/throttle_drive_mode_functions.cpp#L34
        // nextSend->speed_lim = AUTOSTOPLIMIT_KMH;
        // nextSend->ksr = true;
        if (lastSent->drive_mode != AUTOSTOP)
        {
            ephemeralState->msg_autostop_started = true;
            // nextSend->msg_push = MessagePush::AS_STARTED;
        }
        if (nextSend->autostop_marker_found == false) // ATO does nothing, leave control with the driver.
        {
            // nextSend->autostop_distance_remaining_m = 301.0f; // TODO: CR0603 RR - Handled in CAN proto
            // nextSend->driver_throttle_enabled = true;
            // nextSend->out_throttle = currentReceive->driver_throttle_position;
            ephemeralState->autostop_requests_throttle_control = false; // This is the default value anyway, but here for code clarity.
            nextSend->ato_throttle = NEUTRAL_THROTTLE;
        }
        else
        {
            ephemeralState->autostop_requests_throttle_control = true;
            // TODO: CR0601 Kiran Did you correct the condition here for me?
            // TODO: CR0603 RR Yes, I think I did, as part of adding the new PI controller, although it has changed with the refactor.
            if (lastSent->autostop_marker_found == false) // We are initialising autostop mode
            {
                ephemeralState->msg_autostop_braking_initiated = true;
                nextSend->autostop_start_dist_m = nextSend->trip_dist_m;
                nextSend->autostop_distance_remaining_m = AUTOSTOP_DISTANCE_m;
                controller->Reset(); // Clear any old accumulated error and integral history. // PI Controller
            }
            nextSend->autostop_distance_remaining_m = AUTOSTOP_DISTANCE_m - (nextSend->trip_dist_m - nextSend->autostop_start_dist_m); 

            if (nextSend->autostop_distance_remaining_m > AUTOSTOP_SERVICE_BRAKE_DISTANCE && nextSend->speed > STATIONARY_THRESHOLD_KMH)
            {
                if (ephemeralState->new_acceleration_value_available)
                {
                    float speedNow_mps = nextSend->speed / 3.6;
                    // float speedLast_mps = lastSent->speed; // TODO: CR0603 AR - Unused. Even if used, this would not be the speed at the last time ATO was calculated. Please remove line.
                    float speedEnd_mps = 0;
                    float a_target = (speedNow_mps * speedNow_mps - speedEnd_mps * speedEnd_mps) / (2 * nextSend->autostop_distance_remaining_m); //(u²-v²)/2s
                    float error = a_target - nextSend->acceleration_mss;                                                                          // too large braking will result in a positive error, too little braking a negative error
                    uint16_t throttlePI = BRAKE_BASELINE + controller->GetNextControlOutput(error);                                               // TODO: TUNE & baseline brake value also requires tuning
                    if (throttlePI >= NEUTRAL_THROTTLE)
                    {
                        nextSend->ato_throttle = NEUTRAL_THROTTLE; // prevents positive tractive effort. Could be taken out if rules ever change.
                    }
                    else
                    {
                        nextSend->ato_throttle = throttlePI;
                    }
                }
                // Else leave the ATO throttle at its previous position.
            }
            else
            {
                // Autostop has completed. Relinquish ATO control.
                ephemeralState->autostop_requests_throttle_control = false;
                ephemeralState->autostop_requests_brake_hold = true;
                // TODO: Use the request_brake_hold flag to trigger the autostop complete announcement.
                // TODO: CR0603 AR - Prune the dead code below.
                // nextSend->msg_push = MessagePush::AS_FINISHED;
                // applyEStop(nextSend);
            }
        }
    }
    else
    {
        // In other drive modes, ATO throttle does nothing.
        nextSend->ato_throttle = NEUTRAL_THROTTLE;
    }
}

void selectThrottleAndApplySpeedLimit(OutgoingValues *nextSend, EphemeralValues *ephemeralState)
{
    // First determine the KSR state.
    // The e-stop checks are partially redundant as brake hold will be active if e-stop is.
    // This is somewhat safety critical though.
    nextSend->ksr = !(nextSend->brake_hold || nextSend->h_estop || nextSend->s_estop);

    // Driver throttle is active by default, unless something has requested ATO priority.
    if (ephemeralState->autostop_requests_throttle_control)
    {
        nextSend->ato_throttle_enabled = true;
        nextSend->driver_throttle_enabled = false;
    }
    else
    {
        nextSend->ato_throttle_enabled = false;
        // Leave the driver_throttle_enabled status as the value set in remapDriverThrottle()
    }

    if (!nextSend->ksr) //if KSR true = allow throttle, KSR False = Kill switch activated, neutralise throttle.
    {
        nextSend->out_throttle = NEUTRAL_THROTTLE;
    }
    else if (nextSend->driver_throttle_enabled)
    {

        // Emergency code in RC24 to bypass drive modes. Break Glass Code.
        // //TEMP START
        // nextSend->out_throttle = ephemeralState->remapped_driver_throttle;
        // return;

        // //TEMP END

        // apply speed limiting to the driver throttle

        // First, ensure the correct speed limit is applied, based on the drive mode
        switch (nextSend->drive_mode)
        {
        case DriveModeEnum::SHUNTING:
            nextSend->speed_lim = SHUNTLIMIT_KMH;
            break;
        case DriveModeEnum::AUTOSTOP:
            nextSend->speed_lim = AUTOSTOPLIMIT_KMH;
            break;
        default:
            nextSend->speed_lim = STANDARDLIMIT_KMH;
            break;
        }

        // Now apply the actual limiting
        // Algorithm explanation: Do nothing to throttle until 80% of speed limit. Then, scale throttle by % left to the speed limit. For example for 15 kmh limit, start limiting at 12 kmh. 
        // At 13 kmh, we're 1/3 of the way between 12 to 15, so scale the throttle by ((15-13)/(15-12) = 0.67). This does not apply loco brakes at any point, even if speed limit is exceeded going downhill, for example.
        float startLimitSpeed = LIMPERCENT * nextSend->speed_lim;
        float overspeedMargin = nextSend->speed_lim - startLimitSpeed;
        float overSpeedEst = nextSend->speed_lim - nextSend->speed; // The bit of speed where we exceed the start limit speed.
        int32_t throttleIntermediate = ephemeralState->remapped_driver_throttle - NEUTRAL_THROTTLE;
        float throttleScale = overSpeedEst / overspeedMargin;
        if (throttleIntermediate >= 0) //This means we're demanding tractive effort, check if speed is too fast.
        {
            if(nextSend->speed > startLimitSpeed && throttleScale <=1 && throttleScale >= 0){ //do a check here to make sure we don't end up scaling our throttle above 1 or by a negative number
                nextSend->out_throttle = throttleScale * throttleIntermediate + NEUTRAL_THROTTLE;
            }
            else if(nextSend->speed > startLimitSpeed && throttleScale < 0){ //This situation occurs when we're above the speed limit, say 16 kmh so (15-16)/(15-12) will return a negative number. Set throttle to neutral in this case.
                throttleScale = 0;
                nextSend->out_throttle = throttleScale * throttleIntermediate + NEUTRAL_THROTTLE; 
            }
            else{ // we have yet to hit the speed at which we start limiting, proceed as usual. 
                nextSend->out_throttle = ephemeralState->remapped_driver_throttle;
            }
        }
        else{ //intermediate throttle <0, braking requested so we don't touch the throttle at all.
                nextSend->out_throttle = ephemeralState->remapped_driver_throttle;
        }
        //Removed for RC25. Don't want to auto-initiate braking, which could be confusing for the driver. Driver needs to maintain awareness of going overspeed. 
        // if (nextSend->speed > nextSend->speed_lim)
        // {
        //     // TODO: CR0603 AR - This will only apply a very fractional brake that might even be ignored due to the VESC's internal deadband.
        //     // Again, our VESC can regen brake quite strongly, so be mindful of braking too eagerly which could reduce ride comfort.
        //     nextSend->out_throttle = NEUTRAL_THROTTLE - (nextSend->speed - nextSend->speed_lim); // Applies brakes proportionally to how much over the limit we are
        // }
    }
    else if (nextSend->ato_throttle_enabled)
    {
        // If the ATO throttle is enabled, and the driver throttle is not enabled, copy the ato_throttle without applying the limiter.
        // The ATO processor will have its own speed limiting code, we should not interfere with it here.
        nextSend->out_throttle = nextSend->ato_throttle;
    }
    else
    {
        // As a safety fallback, if neither throttle is active, but KSR is not disabling the VESC, set out_throttle to neutral.

        // Don't activate the KSR due to both driver_throttle and ato_throttle not being enabled
        // This might rapidly toggle, and the KSR is a physical chain of relays that we don't want to oscillate.
        nextSend->out_throttle = NEUTRAL_THROTTLE;
    }
}