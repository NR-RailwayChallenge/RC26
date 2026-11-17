#include "lcd_man.h"

LcdManager::LcdManager()
{
    // Initialise the LCD
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    if (ofr.loadFont(TTF_FONT, sizeof(TTF_FONT)) || mofr.loadFont(TTF_MONO_FONT, sizeof(TTF_MONO_FONT)))
    {
        Serial.println("Renderer initialize error");
        return;
    }
}

void LcdManager::drawSpeedMeter(float val, bool fullDraw)
{
    static uint16_t last_angle = 30;
    int x = 60, y = 65, r = 54;

    if (fullDraw)
    {
        last_angle = 30;
        tft.fillCircle(x, y, r, DARKER_GREY);
        tft.drawSmoothCircle(x, y, r, TFT_SILVER, DARKER_GREY);
        uint16_t tmp = r - 3;
        tft.drawArc(x, y, tmp, tmp - tmp / 5, last_angle, 330, TFT_BLACK, DARKER_GREY);
    }

    r -= 3;

    // Clip and scale the value to the arc angle
    float val_clip = val < 0 ? 0 : (val > 16.0f ? 16.0f : val);
    int val_angle = ((int)(val_clip * 18.75f)) + 30;

    if (last_angle != val_angle || fullDraw)
    {
        mofr.setDrawer(spr); // Link renderer to sprite (font will be rendered in sprite spr)

        // Add value in centre if radius is a reasonable size
        if (r >= 25)
        {
            // This code gets the font dimensions in pixels to determine the required the sprite size
            mofr.setFontSize((3 * r) / 5);
            mofr.setFontColor(TFT_WHITE, DARKER_GREY);
            uint8_t w = mofr.getTextWidth("44.4");
            uint8_t h = mofr.getTextHeight("4") + 4;
            spr.createSprite(w, h + 2);
            spr.fillSprite(DARKER_GREY);
            char str_buf[8]; // Bufferfor string
            sprintf(str_buf, "%.1f", val);
            mofr.setCursor(w / 2, -h / 2.5);
            mofr.cprintf(str_buf);
            spr.pushSprite(x - w / 2, y - h / 2); // Push sprite containing the val number
            spr.deleteSprite();                   // Recover used memory

            // Make the TFT the print destination, print the units label direct to the TFT
            ofr.setDrawer(tft);
            ofr.setFontColor(TFT_GOLD, DARKER_GREY);
            ofr.setFontSize(r / 2.0);
            ofr.setCursor(x, y + (r * 0.4));
            ofr.cprintf("km/h");
        }

        // Allocate a value to the arc thickness dependant of radius
        uint8_t thickness = r / 5;
        if (r < 25)
            thickness = r / 3;

        // Update the arc, only the zone between last_angle and new val_angle is updated
        if (val_angle > last_angle)
        {
            tft.drawArc(x, y, r, r - thickness, last_angle, val_angle, SPEED_ORANGE, TFT_BLACK); // TFT_SKYBLUE random(0x10000)
        }
        else
        {
            tft.drawArc(x, y, r, r - thickness, val_angle, last_angle, TFT_BLACK, DARKER_GREY);
        }
        last_angle = val_angle; // Store meter arc position for next redraw
    }
}

void LcdManager::drawTractionMeter(float val, bool jrEn, bool fullDraw)
{
    static uint16_t last_angle_left = 180, last_angle_right = 180;
    static bool lastJrEn = false;
    int x = 180, y = 65, r = 54;
    float fs = 200.0f;

    if (fullDraw)
    {
        last_angle_left = 180;
        last_angle_right = 180;
        tft.fillCircle(x, y, r, DARKER_GREY);
        tft.drawSmoothCircle(x, y, r, TFT_SILVER, DARKER_GREY);
        uint16_t tmp = r - 3;
        // Redraw the track
        tft.drawArc(x, y, tmp, tmp - tmp / 5, 30, 330, TFT_BLACK, DARKER_GREY);
        // Make the TFT the print destination, print the units label direct to the TFT
        ofr.setDrawer(tft);
        ofr.setFontColor(TFT_GOLD, DARKER_GREY);
        ofr.setFontSize(r / 2.0);
        ofr.setCursor(x, y + (r * 0.4));
        ofr.cprintf("Amps");
    }

    r -= 3;

    float val_clip = val < -fs ? -fs : (val > fs ? fs : val);
    int val_angle_left = val_clip < 0 ? 180 + (val / fs) * 150 : 180;
    int val_angle_right = val_clip > 0 ? 180 + (val / fs) * 150 : 180;

    // This code gets the font dimensions in pixels to determine the required the sprite size
    mofr.setFontSize(r / 2);
    mofr.setFontColor(TFT_WHITE, DARKER_GREY);
    uint8_t w = mofr.getTextWidth("444.4");
    uint8_t h = mofr.getTextHeight("4") + 4;
    spr.createSprite(w, h + 2);
    spr.fillSprite(DARKER_GREY);
    char str_buf[8]; // Bufferfor string
    sprintf(str_buf, "%.1f", abs(val));
    mofr.setCursor(w / 2, -h / 2.5);
    mofr.cprintf(str_buf);
    spr.pushSprite(x - w / 2, y - h / 2); // Push sprite containing the val number
    spr.deleteSprite();                   // Recover used memory

    if (last_angle_left != val_angle_left || last_angle_right != val_angle_right || fullDraw)
    {
        mofr.setDrawer(spr); // Link renderer to sprite (font will be rendered in sprite spr)

        // Allocate a value to the arc thickness dependant of radius
        uint8_t thickness = r / 5;
        if (r < 25)
            thickness = r / 3;

        // Update the arc, only the zone between last_angle and new val_angle is updated
        if (val_angle_right > last_angle_right)
        {
            tft.drawArc(x, y, r, r - thickness, last_angle_right, val_angle_right, ACCEL_BLUE, TFT_BLACK);
        }
        else
        {
            tft.drawArc(x, y, r, r - thickness, val_angle_right, last_angle_right, TFT_BLACK, DARKER_GREY);
        }
        last_angle_right = val_angle_right; // Store meter arc position for next redraw

        // Update the arc, only the zone between last_angle and new val_angle is updated
        if (val_angle_left < last_angle_left)
        {
            tft.drawArc(x, y, r, r - thickness, val_angle_left, last_angle_left, REGEN_GREEN, TFT_BLACK);
        }
        else
        {
            tft.drawArc(x, y, r, r - thickness, last_angle_left, val_angle_left, TFT_BLACK, DARKER_GREY);
        }
        last_angle_left = val_angle_left; // Store meter arc position for next redraw
    }

    if (lastJrEn != jrEn || fullDraw)
    {
        lastJrEn = jrEn;
        tft.drawSmoothArc(x, y, r, r - 2, 348, 12, jrEn ? REGEN_GREEN : TFT_RED, DARKER_GREY);
    }
}

void LcdManager::drawRegenBar(float capVolts, float capEnergy, bool regenMode, bool fullDraw)
{
    if (fullDraw)
    {
        tft.drawFastHLine(0, 125, 240, TFT_WHITE);
        tft.drawFastHLine(0, 168, 240, TFT_WHITE);
        tft.fillRoundRect(0, 126, 240, 42, 0, DARKER_GREY);
        ofr.setDrawer(tft);
        ofr.setCursor(5, 126);
        ofr.setFontColor(TFT_GOLD);
        ofr.setBackgroundColor(DARKER_GREY);
        ofr.setFontSize(25);
        ofr.printf("Regen");
    }
    spr.createSprite(184, 22);
    spr.fillSprite(DARKER_GREY);

    mofr.setDrawer(spr);
    mofr.setFontSize(15);
    mofr.setCursor(104, 1);
    mofr.printf("(%.2f V)", capVolts);
    mofr.setCursor(92, -1);
    mofr.setFontColor(TFT_WHITE);
    mofr.setFontSize(20);
    if (regenMode)
    {
        mofr.rprintf("%.1f kJ", capEnergy/1000.0f);
    }
    else
    {
        mofr.rprintf("--.- kJ");
    }

    spr.pushSprite(56, 127); // Push sprite containing the val number
    spr.deleteSprite();
    tft.drawWideLine(10, 160, 230, 160, 4, regenMode ? TFT_BLACK : DARKER_GREY, DARKER_GREY);
    if (regenMode)
    {
        float bar_w = 220 * (capEnergy > fsCapEnergy ? 1.0f : (capEnergy < 0 ? 0.0f : capEnergy / fsCapEnergy));
        if (bar_w > 4.0f)
        {
            tft.drawWideLine(10, 160, bar_w, 160, 4, ACCEL_BLUE, DARKER_GREY);
        }
    }
}

void LcdManager::drawfuelBar(int percentage[3], float voltage[3], float busVoltage[3], bool fullDraw)
{
    if (fullDraw)
    {
        spr.createSprite(240, 19);
        spr.fillSprite(DARKER_GREY);
        ofr.setDrawer(spr);
        ofr.setFontColor(TFT_GOLD);
        ofr.setBackgroundColor(DARKER_GREY);
        ofr.setFontSize(25);
        for (int i = 0; i < 3; i++)
        {
            ofr.setCursor(40 + (80 * i), 0);
            ofr.cprintf("Battery %d", i + 1);
        }
        spr.pushSprite(0, 208);
        spr.deleteSprite();

        spr.createSprite(240, 19);
        spr.fillSprite(DARKER_GREY);
        ofr.setDrawer(spr);
        ofr.setFontColor(TFT_GOLD);
        ofr.setBackgroundColor(DARKER_GREY);
        ofr.setFontSize(25);
        for (int i = 0; i < 3; i++)
        {
            ofr.setCursor(40 + (80 * i), 0);
            ofr.cprintf("%d V Bus", (i + 1) * 12);
        }
        spr.pushSprite(0, 169);
        spr.deleteSprite();
    }
    spr.createSprite(240, 28);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    for (int i = 0; i < 3; i++)
    {
        //uint8_t percent = percentage[i] > 100 ? 100 : percentage[i];
        mofr.setCursor(40 + (80 * i), -3);
        mofr.setFontSize(15);
        mofr.cprintf("%.3f V", voltage[i]);
        //spr.drawWideLine(5 + (80 * i), 21, 75 + (80 * i), 21, 4, TFT_BLACK, DARKER_GREY);
        //if (percent > 5)
        //{
        //    spr.drawWideLine(5 + (80 * i), 21, ((percent * 0.75f)) + (80 * i), 21, 4, REGEN_GREEN, DARKER_GREY);
        //}
    }
    spr.pushSprite(0, 227);
    spr.deleteSprite();
    tft.drawFastVLine(80, 169, 86, TFT_WHITE);
    tft.drawFastVLine(160, 169, 86, TFT_WHITE);
    spr.createSprite(240, 21);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    for (int i = 0; i < 3; i++)
    {
        mofr.setCursor(40 + (80 * i), -1);
        mofr.setFontSize(15);
        mofr.cprintf("%.3f V", busVoltage[i]);
    }
    spr.pushSprite(0, 187);
    spr.deleteSprite();

    tft.drawFastHLine(0, 208, 240, TFT_WHITE);
    tft.drawFastHLine(0, 255, 240, TFT_WHITE);
    tft.drawFastVLine(80, 169, 86, TFT_WHITE);
    tft.drawFastVLine(160, 169, 86, TFT_WHITE);
}

void LcdManager::drawDsd(bool dsdActive, bool fullDraw)
{
    if (fullDraw)
    {
        ofr.setDrawer(tft);
        ofr.setCursor(120, 4);
        ofr.setFontColor(TFT_WHITE);
        ofr.setBackgroundColor(TFT_BLACK);
        ofr.setFontSize(25);
        ofr.cprintf("DSD");
    }
    tft.drawWideLine(110, 26, 130, 26, 4, dsdActive ? REGEN_GREEN : TFT_RED, DARKER_GREY);
}

void LcdManager::drawBrake(bool mbr, bool bk1, bool bk2, bool bk3)
{
    ofr.setDrawer(tft);
    ofr.setCursor(120, 98);
    ofr.setBackgroundColor(TFT_BLACK);
    ofr.setFontColor(mbr || bk1 || bk2 || bk3 ? (mbr ? TFT_RED : TFT_WHITE) : TFT_BLACK);
    ofr.setFontSize(25);
    ofr.cprintf("Brake");
    tft.drawWideLine(90, 120, 104, 120, 4, bk1 ? TFT_RED : TFT_BLACK, TFT_BLACK);
    tft.drawWideLine(113, 120, 127, 120, 4, bk2 ? TFT_RED : TFT_BLACK, TFT_BLACK);
    tft.drawWideLine(136, 120, 150, 120, 4, bk3 ? TFT_RED : TFT_BLACK, TFT_BLACK);
}

void LcdManager::drawDiagMessage(const char *message)
{
    spr.createSprite(240, 20);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setCursor(2, 1);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    mofr.setFontSize(14);
    mofr.printf(message);
    spr.pushSprite(0, 256);
    spr.deleteSprite();
    tft.drawFastHLine(0, 276, 240, TFT_WHITE);
}

void LcdManager::drawMessage(const char *message)
{
    spr.createSprite(240, 320 - 277);
    spr.fillSprite(TFT_BLACK);
    ofr.setDrawer(spr);
    ofr.setCursor(10, 0);
    ofr.setFontColor(TFT_WHITE);
    ofr.setBackgroundColor(TFT_BLACK);
    ofr.setFontSize(30);
    ofr.setLineSpaceRatio(0.75);
    ofr.printf(message);
    spr.pushSprite(0, 277);
    spr.deleteSprite();
}

void LcdManager::drawSplash(const char *message, bool fullDraw)
{
    if (fullDraw)
    {
        bool swap = tft.getSwapBytes();
        tft.setSwapBytes(true);
        tft.pushImage(0, 32, splashWidth, splashHeight, splashImg);
        tft.setSwapBytes(swap);
    }
    spr.createSprite(240, 20);
    spr.fillSprite(TFT_BLACK);
    mofr.setDrawer(spr);
    mofr.setCursor(120, 1);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(TFT_BLACK);
    mofr.setFontSize(14);
    mofr.cprintf(message);
    spr.pushSprite(0, 256);
    spr.deleteSprite();
}

void LcdManager::render(MarshalledData *data)
{
    unsigned long start = micros();
    static LcdState lastState = LcdState::Initial;
    LcdState currentState;
    bool fullDrawRequired = false;
    // Determine which state/screen to draw
    if (data == nullptr)
    {
        currentState = LcdState::Splash;
    }
    else if (data->diagMode)
    {
        currentState = LcdState::Diag;
    }
    else if (!(data->packet.flags & 1 << PICO_PACK_FLAG_CAN_STAT))
    {
        currentState = LcdState::Splash;
    }
    else
    {
        currentState = LcdState::Gauges;
    }

    if (currentState != lastState)
    {
        fullDrawRequired = true;
        tft.fillScreen(TFT_BLACK);
    }
    if (currentState == LcdState::Gauges)
    {
        float f_sp = fabsf(data->packet.speed);
        if(f_sp <=20.0f){
            drawSpeedMeter(f_sp, fullDrawRequired);
        }
        if(fabsf(data->packet.tractionCurrent) < 250.0f){
            drawTractionMeter(data->packet.tractionCurrent, (data->packet.flags >> PICO_PACK_FLAG_KSR) & 1, fullDrawRequired);
        }
        drawRegenBar(data->packet.buscap / 1000.0f, data->packet.regenEnergy, (data->packet.flags >> PICO_PACK_FLAG_RG_EN) & 1, fullDrawRequired);
        // Fuel bar
        int percentages[3];
        float voltages[3];
        float busVoltages[3];
        busVoltages[0] = fabsf(data->packet.bus12 / 1000.0f);
        busVoltages[1] = fabsf(data->packet.bus24 / 1000.0f);
        busVoltages[2] = fabsf(data->packet.bus36 / 1000.0f);
        for (int i = 0; i < 3; i++)
        {
            voltages[i] = busVoltages[i] - ((i < 1) ? 0 : busVoltages[i - 1]);
            /*for (int j = 9; j >= 0; j--)
            {
                if (voltages[i] > SlaMap[j])
                {
                    percentages[i] = (j + 1) * 10;
                    break;
                }
            }*/
        }
        drawfuelBar(percentages, voltages, busVoltages, fullDrawRequired);
        drawDsd((data->packet.flags >> PICO_PACK_FLAG_DSD_STAT) & 1, fullDrawRequired);
        drawBrake((data->packet.flags >> PICO_PACK_FLAG_BK_MBR) & 1, (data->packet.flags >> PICO_PACK_FLAG_BK_BR1) & 1, (data->packet.flags >> PICO_PACK_FLAG_BK_BR2) & 1, (data->packet.flags >> PICO_PACK_FLAG_BK_BR3) & 1);
        // #TODO: fix
        // char mbuf[24];
        // sprintf(mbuf, "VESC Voltage: %.3f V", data->packet.busTrac);
        // drawDiagMessage(mbuf);
        // drawMessage(pushMessages[data->packet.message]);
    }
    else if (currentState == LcdState::Splash)
    {
        if (data != nullptr && !((data->packet.flags >> PICO_PACK_FLAG_CAN_STAT) & 1))
        {
            drawSplash("CAN Bus connecting...", fullDrawRequired);
        }
        else
        {
            drawSplash("Starting up...", fullDrawRequired);
        }
    }
    else if (currentState == LcdState::Diag)
    {
        drawDiagScreen(fullDrawRequired, data->diagPacket.flags, data->diagPacket.rand, data->diagPacket.brakeAnalogue, data->diagPacket.throttleAnalogue);
    }
    lastState = currentState;
}

void LcdManager::drawDiagScreen(bool fullDraw, uint8_t flags, uint8_t live_check, uint16_t brake_level, uint16_t throttle)
{
    const char *diagStrings[6] = {"Autostop", "Auto Coupler", "Change Direction", "Driver Safety Device (HE Relay)", "Horn", "Regen"};
    uint8_t diagIdx[6] = {PICO_DIAG_FLAG_AS, PICO_DIAG_FLAG_AC, PICO_DIAG_FLAG_CD, PICO_DIAG_FLAG_DSD, PICO_DIAG_FLAG_HR, PICO_DIAG_FLAG_RG};

    if (fullDraw)
    {
        ofr.setDrawer(tft);
        ofr.setCursor(120, 2);
        ofr.setFontColor(TFT_WHITE);
        ofr.setBackgroundColor(TFT_BLACK);
        ofr.setFontSize(30);
        ofr.cprintf("SIM Tool Diagnostics Page");
        ofr.setCursor(10, 35);
        ofr.setFontSize(25);
        ofr.setFontColor(TFT_YELLOW);
        ofr.setLineSpaceRatio(0.75);
        ofr.printf("CAUTION: The buttons are still live.\nPress the emergency stop button\nbefore using these diagnostics.");
        ofr.setFontColor(TFT_WHITE);

        ofr.setFontSize(25);
        ofr.setCursor(10, 190);
        ofr.printf("Brake: ");
        ofr.setCursor(10, 215);
        ofr.printf("Throttle: ");
        ofr.setCursor(10, 240);
        ofr.printf("CAN fails: ");

        ofr.setCursor(10, 265);
        ofr.printf("Live Check: ");
    }

    ofr.setFontSize(25);
    for (int i = 0; i < 6; i++)
    {
        tft.fillSmoothCircle(15, 98 + (i * 15), 5, ((flags >> diagIdx[i]) & 1) ? REGEN_GREEN : TFT_RED, TFT_BLACK);
        if (fullDraw)
        {
            ofr.setCursor(25, 88 + (i * 15));
            ofr.printf(diagStrings[i]);
        }
    }

    spr.createSprite(135, 20);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setCursor(2, 0);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    mofr.setFontSize(14);
    mofr.printf("L%1u (%4u/0x%04X)", flags & 0x03, brake_level, brake_level);
    spr.pushSprite(53, 190);
    spr.deleteSprite();

    spr.createSprite(160, 20);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setCursor(2, 0);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    mofr.setFontSize(14);
    // mofr.printf("DISABLED");
    mofr.printf("%u (4095/0x0FFF)",throttle);
    spr.pushSprite(67, 215);
    spr.deleteSprite();

    spr.createSprite(98, 20);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setCursor(2, 0);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    mofr.setFontSize(14);
    mofr.printf("NO INFO");
    // mofr.printf("0000T/0000R");
    spr.pushSprite(72, 240);
    spr.deleteSprite();

    spr.createSprite(45, 20);
    spr.fillSprite(DARKER_GREY);
    mofr.setDrawer(spr);
    mofr.setCursor(2, 0);
    mofr.setFontColor(TFT_WHITE);
    mofr.setBackgroundColor(DARKER_GREY);
    mofr.setFontSize(14);
    mofr.printf("0x%02X", live_check);
    spr.pushSprite(85, 265);
    spr.deleteSprite();
}