// =============================================================================
// Autocoupler Functions
// Written by Oliver Flaks and Kiran Chinnam
// 11.06.2024
// =============================================================================
#include "autocoupler_functions.h"

void processAutocouplerStates(OutgoingValues *nextSend, OutgoingValues *lastSent, IncomingValues *currentReceive, IncomingValues *lastReceived)
{ // ============================================================================
  // Moving between coupler states: - LOCO Initialises in UNCOUPLED STATE
  // ============================================================================

  // Beginning Coupling 
  if ((nextSend->coupler_status == UNCOUPLED) && (currentReceive->auto_coupler_btn == true && lastReceived->auto_coupler_btn == false ))
  {
    nextSend->coupler_status = COUPLING;
  }
  // Reverting to uncoupling from coupling if (button is pressed again after being released, or if we have an E-Stop) - This is the only time we change state as the result of E-stop.
  if ((nextSend->coupler_status == COUPLING) && ((currentReceive->auto_coupler_btn == true && lastReceived->auto_coupler_btn == false) || (nextSend->h_estop || nextSend->s_estop == true)))
  {
    nextSend->coupler_status = UNCOUPLING;
  }
  // Detecting Coupled // TODO - Check how the Pogo Pins connect, can we still use the autocoupler detect delay?
  if ((nextSend->coupler_status == COUPLING) && (nextSend->autocoupler_couple_detect == true))
  {
    nextSend->coupler_status = CouplerStatus::COUPLED;
  }
  // From Coupled to Uncoupling: 
  if ((nextSend->coupler_status == COUPLED) && (currentReceive->auto_coupler_btn == true && lastReceived->auto_coupler_btn == false) && nextSend->speed <= 3.0) //check speed is less than say 3kmh
  {
    nextSend->coupler_status = UNCOUPLING;
  }
  // From Uncoupling to uncoupled:
  if ((nextSend ->coupler_status == UNCOUPLING) && (currentReceive->auto_coupler_btn == true && lastReceived->auto_coupler_btn == false) && (nextSend->autocoupler_couple_detect == false)) 
  {
    nextSend->coupler_status = UNCOUPLED;
  }

  // ===========================================================================
  // Coupler States
  // ===========================================================================

  //ACR = AutoCoupler Relay that drives the actuator arm to RETRACT and allow for UNCOUPLING
  //ACD = AutoCoupler Relay that drives actuator arm FORWARDS and be in Ready to couple state

  if (nextSend->coupler_status == UNCOUPLING) //If uncoupling, set actuator such that coupler releases, otherwise set actuator to allow for coupling.
  {
    nextSend->ACR = true;
    nextSend->ACD = false;
  }
  else  {
    nextSend->ACR = false;
    nextSend->ACD = true;
  }
}
