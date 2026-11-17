#ifndef LED_MAN_H
#define LED_MAN_H

#include <Adafruit_NeoPixel.h>
#include "headers\led_array_layout.h"
#include "marshalled_data.h"

class LedManager
{
private:
    uint32_t red, orange, amber, yellow, green, blue, white, unknown, black;
    Adafruit_NeoPixel *neoPanel[2];

public:
    LedManager(uint32_t leftPanelPin, uint32_t rightPanelPin);
    void render(MarshalledData *data);
    void doSelfTest();
};

#endif // LED_MAN_H