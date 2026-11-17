/* Railway Challenge 2024 - Handheld Main uC Firmware
 *  By Kiran Chinnam (kiran.chinnam@networkrail.co.uk)
 *  Date: 2024-April-05
 *  Version: 2024-04-A1
 */

// includes
#include <Arduino.h>
#include "STM32F446_RCC_init.h"
#include "pin_defs.h"
#include "constants.h"
#include "debug.h"
#include "rc_can_defs.h"
#include "implementation_defs.h"
#include "rx_vars.h"
#include "brake_demand_module.h"
#include "can_transport.h"
#include "pico_interface.h"
#include "timing.h"
#include "can_proto.h"

// Object declarations
BrakeDemandModule brakeMod(PIN_BRAKE);
PicoInterface *picoIf;
CanTransport *can;
STM32_CAN canIf(CAN1, DEF);

// Variables
// Co-operative task switcher vars
unsigned long lastDue = 0;
unsigned long nextDue = 0;
unsigned long timeNow;
// Performance Counters
volatile int droppedFrameCtr = 0;
volatile int rxErrCtr = 0;
volatile int rxRc = 0;

int heartbeatCounter = 0;

// This variable captures the timestamp of the moment the autostop
// button was pressed so that press duration can be calculated.
unsigned long autostop_press_ts = 0;
// This variable captures the timestamp of the moment the regen
// button was pressed so that press duration can be calculated.
unsigned long regen_press_ts = 0;
// A mode request should be cancelled after 500 ms, so need this
// variable tracks when the mode request started.
unsigned long mode_change_rq_ts = 0;

bool messageTypeStatusFlags[tx_msg_type_count];
tracked_outgoing lastValues;
tracked_outgoing currentValues;
incoming_vars lastVariables;
incoming_vars currentVariables;
bool dsdState = false;
int iter_ctr = 0;

void secondary();
void primary();

void setup()
{
  pinMode(PIN_HB_LED, OUTPUT);
  digitalWrite(PIN_HB_LED, HIGH);
  // Debugging delay to allow serial monitor to connect.
  delay(2000);

  digitalWrite(PIN_HB_LED, LOW);
  // Setup of pins
  Serial.begin(115200);
  pinMode(PIN_DSD_DETECT, INPUT);
  pinMode(PIN_HORN, INPUT);
  pinMode(PIN_CHANGE_DIRECTION, INPUT);
  pinMode(PIN_AUTOSTOP, INPUT);
  pinMode(PIN_REGEN, INPUT);
  pinMode(PIN_COUPLE_BTN, INPUT);
  analogReadResolution(12);
  pinMode(PIN_PRIMARY_TASK_TRACE, OUTPUT);
  pinMode(PIN_SECONDARY_TASK_TRACE, OUTPUT);
  // Not required, left here to highlight pin use.
  // pinMode(PIN_BRAKE, INPUT_ANALOG);
  // pinMode(PIN_JS_SIG, INPUT_ANALOG);
  can = new CanTransport(&canIf);
  picoIf = new PicoInterface(PIN_PICO_TX, PIN_PICO_RX);
  // picoIf->wait_until_ready();
  Serial.printf("ready\n");
  Serial.flush();

  // Initialise default mode request as neutral
  currentValues.mode_rq = drive_mode_t::Neutral;
  lastValues.mode_rq = drive_mode_t::Neutral;

  delay(1500);
  lastDue = us();
  nextDue = us() + COOP_OS_MAINTASK_PERIOD;
  digitalWrite(PIN_HB_LED, HIGH);
}

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

void detect_changes()
{
  unsigned long timeNow = ms();
  int autostop_press_duration = 0;
  int regen_press_duration = 0;

  lastValues = currentValues;
  currentValues.local_brake_demand = brakeMod.GetDemand();
  currentValues.autostop_btn = !digitalRead(PIN_AUTOSTOP);
  currentValues.couple = !digitalRead(PIN_COUPLE_BTN);
  currentValues.change_direction = !digitalRead(PIN_CHANGE_DIRECTION);
  currentValues.horn = !digitalRead(PIN_HORN);
  currentValues.regen_btn = !digitalRead(PIN_REGEN);
  currentValues.throttle_pos = analogRead(PIN_JS_SIG);
  dsdState = !digitalRead(PIN_DSD_DETECT);

  // Apply deadbanding - if the throttle is close to neutral, force to neutral.
  // This does mean that it is impossible to apply e.g 2% throttle, but this should be OK.
  if (currentValues.throttle_pos > THROTTLE_DEADBAND_LOWER && currentValues.throttle_pos < THROTTLE_DEADBAND_UPPER)
  {
    currentValues.throttle_pos = THROTTLE_NEUTRAL;
  }

  //Extinguish the request if we've been requesting it for more than the timeout period,
  // or the STCM has acked by switching to the requested mode.
      // If message sent successfully, or we've been repeating the message for more than timeout interval,
    // extinguish the request by setting to neutral.
  if(currentValues.mode_rq!= drive_mode_t::Neutral && (currentValues.mode_rq==lastVariables.drive_mode||(mode_change_rq_ts - ms() > MODE_CHANGE_TIMEOUT_MS))){
    currentValues.mode_rq = drive_mode_t::Neutral;
  }

  // Is the auto stop button being newly pressed? Start the timer if so.
  if (!lastValues.autostop_btn && currentValues.autostop_btn)
  {
    autostop_press_ts = timeNow;
    Serial.println("Start AS TIM");
  }
  else if (lastValues.autostop_btn && !currentValues.autostop_btn)
  {
    // Button has been released, calc the duration.
    autostop_press_duration = timeNow - autostop_press_ts;
    Serial.printf("Stop AS TIM: %dms\n", autostop_press_duration);
  }

  // Same for the regen button
  if (!lastValues.regen_btn && currentValues.regen_btn)
  {
    regen_press_ts = timeNow;
    Serial.println("Start RG TIM");
  }
  else if (lastValues.regen_btn && !currentValues.regen_btn)
  {
    regen_press_duration = timeNow - regen_press_ts;
    Serial.printf("Stop RG TIM: %dms\n", regen_press_duration);
  }

  if ((autostop_press_duration > 0 || regen_press_duration > 0) && can->stcmOnline())
  {
    autostop_press_duration = 0;
    regen_press_duration = 0;
    // There is a mode change request, work out what it is.
    // In the highly unlikely event that the autostop and regen buttons are pressed
    // and released together with 10 ms accuracy, it is possible to have 2 conflicting
    // mode change requests. Process the autostop button last as this should always
    // take priority in a conflict.

    //  -----------------REGEN BUTTON MODE CHANGE HANDLER--------------------------
    // Really short press is probably unintentional. Long press is also likely unintentional, so filter these.
    if (regen_press_duration > MODE_CHANGE_SHORT_PRESS_MS && regen_press_duration <= MODE_CHANGE_PRESS_LIMIT_MS)
    {
      // The press was within limits, so continue with the request.
      if (regen_press_duration >= MODE_CHANGE_LONG_PRESS_MS)
      {
        // Driver long pressed. If the joystick is neutral, this is a request to enter shunt mode, only if currently in standard.
        if (currentValues.throttle_pos == THROTTLE_NEUTRAL && currentVariables.drive_mode == drive_mode_t::Standard)
        {
          currentValues.mode_rq = drive_mode_t::Shunting;
          mode_change_rq_ts = timeNow;
        }
      }
      else
      {
        // It was a short press.

        // In autostop mode or shunting mode, this has no effect.
        // In regen mode, revert to standard.
        // In standard mode, go to regen mode.
        if (currentVariables.drive_mode == drive_mode_t::Standard)
        {
          currentValues.mode_rq = drive_mode_t::Regen;
          mode_change_rq_ts = timeNow;
        }
        else if (currentVariables.drive_mode == drive_mode_t::Regen)
        {
          currentValues.mode_rq = drive_mode_t::Standard;
          mode_change_rq_ts = timeNow;
        }
      }
    }

    //  -----------------AUTOSTOP BUTTON MODE CHANGE HANDLER--------------------------
    // Really short press is probably unintentional. Long press is also likely unintentional, so filter these.
    if (autostop_press_duration > MODE_CHANGE_SHORT_PRESS_MS && autostop_press_duration <= MODE_CHANGE_PRESS_LIMIT_MS)
    {
      // The press was within limits, so continue with the request.
      if (autostop_press_duration >= MODE_CHANGE_LONG_PRESS_MS)
      {
        // Driver long pressed. This is a request to enter standard mode.
        // If currently in shunting mode, there is an additional requirement for the joystick to be neutral.
        if (currentVariables.drive_mode == drive_mode_t::Regen || currentVariables.drive_mode == drive_mode_t::Autostop || (currentVariables.drive_mode == drive_mode_t::Shunting && currentValues.throttle_pos == THROTTLE_NEUTRAL))
        {
          currentValues.mode_rq = drive_mode_t::Standard;
          mode_change_rq_ts = timeNow;
        }
      }
      else
      {
        // It was a short press.

        // In regen mode or shunting mode, this has no effect.
        // In autostop mode, revert to standard.
        // In standard mode, go to autostop mode.
        if (currentVariables.drive_mode == drive_mode_t::Standard)
        {
          currentValues.mode_rq = drive_mode_t::Autostop;
          mode_change_rq_ts = timeNow;
        }
        else if (currentVariables.drive_mode == drive_mode_t::Autostop)
        {
          currentValues.mode_rq = drive_mode_t::Standard;
          mode_change_rq_ts = timeNow;
        }
      }
    }
  }

  if (currentValues.mode_rq != lastValues.mode_rq || currentValues.change_direction != lastValues.change_direction || currentValues.horn != lastValues.horn || currentValues.couple != lastValues.couple || currentValues.local_brake_demand != lastValues.local_brake_demand)
  {
    messageTypeStatusFlags[MTSF_HH_BTN_STATE] = STALE;
  }

  if (currentValues.throttle_pos != lastValues.throttle_pos)
  {
    messageTypeStatusFlags[MTSF_HH_THROTTLE] = STALE;
  }
}

void primary()
{
  // Process queued state changes from the secondary task
  // Look for any changes
  detect_changes();

  // Transmit the default message if we're at risk of exceeding the keepalive interval.
  if (can->stcmTxTimerElapsed())
  {
    messageTypeStatusFlags[MTSF_HH_THROTTLE] = STALE;
    //Serial.printf("PTSS %lu\n", ms());
  }

  // Mark all as stale if the STCM connection drops or every 50 ms.
  bool stcmOnline = can->stcmOnline();
  if (!stcmOnline || !currentVariables.self_online || iter_ctr%5 == 1)
  {
    memset(messageTypeStatusFlags, STALE, sizeof(messageTypeStatusFlags));
  }

  // No other audience for our messages except for RDT. RDT is 'passive logging',
  // so don't explicitly mark as stale for RDT offline.

  // Transmit messages
  if (messageTypeStatusFlags[MTSF_HH_BTN_STATE] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::HH_BTN_STATE, &currentValues);
    messageTypeStatusFlags[MTSF_HH_BTN_STATE] = res && currentVariables.self_online && stcmOnline;
  }

  if (messageTypeStatusFlags[MTSF_HH_THROTTLE] == STALE)
  {
    bool res = compose_and_send(can, Can_Aid::HH_THROTTLE, &currentValues);
    messageTypeStatusFlags[MTSF_HH_THROTTLE] = res && currentVariables.self_online && stcmOnline;
  }

  // Send a PICO_PACK, but only every 100 ms;
  if (iter_ctr % 10 == 0)
  {
    picoIf->update_dsd_state(dsdState);
    picoIf->import_states(&currentVariables);
    picoIf->update_can_state(can->stcmOnline());
    // if(can->stcmOnline()){
    //   Serial.println("Online");
    // }else{
    //   Serial.println("Offline");
    // }
    picoIf->tx_pico_pack();
  }
  else if (iter_ctr % 10 == 5)
  {
    picoIf->import_vals(&currentValues);
    picoIf->update_dsd_state(dsdState);
    picoIf->import_analogue_values(brakeMod.GetCachedRaw(), currentValues.throttle_pos);
    picoIf->tx_pico_diag();
  }
  lastVariables = currentVariables;
  // Reset the MSG_PUSH var so we don't keep displaying the same message.
  currentVariables.msg_push = 0;
  iter_ctr++;

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
    parse_frame(&canFrame, &currentVariables);
  }
}
