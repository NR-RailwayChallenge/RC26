#ifndef LED_MAN_H
#define LED_MAN_H

#include <Adafruit_NeoPixel.h>
#include "led_array_layout.h"
#include "marshalled_data.h"
#include "hardware/regs/vreg_and_chip_reset.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#define HUE_RED 0
#define HUE_ORANGE 5462
#define HUE_AMBER 8192
#define HUE_YELLOW 10923
#define HUE_GREEN 21845
#define HUE_BLUE 41943

class LedManager
{
private:
    uint32_t red, orange, amber, yellow, green, blue, white, unknown, black;
    Adafruit_NeoPixel *neoPanel[2];
    enum resetReason_t {UNKNOWN_RESET, PWRON_RESET, RUN_PIN_RESET, SOFT_RESET, WDT_RESET};

public:
    LedManager(uint32_t leftPanelPin, uint32_t rightPanelPin);
    void render(MarshalledData *data);
    void doSelfTest();
};

#endif // LED_MAN_H