#ifndef SensorReadings_H
#define SensorReadings_H

#include <Arduino.h>

typedef struct {

    uint16_t temperature_f;
    uint16_t humidity_f;
    uint32_t dustConcentration_f;
    uint16_t lightSensorValue;
    uint16_t lightResistance;
    uint16_t loudness;
    uint8_t floatNormalizer;
} SensorData;

class SensorReadings {
public:

    SensorData data;

    void serialize(uint8_t *dst);
    void deserialize(uint8_t *src, uint8_t size);
    uint8_t size();
    void reset();
    void print();
};

#endif
