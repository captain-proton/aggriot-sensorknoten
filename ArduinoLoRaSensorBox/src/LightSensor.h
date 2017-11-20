#ifndef LightSensor_H
#define LightSensor_H

#include <Arduino.h>

/**
* Light sensor integrates a photo-resistor(light dependent resistor) to detect the intensity of light. The sensor value only reflects the approximated trend of the intensity of light, it DOES NOT represent the exact Lumen. Based on [Groove Light Sensor](http://wiki.seeedstudio.com/wiki/Grove_-_Light_Sensor_v1.2). Use the calculated resistance value of this sensor if you want to check light incidence.
* @param pin Pin that the sensor is connected to.
*/
class LightSensor {
public:
    /** Create an instance of this sensor. */
    LightSensor(uint8_t pin);
    /** Print instance data onto the serial output */
    void print();
    /** Call this method on main program loop to measure data */
    void loop();
    /** Resets all measurement data of this instance. */
    void reset();
    /** Returns the mean of raw sensor data */
    uint16_t getSensorData();
    /** Returns the calculated resistance of the sensor. */
    uint16_t getResistance();
private:
    uint8_t _pin;
    float _raw;
    float _rsensor;
    uint8_t _n;
};

#endif
