#include "led_man.h"

LedManager::LedManager(uint32_t leftPanelPin, uint32_t rightPanelPin)
{
    neoPanel[0] = new Adafruit_NeoPixel(6, leftPanelPin, NEO_GRB + NEO_KHZ800);
    neoPanel[1] = new Adafruit_NeoPixel(6, rightPanelPin, NEO_GRB + NEO_KHZ800);
    neoPanel[0]->begin();
    neoPanel[0]->setBrightness(NEOPIX_BRIGHTNESS);
    neoPanel[0]->show();
    neoPanel[1]->begin();
    neoPanel[1]->setBrightness(NEOPIX_BRIGHTNESS);
    neoPanel[1]->show();

    // Pre-calc static colours
    red = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(0, 255, 255));
    orange = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(5462, 255, 255));
    amber = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(8192, 255, 255));
    yellow = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(10923, 255, 255));
    green = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(21845, 255, 255));
    blue = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(41943, 255, 255));
    white = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(255, 255, 255));
    unknown = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(50, 50, 50));
    black = Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(0, 0, 0));
}

void LedManager::render(MarshalledData *data)
{
    unsigned char pulse_mul = (sin(0.008377580409573 * millis()) * 127) + 127;
    if (!(data->diagMode))
    {
        if ((data->packet.flags >> PICO_PACK_FLAG_CAN_STAT) & 1)
        {
            neoPanel[LED_POWER_PANEL]->setPixelColor(LED_POWER_INDEX, blue);
        }
        else
        {
            neoPanel[0]->fill(unknown);
            neoPanel[1]->fill(unknown);
            neoPanel[LED_POWER_PANEL]->setPixelColor(LED_POWER_INDEX, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(41943, 255, pulse_mul)));
            neoPanel[0]->show();
            neoPanel[1]->show();
            return;
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_DIR) & 1)
        {
            // Reverse direction (DIR flag set)
            neoPanel[LED_DIR_FWD_PANEL]->setPixelColor(LED_DIR_FWD_INDEX, black);
            neoPanel[LED_DIR_REV_PANEL]->setPixelColor(LED_DIR_REV_INDEX, white);
        }
        else
        {
            // Forward or normal direction (DIR flag not set)
            neoPanel[LED_DIR_FWD_PANEL]->setPixelColor(LED_DIR_FWD_INDEX, white);
            neoPanel[LED_DIR_REV_PANEL]->setPixelColor(LED_DIR_REV_INDEX, black);
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_JR_EN) & 1)
        {
            neoPanel[LED_JR_EN_PANEL]->setPixelColor(LED_JR_EN_INDEX, green);
        }
        else
        {
            neoPanel[LED_JR_EN_PANEL]->setPixelColor(LED_JR_EN_INDEX, red);
        }

        if (data->packet.tractionCurrent > 5.0f)
        {
            neoPanel[LED_TEI_FWD_PANEL]->setPixelColor(LED_TEI_FWD_INDEX, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(21845 * (1.0f - (data->packet.tractionCurrent / 200.0f)), 255, 255)));
            neoPanel[LED_TEI_REV_PANEL]->setPixelColor(LED_TEI_REV_INDEX, black);
        }
        else if (data->packet.tractionCurrent < -5.0f)
        {
            neoPanel[LED_TEI_REV_PANEL]->setPixelColor(LED_TEI_REV_INDEX, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(21845 * (1.0f - (data->packet.tractionCurrent / -200.0f)), 255, 255)));
            neoPanel[LED_TEI_FWD_PANEL]->setPixelColor(LED_TEI_FWD_INDEX, black);
        }
        else
        {
            neoPanel[LED_TEI_REV_PANEL]->setPixelColor(LED_TEI_REV_INDEX, black);
            neoPanel[LED_TEI_FWD_PANEL]->setPixelColor(LED_TEI_FWD_INDEX, black);
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_ES) & 1)
        {
            neoPanel[LED_ES_PANEL]->setPixelColor(LED_ES_INDEX, red);
        }
        else
        {
            neoPanel[LED_ES_PANEL]->setPixelColor(LED_ES_INDEX, black);
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_RG_EN) & 1)
        {
            neoPanel[LED_REGEN_PANEL]->setPixelColor(LED_REGEN_INDEX, blue);
        }
        else
        {
            neoPanel[LED_REGEN_PANEL]->setPixelColor(LED_REGEN_INDEX, black);
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_AS_EN) & 1)
        {
            if ((data->packet.flags >> PICO_PACK_FLAG_AS_MD & 1))
            {
                neoPanel[LED_AS_PANEL]->setPixelColor(LED_AS_INDEX, red);
            }
            else
            {
                neoPanel[LED_AS_PANEL]->setPixelColor(LED_AS_INDEX, amber);
            }
        }
        else
        {
            neoPanel[LED_AS_PANEL]->setPixelColor(LED_AS_INDEX, black);
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_FIRE_S) & 1 || (data->packet.flags >> PICO_PACK_FLAG_FIRE_H) & 1)
        {
            neoPanel[LED_FIRE_PANEL]->setPixelColor(LED_FIRE_INDEX, red);
        }
        else
        {
            neoPanel[LED_FIRE_PANEL]->setPixelColor(LED_FIRE_INDEX, black);
        }

        if ((data->packet.flags >> PICO_PACK_FLAG_BF) & 1)
        {
            neoPanel[LED_BF_PANEL]->setPixelColor(LED_BF_INDEX, red);
        }
        else
        {
            neoPanel[LED_BF_PANEL]->setPixelColor(LED_BF_INDEX, black);
        }
        if ((data->packet.flags >> PICO_PACK_FLAG_BK_SL) & 1)
        {
            neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(0, 255, pulse_mul)));
        }
        else if ((data->packet.flags >> PICO_PACK_FLAG_BK_MBR) & 1)
        {
            neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, red);
        }
        else
        {
            int num_brakes = 0;
            num_brakes += ((data->packet.flags >> PICO_PACK_FLAG_BK_BR1) & 1);
            num_brakes += ((data->packet.flags >> PICO_PACK_FLAG_BK_BR2) & 1);
            num_brakes += ((data->packet.flags >> PICO_PACK_FLAG_BK_BR3) & 1);
            switch (num_brakes)
            {
            case 1:
                neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, yellow);
                break;
            case 2:
                neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, orange);
                break;
            case 3:
                neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, red);
                break;

            default:
                neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, black);
                break;
            }
        }
    }
    else
    {
        // Diag mode
        neoPanel[0]->fill(unknown);
        neoPanel[1]->fill(unknown);
        // Orange power light
        neoPanel[LED_POWER_PANEL]->setPixelColor(LED_POWER_INDEX, orange);
        // Auto stop
        neoPanel[LED_AS_PANEL]->setPixelColor(LED_AS_INDEX, ((data->diagPacket.flags >> PICO_DIAG_FLAG_AS) & 1) ? green : red);
        // Brake release
        neoPanel[LED_ES_PANEL]->setPixelColor(LED_ES_INDEX, ((data->diagPacket.flags >> PICO_DIAG_FLAG_BR) & 1) ? green : red);
        // Change direction
        neoPanel[LED_DIR_FWD_PANEL]->setPixelColor(LED_DIR_FWD_INDEX, ((data->diagPacket.flags >> PICO_DIAG_FLAG_CD) & 1) ? green : red);
        // Horn
        neoPanel[LED_FIRE_PANEL]->setPixelColor(LED_FIRE_INDEX, ((data->diagPacket.flags >> PICO_DIAG_FLAG_HR) & 1) ? green : red);
        // Regen
        neoPanel[LED_REGEN_PANEL]->setPixelColor(LED_REGEN_INDEX, ((data->diagPacket.flags >> PICO_DIAG_FLAG_RG) & 1) ? green : red);
        // Brake Demand
        switch (data->diagPacket.flags & 0x03)
        {
        case 1:
            neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, yellow);
            break;
        case 2:
            neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, orange);
            break;
        case 3:
            neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, red);
            break;
        default:
            neoPanel[LED_BRAKE_PANEL]->setPixelColor(LED_BRAKE_INDEX, white);
            break;
        }
    }
    neoPanel[0]->show();
    neoPanel[1]->show();
}

void LedManager::doSelfTest()
{
    const int fr_time = 80;
    const int wait_time = 500;
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(red, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(green, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(black, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(black, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(green, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(blue, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(black, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(black, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(red, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(green, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(black, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(black, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(blue, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(red, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(black, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(black, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(white, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(white, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
    for (int i = 1; i <= 6; i++)
    {
        neoPanel[0]->fill(black, 0, i);
        neoPanel[0]->show();
        neoPanel[1]->fill(black, 0, i);
        neoPanel[1]->show();
        delay(fr_time);
    }
    delay(wait_time);
}