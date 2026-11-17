#ifndef ROLL_AVE_VOLT_H
#define ROLL_AVE_VOLT_H

#define SAMPLE_HISTORY 10

#include <Arduino.h>

class RollingAverageAnalogIn
{

private:
    uint16_t sample_history[SAMPLE_HISTORY] = {0};
    int bufferPtr = 0;
    float mul;
    uint32_t analogPin = -1;

    uint16_t rawToVolt(uint16_t raw)
    {
        float volt = raw * mul;
        return volt * 1e3;
    }

public:
    uint16_t read()
    {
        // Read the raw sample
        uint16_t sample = analogRead(analogPin);
        // Add to the buffer
        sample_history[bufferPtr] = sample;
        // Increment the buffer pointer with wrap around
        bufferPtr = (bufferPtr + 1) % SAMPLE_HISTORY;
        uint32_t accumulator = 0;
        for (int i = 0; i < SAMPLE_HISTORY; i++)
        {
            accumulator += sample_history[i];
        }
        // Get the geometric mean
        accumulator /= SAMPLE_HISTORY;

        return rawToVolt(accumulator);
    }

    RollingAverageAnalogIn(uint32_t pinNum, uint32_t r1, uint32_t r2)
    {
        float ratio = (float)r1 / (r1 + r2);
        mul = 3.3f / (4095 * ratio);
        analogPin = pinNum;
    }
};

#endif // ROLL_AVE_VOLT_H