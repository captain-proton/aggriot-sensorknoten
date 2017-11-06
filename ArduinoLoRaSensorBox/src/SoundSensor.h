/*
Measures the mean loudness with the use of an sound sensor until a reset.

Based on:

- Grove Sound Sensor
    http://wiki.seeed.cc/Grove-Sound_Sensor/
 */
#ifndef SoundSensor_H
#define SoundSensor_H

#include <Arduino.h>

class SoundSensor {
public:
    SoundSensor(uint8_t pin);
    void print();
    void loop();
    void reset();
    uint16_t getLoudness();
private:
    uint8_t _pin;
    float _loudness;
    uint32_t _n;
};

#endif
