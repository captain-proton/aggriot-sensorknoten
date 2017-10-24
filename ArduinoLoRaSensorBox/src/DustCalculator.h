/*
A dust calculator has to be initialized with a sample time that is used
to calculate polution and the source pin that the sensor is connected on.

Based on:
- Seeed Wiki
    http://wiki.seeed.cc/Grove-Dust_Sensor/
- Grove - Dust Sensor Demo v1.0
    https://github.com/Seeed-Studio/Grove_Dust_Sensor/blob/master/Grove_Dust_Sensor.ino
 */
#ifndef DustCalculator_H
#define DustCalculator_H

#include <Arduino.h>

class DustCalculator {

public:
    DustCalculator(uint32_t sampletimeMs, uint8_t srcPin);
    void init();
    float getConcentration();
    boolean calculate();
    void print();
private:
    uint32_t _duration;
    uint32_t _startTime;
    uint32_t _lowPulseOccupancy;
    uint32_t _sampleTimeMs;
    uint8_t _srcPin;
    float _ratio;
    float _concentration;
};

#endif
