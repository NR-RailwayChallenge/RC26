/* Railway Challenge 2024 - STCM Module Main Code
 *  By Oliver Flaks (oliver.flaks@networkrail.co.uk)
 *  Modified 2025 by Mengyu Shou (mengyu.shou@networkrail.co.uk)
 *  Date: 2025-June-16
 *  Version: 2025-06-A2
 */

// =============================================================================
// Includes
// =============================================================================
#include <Arduino.h>
#include <cstdint>
#include "constants.h"
#include "debug.h"
#include "pin_defs.h"
#include "rc_can_defs.h"
#include "implementation_defs.h"
#include "rx_vars.h"
#include "can_transport.h"
#include "timing.h"
#include "can_proto.h"
#include "STM32_CAN.h"
#include "STM32F446_conf.h"
#include "hh_enumerations.h"
#include "braking_functions.h"
#include "throttle_drive_mode_functions.h"
#include "vesc_proto_defs.h"
#include "utils.h"
#include "autocoupler_functions.h"
#include "pi_controller.h"
#include "gpio_functions.h"
#include "vesc_functions.h"
#include "rolling_average_voltage.h"
#include "vesc_proto_defs.h"

// Object declarations
CanTransport *can;
STM32_CAN canIf(PIN_CAN_PORT, PIN_CAN_PINS);
STM32_CAN canIfV(PIN_CAN_VESC_PORT, PIN_CAN_VESC_PINS);
PiController autoStopController(1.0e-6f * COOP_OS_MAINTASK_PERIOD * AUTOSTOP_DIVIDER, AUTOSTOP_KP, AUTOSTOP_KI);
RollingAverageAnalogIn bus12(PIN_BUS_12, R1_12V, R2_12V);
RollingAverageAnalogIn bus24(PIN_BUS_24, R1_24V, R2_24V);
RollingAverageAnalogIn bus36(PIN_BUS_36, R1_36V, R2_36V);
RollingAverageAnalogIn busSuperCap(PIN_BUS_SUPERCAP, R1_SUPERCAP, R2_SUPERCAP);

// Variables
// Co-operative task switcher vars
unsigned long lastDue = 0;
unsigned long nextDue = 0;
unsigned long timeNow = 0;

// Performance Counters
volatile int droppedFrameCtr = 0;
volatile int rxErrCtr = 0;
volatile int rxRc = 0;

unsigned long resetProtectTimer = 0;

// This variable is used to make the heartbeat LED work
int heartbeatCounter = 0;

// Counter to control the frequency with which auto stop runs and acceleration is calculated.
int accelerationAutoStopCounter = AUTOSTOP_DIVIDER - 1;
float previousSpeedCacheForAcceleration = 0.0f; // Variable to store the previous speed for acceleration calculations.
const float accelerationInterval = 1.0e-6f * COOP_OS_MAINTASK_PERIOD * AUTOSTOP_DIVIDER;

bool messageTypeStatusFlags[tx_msg_type_count];
OutgoingValues lastSent;
OutgoingValues nextSend;
IncomingValues lastReceived;
IncomingValues currentReceive;

VescStatus currentVescState[VESC_COUNT];
VescStatus previousVescState[VESC_COUNT];
// defines a VescStatus struct for each vescs

// TODO: CR0603 RR - I've renamed this to mark as single-purpose as your link_report code modifies the value.
int link_report_ctr = 0; // iteration counter
bool LR_arm = false; //counter used in the function to process the light relay
int LR_time = 2137483647; //high number to stop relays switching

void secondary();
void primary();

// =============================================================================
// Setup Code:
// =============================================================================
void setup()
{
  setBrakeRelays(BRAKE_HOLD_LEVEL, &nextSend);
  pinMode(PIN_HB_LED, OUTPUT); // heartbeat LED
  digitalWrite(PIN_HB_LED, HIGH);
  // Debugging delay to allow serial monitor to connect.
  delay(2000);

  digitalWrite(PIN_HB_LED, LOW);
  // Setup of pins
  Serial.begin(115200);
  while(!Serial){}
  // definition of input pins
  pinMode(PIN_SMOKE_DETECT, INPUT);
  pinMode(PIN_HEAT_DETECT, INPUT);
  pinMode(PIN_E_STOP_HEALTH, INPUT);
  pinMode(PIN_AUTO_COUPLER_DETECT, INPUT);
  pinMode(PIN_COUPLING_DIR_DETECT, INPUT);
  pinMode(PIN_AUTOSTOP, INPUT);
  //ECM Pins
  pinMode(PIN_C1, INPUT);
  pinMode(PIN_C2, INPUT);
  pinMode(PIN_C3, INPUT);
  pinMode(PIN_B5_OK, INPUT);
  pinMode(PIN_B12_OK, INPUT);

  //BUSBAR ALIAS Pins. Because we wired the PCB wrong. Set these to input so there isn't an accidental high output voltage to drown out busbar voltage.
  // pinMode(PIN_BUS_12_ALIAS,INPUT);
  // pinMode(PIN_BUS_24_ALIAS,INPUT);
  // pinMode(PIN_BUS_36_ALIAS,INPUT);
  // pinMode(PIN_BUS_SUPERCAP_ALIAS,INPUT);


  // Set up analogue Resolutions
  analogReadResolution(12);
  analogWriteResolution(12);

  // Definition of output pins
  pinMode(PIN_MBR, OUTPUT);
  pinMode(PIN_BR1, OUTPUT);
  pinMode(PIN_BR2, OUTPUT);
  pinMode(PIN_BR3, OUTPUT);
  pinMode(PIN_LR, OUTPUT);
  pinMode(PIN_HR, OUTPUT);
  pinMode(PIN_KSR, OUTPUT);
  pinMode(PIN_ACR, OUTPUT);
  pinMode(PIN_ACDR, OUTPUT);
  //ECM Pins
  pinMode(PIN_RCR, OUTPUT);

  // Initial PIN Startup conditions
  digitalWrite(PIN_LR, HIGH); // High output = forward

  // Not required, left here to highlight pin use.
  // pinMode(PIN_BRAKE, INPUT_ANALOG);
  // pinMode(PIN_JS_SIG, INPUT_ANALOG);
  can = new CanTransport(&canIf);
  //--------CAN-V Init---------------
  canIfV.begin(true);
  canIfV.setBaudRate(CAN_BAUDRATE);

  // Belt and Braces - Zero out the statuses structs to initialise as zero.
  memset(previousVescState, 0, sizeof(previousVescState));
  memset(currentVescState, 0, sizeof(currentVescState));

  Serial.print("ready\n");
  Serial.flush();
  delay(1500);
  lastDue = us();                           // Now
  nextDue = us() + COOP_OS_MAINTASK_PERIOD; // How often for primary task to be run//Now+10ms
  digitalWrite(PIN_HB_LED, HIGH);
}

// The main loop is here just to manage the running of a simple 'co-operative' task switcher.
// The real work happens in the primary and secondary functions. loop() ensures that the primary task
// is executed once every 10 ms, and tries to squeeze in as many executions of secondary in between as possible.
//
// While the nucleo has the capability to run a full RTOS, a simple co-operative model should work just fine
// Maybe a future improvement?

// =============================================================================
// Loop Code: (Do not mess with, only calls primary and secondary tasks in turn)
// =============================================================================
void loop()
{
  timeNow = us();
  // Second OR clause is to handle the case where micros() overflows
  if (timeNow < nextDue || (nextDue < lastDue && timeNow > lastDue))
  {
    TDW(PIN_SECONDARY_TASK_TRACE, HIGH);
    secondary();
    TDW(PIN_SECONDARY_TASK_TRACE, LOW);
  }
  else
  {
    lastDue = nextDue;
    nextDue += COOP_OS_MAINTASK_PERIOD;
    TDW(PIN_PRIMARY_TASK_TRACE, HIGH);
    primary();
    TDW(PIN_PRIMARY_TASK_TRACE, LOW);
  }
}
//============================================================================================
// ECM Functions: Ported over from the old ECM. That's why some flags will reference the ECM.
//============================================================================================

// This function checks to see what has changed since last time. If a variable has changed, then the CAN message than contains
// that variable needs to be set to 'STALE' to transmit it.
void detect_changes()
{
  lastSent = nextSend;
  // Set your current variables to the latest values by calling digitalRead, analogRead etc...
  nextSend.contactor_1 = !digitalRead(PIN_C1);
  nextSend.contactor_2 = !digitalRead(PIN_C2);
  nextSend.contactor_3 = !digitalRead(PIN_C3);
  nextSend.dc_dc_12_ok = digitalRead(PIN_B12_OK);
  nextSend.dc_dc_5_ok = digitalRead(PIN_B5_OK);

  // Bus voltages are handled by a utility class to handle conversion to voltage and averaging.
  nextSend.bus_12 = bus12.read();
  nextSend.bus_24 = bus24.read();
  nextSend.bus_36 = bus36.read();
  nextSend.cap_mv = busSuperCap.read();

  // RCR closes during regen to disconnect the battery.
  nextSend.rcr_state = currentReceive.drive_mode == DriveModeEnum::REGEN;

  if (currentReceive.drive_mode == DriveModeEnum::REGEN && lastReceived.drive_mode != DriveModeEnum::REGEN && ms() - resetProtectTimer > RESET_PROTECTION_MS)
  {
    // We have just switched to regen mode, so zero the counter, provided that we exited regen mode more than RESET_PROTECTION_MS ago.
    nextSend.rel_wh = 0;
    nextSend.rel_j_offset = lastSent.abs_j;
  }
  else if (currentReceive.drive_mode != DriveModeEnum::REGEN && lastReceived.drive_mode == DriveModeEnum::REGEN)
  {
    // Exiting regen mode, start the protection timer.
    resetProtectTimer = ms();
  }

  // Check if the contents have changed which would mean the message needs to be sent.
  if (nextSend.vesc_mv != lastSent.vesc_mv || nextSend.vesc_current_scaled != lastSent.vesc_current_scaled || nextSend.vesc_inst_eff != lastSent.vesc_inst_eff)
  {
    messageTypeStatusFlags[MTSF_ECM_VESC_POWER] = STALE;
  }

  if (nextSend.contactor_1 != lastSent.contactor_1 || nextSend.contactor_2 != lastSent.contactor_2 || nextSend.contactor_3 != lastSent.contactor_3 ||
      nextSend.dc_dc_12_ok != lastSent.dc_dc_12_ok || nextSend.dc_dc_5_ok != lastSent.dc_dc_5_ok ||
      nextSend.bus_36 != lastSent.bus_36 || nextSend.bus_24 != lastSent.bus_24 || nextSend.bus_12 != lastSent.bus_12)
  {
    messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] = STALE;
  }

  if (nextSend.cap_mv != lastSent.cap_mv)
  {
    messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] = STALE;
  }
}

void calc_vesc_stats(){
  // From the VESC data, we need to calculate:
  // VESC Bus Voltage - Take an average of all VESCs
  // VESC Net Current In - Take the sum of all VESCs
  // Speed - To calculate instantaneous energy efficiency, average of all VESCs 
  // Delta of Net Energy Consumption - Sum of all online VESCs

  int onlineVescs = 0;
  float voltsAcc = 0;
  float currentAcc = 0;
  long rpmAcc = 0;
  // +ve values = Net Recovered Energy, -ve values = Net Consumed Energy.
  float whDelta = 0;

  unsigned long tNow = ms();

  for (int i = 0; i < VESC_COUNT; i++)
  {
    if(tNow - currentVescState[i].lastContactMillis< VESC_TIMEOUT_MS){
      onlineVescs++;
    }else{
      //TODO: What should happen if a VESC is offline?
      continue;
    }

    voltsAcc += currentVescState[i].voltageIn;
    currentAcc += currentVescState[i].inputCurrent;
    rpmAcc += currentVescState[i].rpm;
    whDelta += previousVescState[i].whCharged - currentVescState[i].whCharged;
    whDelta -= previousVescState[i].whConsumed - currentVescState[i].whConsumed;
  }

  // Volts and RPM are sum, divide by online count to get arithmetic mean.
  voltsAcc/=onlineVescs;
  rpmAcc/=onlineVescs;

  // See https://lucidar.me/en/unit-converter/revolutions-per-minute-to-kilometers-per-hour/
  float speedKmh = 0.12f * PI * WHEEL_RADIUS_M * (rpmAcc/(DRIVE_REDUCTION_GEAR_RATIO * MOTOR_POLE_PAIRS));
  float consumedWatts = voltsAcc * currentAcc;

  float whKm = 0.0f;
  if(speedKmh > 0 && abs(consumedWatts)> 0){
    whKm = consumedWatts/speedKmh;
  }

  if(whKm > 999.9f){
    whKm = 999.9f;
  }else if (whKm<-999.9f){
    whKm = -999.9f;
  }

  nextSend.abs_wh += whDelta;
  nextSend.rel_wh += whDelta;
  nextSend.vesc_mv = voltsAcc * 1000;
  nextSend.vesc_current_scaled = currentAcc * 10;
  nextSend.vesc_inst_eff = whKm * 10;
}

// =============================================================================
// Reads from the VGM and stores the relevant data from there
// =============================================================================
void processVescData()
{
  // From the VESC data, we need to calculate:

  // Speed - To calculate instantaneous energy efficiency, average of all VESCs
  // Delta of Net Energy Consumption - Sum of all online VESCs

  int onlineVescs = 0;
  long rpmAcc = 0;
  int32_t tachoAcc = 0;
  int tachoDiv = 0;

  unsigned long tNow = ms();

  for (int i = 0; i < VESC_COUNT; i++)
  {
    if (tNow - currentVescState[i].lastContactMillis < KEEPALIVE_MS)
    {
      onlineVescs++;
      rpmAcc += currentVescState[i].rpm;
    }
    else
    {
      // TO-DO: Kiran is the VESC Off to on code written now?What should happen if a VESC is offline?
      // TODO: CR0603 RR - Yes. If we don't have at least 1 VESC measurement, distance counter does not increment, and speed indication goes to 15 kmh.
      // For this to happen though, the VESC needs to have not reported for over 100 ms though, so temporary drops shouldn't affect us.
      // If the VESCS do not supply new data, the old speed value is reused.
      continue;
    }
    if (previousVescState[i].lastContactMillis < currentVescState[i].lastContactMillis && (ms() - previousVescState[i].lastContactMillis) < (COOP_OS_MAINTASK_PERIOD / 1000))
    {
      tachoAcc += abs(currentVescState[i].tachoRel - previousVescState[i].tachoRel);
      tachoDiv++;
    }
  }
  // If not all VESCs are seen, mark the VGM as offline.
  nextSend.vgm_online = (onlineVescs >= VESC_COUNT);

  if (onlineVescs > 0)
  {
    rpmAcc /= onlineVescs;
    // See https://lucidar.me/en/unit-converter/revolutions-per-minute-to-kilometers-per-hour/
    float speedKmh = 0.12f * PI * WHEEL_RADIUS_M * (rpmAcc / (DRIVE_REDUCTION_GEAR_RATIO * MOTOR_POLE_PAIRS));
    nextSend.speed = fabsf(speedKmh);
  }
  else
  {
    // TODO: CR0603 AR - Do you agree with the comment below? Ans: yes
    // TO-DO: Something has gone wrong with speed measurement. Safest to show we're going faster than we are?
    nextSend.speed = 15.0f;
  }
  if (tachoDiv > 0)
  {
    tachoAcc /= tachoDiv;
    nextSend.trip_dist_m += ((tachoAcc / (TACHO_RATIO * MOTOR_POLE_PAIRS)) / DRIVE_REDUCTION_GEAR_RATIO) * PI * WHEEL_DIAMETER_m;
  }
  else
  {
    // TODO: CR0603 AR - Do you agree with the comment below?ass:yes
    // TO-DO: If we don't have reliable trip_dist data, we currently ignore the increment. Is there a better way?
  }
}
// =============================================================================
// Identifies changes in outgoing values that need to be sent Via CAN.
// =============================================================================
void detectCanPayloadChanges(bool forceAllStale) // Finds changes in the CAN Messages that we're interested in.
{
  // the messages written by the STCM are the: STCM_TRACTION_STATE, STCM_AUX_STATE, STCM_THROTTLE_STAT, STCM_MESSAGE_PUSH

  if (forceAllStale || nextSend.speed != lastSent.speed || nextSend.speed_lim != lastSent.speed_lim || nextSend.autostop_distance_remaining_m != lastSent.autostop_distance_remaining_m || lastSent.trip_dist_m != nextSend.trip_dist_m)
  {
    messageTypeStatusFlags[MTSF_STCM_TRACTION_STATE] = STALE;
  }

  if (forceAllStale || nextSend.brake_fault != lastSent.brake_fault || nextSend.current_direction_is_reverse != lastSent.current_direction_is_reverse || nextSend.mbr != lastSent.mbr || nextSend.br1 != lastSent.br1 || nextSend.br2 != lastSent.br2 || nextSend.br3 != lastSent.br3 || nextSend.brake_hold != lastSent.brake_hold || nextSend.fire != lastSent.fire || nextSend.h_estop != lastSent.h_estop || nextSend.s_estop != lastSent.s_estop || nextSend.drive_mode != lastSent.drive_mode)
  {
    messageTypeStatusFlags[MTSF_STCM_AUX_STATE] = STALE;
  }

  if (forceAllStale || nextSend.ato_throttle != lastSent.ato_throttle || nextSend.out_throttle != lastSent.out_throttle)
  {
    messageTypeStatusFlags[MTSF_STCM_THROTTLE_STATE] = STALE;
  }

  if (forceAllStale || nextSend.msg_push != lastSent.msg_push)
  {
    messageTypeStatusFlags[MTSF_STCM_MESSAGE_PUSH] = STALE;
  }
  //ECM Message stale check
  if (nextSend.abs_j != lastSent.abs_j || nextSend.rel_j != lastSent.rel_j){
    messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] = STALE;
  }
}
void processLR(){
  int currTime = millis();
  //Process LED Driver Switching. Turn off for 40 ms, switch LR, Wait 40ms, turn on driver.
  if (nextSend.current_direction_is_reverse != lastSent.current_direction_is_reverse){
      LR_arm = true;
      LR_time = millis();
      //digitalWrite(PIN_LED_DRIVER,LOW); //Direction Change requested, turn off driver
  }
  if (LR_arm && currTime - LR_time >= 40 && currTime - LR_time < 500){ //make sure the time window is correct. Wait at least 40ms before switching LR. If more than 500ms elapsed since request, something's wrong and don't do anything
      digitalWrite(PIN_LR, nextSend.current_direction_is_reverse); // Low for reverse, high for normal, always sent. 
      LR_arm = false; // dearm the LR to indicate switching is finished.
  }
  if (!LR_arm && currTime - LR_time >=85 && currTime - LR_time < 500){ //LR unarmed to show switching completed, enough time must elapse since the switching happens. supposedly 80ms, but used 85ms here for a buffer in case of delays
      //digitalWrite(PIN_LED_DRIVER,HIGH); //turn on driver now
      LR_time = 2137483647; // large number to make sure we don't accidentally switch any of the relays
  }
  
}
// =============================================================================
// Primary task
// =============================================================================
void primary()
{
  // We declare our ephemeral state here so it will reset every loop.
  EphemeralValues ephemeralState;
  readGpioInputs(&nextSend, &lastSent, &ephemeralState);
  processVescData();
  // ===========================================================================
  // Calculate the acceleration value
  // ===========================================================================
  if (accelerationAutoStopCounter-- == 0)
  {
    // Reload the counter
    accelerationAutoStopCounter = AUTOSTOP_DIVIDER - 1;
    nextSend.acceleration_mss = ((nextSend.speed - previousSpeedCacheForAcceleration) / 3.6f) / accelerationInterval;
    previousSpeedCacheForAcceleration = nextSend.speed;
    ephemeralState.new_acceleration_value_available = true;
  }
  // =============================================================================
  // DRIVE MODE SELECTION
  // =============================================================================
  // select throttle map we would like
  // announce on CAN what throttle mode has been selected
  // can't change to or from shunting mode without neutral throttle
  // throttle 0->2047 = braking, 2047->4095 = acceleration
  processDriveModeSwitching(&nextSend, &lastSent, &currentReceive, &lastReceived, &ephemeralState);

  calculateVirtualThrottle(&nextSend, &lastSent, &currentReceive, &ephemeralState, &autoStopController);

  // Apply any remapping and copy the processed driver throttle to the ephemeral state.
  remapDriverThrottle(&nextSend, &lastSent, &currentReceive, &ephemeralState);

  // TODO: CR0603 RR - This should come after processing drive mode change requests, since the eStop checks rely on drive mode.
  // ===========================================================================
  // Checks every 5 iterations of the primary loop that all of the modules are online
  // ===========================================================================
  if (++link_report_ctr >= 5)
  {
    link_report_ctr = 0;
    nextSend.hh_online = can->HHOnline();
    nextSend.ecm_online = true; //can->ECMOnline(); 
    nextSend.lam_online = true; //can->LAMOnline();
    nextSend.rdt_online = true; //can->RDTOnline();
    messageTypeStatusFlags[MTSF_STCM_LINKREPORT] = STALE;
    // VGM online checks are handled in processVescData.
    if (nextSend.hh_online == false || nextSend.ecm_online == false || (nextSend.drive_mode == DriveModeEnum::REGEN && (nextSend.vgm_online == false)))
    {
      // TODO: CR0603 RR - Currently, there's only one reason soft eStop might be active, but for future, it's better to
      // set an ephemeral variable and evaulate later, rather than multiple bits of code overwriting the same var.
      ephemeralState.e_stop_offline_condition = true;
    }
  }

  // ============================================================================
  // Direction Change
  // ============================================================================

  // Direction is normal by default, set with the struct.
  // This sees if the direction button is pressed, and checks the loco is stationary before allowing the direction to be reversed
  //  check that only one throttle is active//TO-DO: CHECK With KIRAN if this is necessary. Loco will always be stationary.

  // TODO: CR0603 RR - Restructured the ifs below. In general, prefer use of else.
  // Not sure what you mean about checking if only one throttle is active.
  if (lastReceived.direction_btn == false && currentReceive.direction_btn == true)
  {
    if (nextSend.speed <= STATIONARY_THRESHOLD_KMH)
    {
      nextSend.current_direction_is_reverse = !lastSent.current_direction_is_reverse;
      Serial.println("Direction Fliped.");
    }
    else
    {
      ephemeralState.msg_direction_change_blocked = true;
      Serial.println("Direction change blocked.");
    }
  }

  /*
  Things that will cause all brakes applied and require brake release protocol to be removed:
  -Any ESTop present
  -Autostop complete - within autostop function function
  -Autocoupler in operation - within autocoupler function - autocoupler requires brake hold, not the other way around
  -Autocoupler lost detection erroneously - within autocoupler function - SES
  -Driver brake demand && speed low enough (Very low speed)
  -Re-apply brakes after 5 seconds of non-movement
  */
  // Allows drive if the brake button is pressed and the horn button pressed
  // Brake hold function
  // activate if stationary for 5 seconds, or there is brake demand while stationary that's not part of brake release sequence
  // brake release sequence will remove brake hold
  // brake release sequence :
  //-Brake demand held
  //-Horn Press and release
  // =============================================================================
  // Brake Hold Operation
  // =============================================================================
  evaluateBrakesAndEmergencyStop(&nextSend, &lastSent, &currentReceive, &lastReceived, &ephemeralState);

  // =============================================================================
  // Autocoupler Control:
  // =============================================================================
  processAutocouplerStates(&nextSend, &lastSent, &currentReceive, &lastReceived);

  // =============================================================================
  // Throttle output writing, takes speed limits into account.-doesn't apply to the ATO_Throttle, the ATO_throttle under autostop
  // =============================================================================
  selectThrottleAndApplySpeedLimit(&nextSend, &ephemeralState);


  // TODO: CR0603 RR - E-Stop is now processed inside the braking function. This block is now deprecated.
  // // =============================================================================
  // // E-Stop Function Call
  // // =============================================================================
  // if (nextSend.h_estop == true || nextSend.s_estop == true)
  // {
  //   setBrakeRelays(BRAKE_HOLD_LEVEL, &nextSend);
  //   nextSend.ksr = false;
  //   nextSend.brake_hold = true;
  // }
  // //  these will cause actions, and will update the nextSend values?
  // //  Reset the MSG_PUSH var so we don't keep displaying the same message.
  // // currentReceive.msg_push = 0;

  // =============================================================================
  // ECM Functionality. Calculate capacitor energy, detect changes, etc.
  // =============================================================================
  // Process queued state changes from the secondary task
  // Look for any changes
  detect_changes();

  calc_vesc_stats();

  // E = ½CV²
  float v = nextSend.cap_mv*1000.0f;
  nextSend.abs_j = 0.5f * v * v * CAPACITANCE_F;
  nextSend.rel_j = nextSend.abs_j - nextSend.rel_j_offset;

  // =============================================================================
  // Output stage - GPIO, then throttle over CAN-V, then other CAN-P
  // =============================================================================
  writeGpioOutputs(&nextSend, &currentReceive);
  //processLR();  //process Light Relay. LR GPIO Write instruction is here. REMEMBER TO DISABLE GPIO WRITE IN writeGpioOutputs if implemented!

  //NEW!! Send Throttle Output to VGM
  CAN_message_t throttle_packet;
  throttle_packet.id = CAN_STCM_THROTTLE_INPUT;
  throttle_packet.len = CAN_DLC_STCM_THROTTLE_INPUT;
  throttle_packet.buf[CAN_STCM_THROTTLE_INPUT_BIG_BYTE] = (nextSend.out_throttle>> 8) & 0xFF;
  throttle_packet.buf[CAN_STCM_THROTTLE_INPUT_LITTLE_BYTE] = (nextSend.out_throttle>> 0) & 0xFF;
  throttle_packet.buf[CAN_STCM_THROTTLE_INPUT_DIR_BYTE] = nextSend.current_direction_is_reverse;

  canIfV.write(throttle_packet);
  // =============================================================================
  // Sending of CAN Messages based on what values require updating
  // =============================================================================
  //  Mark relevant message as stale if module is offline
  // Force all messages to expire in the loop cycle after sending the link report.
  bool forceStale = link_report_ctr == 1;
  detectCanPayloadChanges(forceStale);
  if (messageTypeStatusFlags[MTSF_STCM_LINKREPORT] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::STCM_LINKREPORT, &nextSend);
    messageTypeStatusFlags[MTSF_STCM_LINKREPORT] = res; // LINK REPORT should only probe every 50 ms.
  }

  if (messageTypeStatusFlags[MTSF_STCM_TRACTION_STATE] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::STCM_TRACTION_STATE, &nextSend);
    messageTypeStatusFlags[MTSF_STCM_TRACTION_STATE] = res && nextSend.hh_online && nextSend.ecm_online && nextSend.rdt_online;
  }

  if (messageTypeStatusFlags[MTSF_STCM_AUX_STATE] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::STCM_AUX_STATE, &nextSend);
    messageTypeStatusFlags[MTSF_STCM_AUX_STATE] = res && nextSend.hh_online && nextSend.ecm_online && nextSend.lam_online && nextSend.rdt_online;
  }

  if (messageTypeStatusFlags[MTSF_STCM_THROTTLE_STATE] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::STCM_THROTTLE_STAT, &nextSend);
    messageTypeStatusFlags[MTSF_STCM_THROTTLE_STATE] = res && nextSend.hh_online && nextSend.rdt_online;
  }

  if (messageTypeStatusFlags[MTSF_STCM_MESSAGE_PUSH] == STALE && false) // TODO: KC - Remove '&& false' once MSG PUSH is implemented.
  {
    bool res = compose_and_send(can, Can_Aid::STCM_MSG_PUSH, &nextSend);
    messageTypeStatusFlags[MTSF_STCM_MESSAGE_PUSH] = res && nextSend.hh_online && nextSend.rdt_online;
  }
  
  //=========================================================================
  //ECM Messages
  //=========================================================================

  if (forceStale || messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::ECM_CAP_ENERGY, &nextSend);
    // This line will mark the message as fresh if ALL of these conditions are true (in order):
    //  * The message was either sent on the CAN bus, or it was added to the queue
    //  * The STCM is reporting this node as online.
    //  * The receiver is online.
    messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] = res && currentReceive.handheld_online;
  }
  if (forceStale || messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::ECM_POWER_SUPPLY, &nextSend);
    // Notice that hear, we are sending to the mm, so we check for mm_online instead of stcmOnline.
    messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] = res && currentReceive.handheld_online;
  }
  if (forceStale || messageTypeStatusFlags[MTSF_ECM_VESC_POWER] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::ECM_VESC_POWER, &nextSend);
    messageTypeStatusFlags[MTSF_ECM_VESC_POWER] = res && currentReceive.handheld_online;
  }

  //ECM Has this next 2 lines, not sure if still needed?
   // TODO: Confirm - Relay breakout requires LOW signal to energise relay.
  digitalWrite(PIN_RCR, !nextSend.rcr_state);

  lastSent = nextSend;
  lastReceived = currentReceive;
  // Copy current Vesc State to the previous Vesc State
  memcpy(previousVescState, currentVescState, sizeof(previousVescState));

  // Blink the LED to show we're still alive
  heartbeatCounter++;
  heartbeatCounter %= 100;
  if (heartbeatCounter == 0 || heartbeatCounter == 20)
  {
    digitalWrite(PIN_HB_LED, HIGH);
  }
  if (heartbeatCounter == 10 || heartbeatCounter == 30)
  {
    digitalWrite(PIN_HB_LED, LOW);
  }
}

void secondary()
{
  if (can->readable())
  {
    CAN_message_t canFrame = can->read();
    parse_frame(&canFrame, &currentReceive);
    // recieve from VESC gateway
  }
  receive_and_parse_vesc(&canIfV, currentVescState, previousVescState);
}
