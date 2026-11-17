/* Railway Challenge 2023 - Handheld Indications uC Firmware
 *  By Kiran Chinnam (kiran.chinnam@networkrail.co.uk)
 *  Date: 2023-June-25
 *  Version: 2023-06-F1
 */

// This sketch requires the following lib which isn't in the library manager.
// git clone https://github.com/Bodmer/OpenFontRender
// into your arduino/libraries directory.

#include "lcd_man.h"
#include "led_man.h"
#include "nucleo_interface.h"
#include "pin_defs.h"
#include "marshalled_data.h"

// Variables
// Co-operative task switcher vars
unsigned long lastDue = 0;
unsigned long nextDue = COOP_OS_MAINTASK_PERIOD;
unsigned long timeNow;
unsigned int iterCtr = 0;

LedManager ledm(PIN_LED_LEFT, PIN_LED_RIGHT);
NucleoInterface *nucIf;
LcdManager *lcdm;

MarshalledData C0Data;
MarshalledData C1Data;

bool simToolBtnState = false;

void primary();
void secondary();

void setup(void)
{
    memset(&C0Data, 0, sizeof(C0Data));
    memset(&C1Data, 0, sizeof(C1Data));
    Serial.begin(115200);
    Serial.println("Start");
    nucIf = new NucleoInterface(&NUCLEO_UART);
    pinMode(PIN_SIM_TOOL_DIAG, INPUT_PULLUP);
    simToolBtnState = digitalRead(PIN_SIM_TOOL_DIAG);
    // This function blocks while the LED self-test pattern is running.
    ledm.doSelfTest(); 
    nucIf->signalReady();
}

void loop()
{
    timeNow = micros();
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

void primary()
{
    // Check if we should toggle diag mode
    bool newSimToolState = digitalRead(PIN_SIM_TOOL_DIAG);
    if(simToolBtnState == 1 && newSimToolState == 0){
        C0Data.diagMode=!C0Data.diagMode;
    }
    simToolBtnState = newSimToolState;
    // Check if core 1 is asking for new data
    uint32_t *rv = 0;
    if (rp2040.fifo.pop_nb(rv))
    {
        // It's asking for data, so core 0 can safely access the lcd data struct
        memcpy(&C1Data, &C0Data, sizeof(C0Data));
        // Signal to the other core that it can continue.
        rp2040.fifo.push(1);
    }
    ledm.render(&C0Data);
    iterCtr++;
    if (iterCtr % 20 == 0)
    {
        Serial.printf("HB %u\n", iterCtr);
        Serial.flush();
    }
}

void secondary()
{
    if (nucIf->processNextChunk())
    {
        MessageType mt = nucIf->checkMessageType();
        switch (mt)
        {
        case MessageType::Pico_Pack:
            C0Data.packet = nucIf->getNextPacket();
            Serial.println("G-Pk");
            break;
        case MessageType::Pico_Diag:
            C0Data.diagPacket = nucIf->getNextDiagPacket();
            Serial.println("G-PD");
        default:
            break;
        }
    }
}

void setup1()
{
    memset(&C1Data, 0, sizeof(C1Data));
    lcdm = new LcdManager();
    lcdm->render(nullptr);
}

void loop1()
{
    // Signal a request for next data packet
    rp2040.fifo.push(1);
    // Wait until we get the signal back
    rp2040.fifo.pop();
    // Render the instructions
    lcdm->render(&C1Data);
}