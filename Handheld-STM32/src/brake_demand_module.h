#ifndef BRAKE_DEMAND_MOD_H
#define BRAKE_DEMAND_MOD_H

#include <Arduino.h>
#include <cstdint>

/// @brief A class to manage implementing hysteresis for the force sensitive brake input. Assumes 16-bit ADC readings.
class BrakeDemandModule {

private:
    const uint16_t brakeUp[3] = {2300, 1850, 1600};
    const uint16_t brakeDown[3] = {2500, 2200, 1780};
    uint16_t readingCache;
    uint8_t lastBrakeDemand;
    uint32_t brakePin;
public:
    BrakeDemandModule(uint32_t BrakePin);

    uint8_t GetDemand();

    uint16_t GetCachedRaw();
};

#endif