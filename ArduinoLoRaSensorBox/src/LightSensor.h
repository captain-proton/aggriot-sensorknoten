/*

The light sensor value only reflects the approximated trend of the intensity
of light, it DOES NOT represent the exact Lumen.

http://wiki.seeedstudio.com/wiki/Grove_-_Light_Sensor_v1.2
 */
#ifndef LightSensor_H
#define LightSensor_H

#include <Arduino.h>

class LightSensor {
public:
    LightSensor(uint8_t pin);
    void print();
    void loop();
    void reset();
    uint16_t getSensorData();
    uint16_t getResistance();
private:
    uint8_t _pin;
    float _raw;
    float _rsensor;
    uint32_t _n;
};

#endif
