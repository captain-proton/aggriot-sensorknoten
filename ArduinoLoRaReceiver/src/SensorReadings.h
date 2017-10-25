#ifndef SensorReadings_H
#define SensorReadings_H

#include <Arduino.h>

class SensorReadings {
public:
    uint16_t temperature_f;
    uint16_t humidity_f;
    uint32_t dustConcentration_f;
    uint16_t lightSensorValue;
    uint16_t lightResistance;
    uint16_t loudnessMean;
    uint8_t floatNormalizer;
    uint32_t counter;
};

#endif
