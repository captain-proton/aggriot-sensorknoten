#ifndef SensorReadings_H
#define SensorReadings_H

#include <Arduino.h>

class SensorReadings {
public:
    uint32_t temperature;
    uint32_t humidity;
    uint32_t dustConcentration;
    uint16_t lightSensorValue;
    uint16_t lightResistance;
    uint8_t floatNormalizer;
    uint32_t counter;
};

#endif
