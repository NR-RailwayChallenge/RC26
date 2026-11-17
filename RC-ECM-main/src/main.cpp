/*  Railway Challenge 2024 - Energy Control Module
 *  By Kiran Chinnam (kiran.chinnam@networkrail.co.uk)
 *  Date: 2024-May-19
 *  Version: 2024-05-A1
 */

// includes
#include <Arduino.h>
#include "STM32F446_RCC_init.h"
#include "STM32_CAN.h"
#include "pin_defs.h"
#include "constants.h"
#include "debug.h"
#include "rc_can_defs.h"
#include "outgoing_definitions.h"
#include "incoming_struct.h"
#include "can_transport.h"
#include "timing.h"
#include "can_proto.h"
#include "rolling_average_voltage.h"
#include "vesc_proto_defs.h"
#include "utils.h"

// Object declarations
CanTransport *can;
STM32_CAN canIf(PIN_CAN_PORT, PIN_CAN_PINS);
STM32_CAN canIfV(PIN_CAN_VESC_PORT, PIN_CAN_VESC_PINS);
RollingAverageAnalogIn bus12(PIN_BUS_12, R1_12V, R2_12V);
RollingAverageAnalogIn bus24(PIN_BUS_24, R1_24V, R2_24V);
RollingAverageAnalogIn bus36(PIN_BUS_36, R1_36V, R2_36V);
RollingAverageAnalogIn busSuperCap(PIN_BUS_SUPERCAP, R1_SUPERCAP, R2_SUPERCAP);

// Variables
// Co-operative task switcher vars
unsigned long lastDue = 0;
unsigned long nextDue = 0;
unsigned long timeNow;
// Performance Counters
volatile int droppedFrameCtr = 0;
volatile int rxErrCtr = 0;
volatile int rxRc = 0;

unsigned long resetProtectTimer = 0;

// This variable is used to make the heartbeat LED work
int heartbeatCounter = 0;

bool messageTypeStatusFlags[tx_msg_type_count];
outgoing_values lastOutgoing;
outgoing_values currentOutgoing;
incoming_values lastIncoming;
incoming_values currentIncoming;

VescStatus currentVescState[VESC_COUNT];
VescStatus previousVescState[VESC_COUNT];

// iter_ctr can be used to perform actions only on every nth run of the primary.
// e.g to run something every 50 ms, use an if statement like this:
// if (iter_ctr % 5 == 0){
//
// }
int iter_ctr = 0;

void secondary();
void primary();

void setup()
{
  pinMode(PIN_HEARTBEAT_LED, OUTPUT);
  digitalWrite(PIN_HEARTBEAT_LED, HIGH);
  // Debugging delay to allow serial monitor to connect.
  delay(2000);

  digitalWrite(PIN_HEARTBEAT_LED, LOW);
  // Setup of pins
  Serial.begin(115200);

  pinMode(PIN_C1, INPUT);
  pinMode(PIN_C2, INPUT);
  pinMode(PIN_C3, INPUT);
  pinMode(PIN_RCR, OUTPUT);
  pinMode(PIN_B5_OK, INPUT);
  pinMode(PIN_B12_OK, INPUT);

  analogReadResolution(12);
  pinMode(PIN_PRIMARY_TASK_TRACE, OUTPUT);
  pinMode(PIN_SECONDARY_TASK_TRACE, OUTPUT);

  // pinMode(PIN_BUS_12, INPUT_ANALOG);
  // pinMode(PIN_BUS_24, INPUT_ANALOG);
  // pinMode(PIN_BUS_36, INPUT_ANALOG);
  // pinMode(PIN_BUS_SUPERCAP, INPUT_ANALOG);

  // Belt and Braces - Zero out the statuses structs to initialise as zero.
  memset(previousVescState, 0, sizeof(previousVescState));
  memset(currentVescState, 0, sizeof(currentVescState));

  can = new CanTransport(&canIf);

  //--------CAN-V Init---------------
  canIfV.begin(true);
  canIfV.setBaudRate(CAN_BAUDRATE);

  Serial.printf("ready\n");
  lastDue = us();
  nextDue = us() + COOP_OS_MAINTASK_PERIOD;
  digitalWrite(PIN_HEARTBEAT_LED, HIGH);
}

// -------------DO NOT EDIT THE MAIN LOOP!!!----------------
// The main loop is here just to manage the running of a simple 'co-operative' task switcher.
// The real work happens in the primary and secondary functions. loop() ensures that the primary task
// is executed once every 10 ms, and tries to squeeze in as many executions of secondary in between as possible.
//
// While the nucleo has the capability to run a full RTOS, a simple co-operative model should work just fine
// Maybe a future improvement?
void loop()
{
  timeNow = us();
  // Second OR clause is to handle the case where micros() overflows
  if (timeNow < nextDue || (nextDue < lastDue && timeNow > lastDue))
  {
    digitalWrite(PIN_SECONDARY_TASK_TRACE, HIGH);
    secondary();
    digitalWrite(PIN_SECONDARY_TASK_TRACE, LOW);
  }
  else
  {
    lastDue = nextDue;
    nextDue += COOP_OS_MAINTASK_PERIOD;
    digitalWrite(PIN_PRIMARY_TASK_TRACE, HIGH);
    primary();
    digitalWrite(PIN_PRIMARY_TASK_TRACE, LOW);
  }
}

// This function checks to see what has changed since last time. If a variable has changed, then the CAN message than contains
// that variable needs to be set to 'STALE' to transmit it.
void detect_changes()
{
  lastOutgoing = currentOutgoing;
  // Set your current variables to the latest values by calling digitalRead, analogRead etc...
  currentOutgoing.contactor_1 = !digitalRead(PIN_C1);
  currentOutgoing.contactor_2 = !digitalRead(PIN_C2);
  currentOutgoing.contactor_3 = !digitalRead(PIN_C3);
  currentOutgoing.dc_dc_12_ok = digitalRead(PIN_B12_OK);
  currentOutgoing.dc_dc_5_ok = digitalRead(PIN_B5_OK);

  // Bus voltages are handled by a utility class to handle conversion to voltage and averaging.
  currentOutgoing.bus_12 = bus12.read();
  currentOutgoing.bus_24 = bus24.read();
  currentOutgoing.bus_36 = bus36.read();
  currentOutgoing.cap_mv = busSuperCap.read();

  // RCR closes during regen to disconnect the battery.
  currentOutgoing.rcr_state = currentIncoming.drive_mode == drive_mode_t::Regen;

  if (currentIncoming.drive_mode == drive_mode_t::Regen && lastIncoming.drive_mode != drive_mode_t::Regen && ms() - resetProtectTimer > RESET_PROTECTION_MS)
  {
    // We have just switched to regen mode, so zero the counter, provided that we exited regen mode more than RESET_PROTECTION_MS ago.
    currentOutgoing.rel_wh = 0;
    currentOutgoing.rel_j_offset = lastOutgoing.abs_j;
  }
  else if (currentIncoming.drive_mode != drive_mode_t::Regen && lastIncoming.drive_mode == drive_mode_t::Regen)
  {
    // Exiting regen mode, start the protection timer.
    resetProtectTimer = ms();
  }

  // Check if the contents have changed which would mean the message needs to be sent.
  if (currentOutgoing.vesc_mv != lastOutgoing.vesc_mv || currentOutgoing.vesc_current_scaled != lastOutgoing.vesc_current_scaled || currentOutgoing.vesc_inst_eff != lastOutgoing.vesc_inst_eff)
  {
    messageTypeStatusFlags[MTSF_ECM_VESC_POWER] = STALE;
  }

  if (currentOutgoing.contactor_1 != lastOutgoing.contactor_1 || currentOutgoing.contactor_2 != lastOutgoing.contactor_2 || currentOutgoing.contactor_3 != lastOutgoing.contactor_3 ||
      currentOutgoing.dc_dc_12_ok != lastOutgoing.dc_dc_12_ok || currentOutgoing.dc_dc_5_ok != lastOutgoing.dc_dc_5_ok ||
      currentOutgoing.bus_36 != lastOutgoing.bus_36 || currentOutgoing.bus_24 != lastOutgoing.bus_24 || currentOutgoing.bus_12 != lastOutgoing.bus_12)
  {
    messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] = STALE;
  }

  if (currentOutgoing.cap_mv != lastOutgoing.cap_mv)
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

  currentOutgoing.abs_wh += whDelta;
  currentOutgoing.rel_wh += whDelta;
  currentOutgoing.vesc_mv = voltsAcc * 1000;
  currentOutgoing.vesc_current_scaled = currentAcc * 10;
  currentOutgoing.vesc_inst_eff = whKm * 10;
}

void primary()
{
  // Process queued state changes from the secondary task
  // Look for any changes
  detect_changes();

  calc_vesc_stats();

  // E = ½CV²
  float v = currentOutgoing.cap_mv*1000.0f;
  currentOutgoing.abs_j = 0.5f * v * v * CAPACITANCE_F;
  currentOutgoing.rel_j = currentOutgoing.abs_j - currentOutgoing.rel_j_offset;

  if (currentOutgoing.abs_j != lastOutgoing.abs_j || currentOutgoing.rel_j != lastOutgoing.rel_j){
    messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] = STALE;
  }

  bool forceStale = iter_ctr % 5 == 1;

  // Transmit the default message if we're at risk of exceeding the keepalive interval.
  // The details of which message is default for your node will be in the CAN protocol spec.
  // Ask the Electrical Team Lead if you're not sure.
  if (forceStale || can->stcmTxTimerElapsed())
  {
    messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] = STALE;
    // Another disabled debugging feature. Uncomment if investigating unexpected behaviour.
    // Serial.printf("PTSS %lu\n", ms());
  }

  // Mark all messages as stale if the STCM connection drops.
  bool stcmOnline = can->stcmOnline();
  if (forceStale || !stcmOnline || !currentIncoming.self_online)
  {
    // memset is a little 'performance optimising hack'
    memset(messageTypeStatusFlags, STALE, sizeof(messageTypeStatusFlags));
  }

  // Send messages if they're marked as stale. Add an if statement for each of your outgoing CAN message types.
  if (forceStale || messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::ECM_CAP_ENERGY, &currentOutgoing);
    // This line will mark the message as fresh if ALL of these conditions are true (in order):
    //  * The message was either sent on the CAN bus, or it was added to the queue
    //  * The STCM is reporting this node as online.
    //  * The receiver is online.
    messageTypeStatusFlags[MTSF_ECM_CAP_ENERGY] = res && currentIncoming.self_online && stcmOnline && currentIncoming.handheld_online;
  }
  if (forceStale || messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::ECM_POWER_SUPPLY, &currentOutgoing);
    // Notice that hear, we are sending to the mm, so we check for mm_online instead of stcmOnline.
    messageTypeStatusFlags[MTSF_ECM_POWER_SUPPLY] = res && currentIncoming.self_online && stcmOnline && currentIncoming.handheld_online;
  }
  if (forceStale || messageTypeStatusFlags[MTSF_ECM_VESC_POWER] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::ECM_VESC_POWER, &currentOutgoing);
    messageTypeStatusFlags[MTSF_ECM_VESC_POWER] = res && currentIncoming.self_online && stcmOnline && currentIncoming.handheld_online;
  }

  // Do any other work here - e.g. for the LAM, this could be starting to play an audio file.

  // TODO: Confirm - Relay breakout requires LOW signal to energise relay.
  digitalWrite(PIN_RCR, !currentOutgoing.rcr_state);

  lastIncoming = currentIncoming;
  // Copy current Vesc State to the previous Vesc State
  memcpy(previousVescState, currentVescState, sizeof(previousVescState));

  iter_ctr++;

  // Blink the LED to show we're still alive
  heartbeatCounter++;
  heartbeatCounter %= 100;
  if (heartbeatCounter == 0 || heartbeatCounter == 20)
  {
    digitalWrite(PIN_HEARTBEAT_LED, HIGH);
  }
  if (heartbeatCounter == 10 || heartbeatCounter == 30)
  {
    digitalWrite(PIN_HEARTBEAT_LED, LOW);
  }
}

void receive_and_parse_vesc()
{
  CAN_message_t rxMsg;
  if (canIfV.read(rxMsg))
  {
    // Received a message from a VESC
    // Extract the VESC ID and see if it is from a VESC that we know
    int vescIndex = (rxMsg.id >> 4)-1;
    // Discard the packet if it doesn't match a pre-defined index.
    if (vescIndex >= VESC_COUNT)
      return;
    switch (rxMsg.id & 0x0F)
    {
    case 1:
      currentVescState[vescIndex].rpm = decodeBigEndianInt32(rxMsg.buf);
      currentVescState[vescIndex].current = decodeBigEndianInt16(&rxMsg.buf[4]) / 10.0f;
      currentVescState[vescIndex].dutyCycle = decodeBigEndianInt16(&rxMsg.buf[6]) / 1000.0f;
      break;
    case 2:
      currentVescState[vescIndex].ahConsumed = decodeBigEndianInt32(rxMsg.buf) / 10000.0f;
      currentVescState[vescIndex].ahCharged = decodeBigEndianInt32(&rxMsg.buf[4]) / 10000.0f;
      break;
    case 3:
      currentVescState[vescIndex].whConsumed = decodeBigEndianInt32(rxMsg.buf) / 10000.0f;
      currentVescState[vescIndex].whCharged = decodeBigEndianInt32(&rxMsg.buf[4]) / 10000.0f;
      break;
    case 4:
      currentVescState[vescIndex].mosfetTemp = decodeBigEndianInt16(rxMsg.buf) / 10.0f;
      currentVescState[vescIndex].motorTemp = decodeBigEndianInt16(&rxMsg.buf[2]) / 10.0f;
      currentVescState[vescIndex].inputCurrent = decodeBigEndianInt16(&rxMsg.buf[4]) / 10.0f;
      currentVescState[vescIndex].pidPosition = decodeBigEndianInt16(&rxMsg.buf[6]) / 50.0f;
      break;
    case 5:
      currentVescState[vescIndex].tachoRel = decodeBigEndianInt32(rxMsg.buf);
      currentVescState[vescIndex].voltageIn = decodeBigEndianInt16(&rxMsg.buf[4]) / 10.0f;
      break;
    case 6:
      currentVescState[vescIndex].adc1millivolts = decodeBigEndianInt16(rxMsg.buf);
      currentVescState[vescIndex].adc2millivolts = decodeBigEndianInt16(&rxMsg.buf[2]);
      currentVescState[vescIndex].adc3millivolts = decodeBigEndianInt16(&rxMsg.buf[4]);
      currentVescState[vescIndex].servoPosition = decodeBigEndianInt16(&rxMsg.buf[6]);
      break;
    default:
      // Unrecognised message somehow got through the filters - ignore it.
      return;
    }
    currentVescState[vescIndex].seenBefore = true;
    currentVescState[vescIndex].lastContactMillis = ms();
  }
}

// This function should not need much/any modification.
void secondary()
{
  if (can->readable())
  {
    CAN_message_t canFrame = can->read();
    parse_frame(&canFrame, &currentIncoming);
  }
  receive_and_parse_vesc();
}
