#ifndef LCD_MAN_H
#define LCD_MAN_H

#include "OpenFontRender.h"
#include "NotoSansMonoBold.h"
#include "NotoSansBold.h"
#define TTF_FONT NOTO_SANS_BOLD
#define TTF_MONO_FONT NOTO_SANS_MONO_BOLD

#define DARKER_GREY 0x2965
#define REGEN_GREEN 0x3FE9
#define ACCEL_BLUE 0x047F
#define SPEED_ORANGE 0xFC43

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "marshalled_data.h"
#include "splash.h"

enum LcdState
{
    Initial,
    Splash,
    Gauges,
    Diag
};

class LcdManager
{
private:
    const float SlaMap[10] = {12.188f, 12.266f, 12.344f, 12.422f, 12.5f, 12.578f, 12.656f, 12.734f, 12.812f, 12.89f};
    const char *pushMessages[14] = {"", "Smoke detected - Evacuate now!", "Overheat detected - Evacuate now!", "MM Comms Failure", "MM Comms Restore", "Direction change not possible in motion", "Check direction lights", "Action blocked by E-Stop", "Brakes released", "Autostop Mode Started", "Autostop Mode Finished", "Autostop Marker Detected", "Regen Mode Started", "Regen Mode Finished"};

    TFT_eSPI tft; // Invoke custom library with default width and height
    static TFT_eSPI *glTft;
    TFT_eSprite spr = TFT_eSprite(&tft); // Declare Sprite object "spr" with pointer to "tft" object
    OpenFontRender ofr;                  // Use for noto sans
    OpenFontRender mofr;                 // Use for noto sans mono
    const float fsCapEnergy = 25000.0; // Joules

    uint32_t runTime = 0; // time for next update

    float speed = 0.0f; // Value to be displayed
    float current = 0.0f;
    float speed_inc = 0.1f, current_inc = 0.5f;

    void drawSpeedMeter(float val, bool fullDraw);

    void drawTractionMeter(float val, bool jrEn, bool fullDraw);

    void drawRegenBar(float capVolts, float charge, bool regenMode, bool fullDraw);

    void drawfuelBar(int percentage[3], float voltage[3], float busVoltage[3], bool fullDraw);

    void drawDsd(bool dsdActive, bool fullDraw);

    void drawBrake(bool mbr, bool bk1, bool bk2, bool bk3);

    void drawDiagMessage(const char *message);

    void drawMessage(const char *message);

    void drawSplash(const char *message, bool fullDraw);

    void drawDiagScreen(bool fullDraw, uint8_t flags, uint8_t live_check, uint16_t brake_level, uint16_t throttle);

public:
    LcdManager();
    void render(MarshalledData *data);
};

#endif