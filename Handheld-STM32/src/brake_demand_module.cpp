#include "brake_demand_module.h"

BrakeDemandModule::BrakeDemandModule(uint32_t BrakePin)
{
    brakePin = BrakePin;
    lastBrakeDemand = 3;
    readingCache = 0;
};

uint8_t BrakeDemandModule::GetDemand()
{
    uint8_t prelimLevel;
    readingCache = analogRead(brakePin);
    // For safety, the default case should be brake level 3.
    // Therefore, start at the bottom and work our way up.
    if (readingCache > brakeDown[0] || readingCache > brakeUp[0] && lastBrakeDemand == 0)
    {
        prelimLevel = 0;
    }
    else if (readingCache > brakeDown[1] || readingCache > brakeUp[1] && lastBrakeDemand == 1)
    {
        prelimLevel = 1;
    }
    else if (readingCache > brakeDown[2] || readingCache > brakeUp[2] && lastBrakeDemand == 2)
    {
        prelimLevel = 2;
    }
    else
    {
        prelimLevel = 3;
    }
    lastBrakeDemand = prelimLevel;
    return prelimLevel;
}
uint16_t BrakeDemandModule::GetCachedRaw()
{
    return readingCache;
};