#ifndef SensorReadings_H
#define SensorReadings_H

#include <Arduino.h>

class SensorReadings {
public:
    uint32_t dustConcentration;
    uint8_t concentrationNormalizer;
    uint32_t counter;
};

#endif
