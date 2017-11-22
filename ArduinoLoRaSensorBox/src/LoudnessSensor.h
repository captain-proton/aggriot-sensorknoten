#ifndef LoudnessSensor_H
#define LoudnessSensor_H

#include <Arduino.h>

/**
* Measures the mean loudness with the use of an sound sensor until a reset. Base on [Grove Loudness Sensor](http://wiki.seeed.cc/Grove-Loudness_Sensor/).
* @param pin Pin that the sensor is connected to.
*/
class LoudnessSensor {
public:
    /** Create an instance of this sensor */
    LoudnessSensor(uint8_t pin);
    /** Print instance data onto the serial output */
    void print();
    /** Call this method on main program loop to measure data */
    void loop();
    /** Resets all measurement data of this instance. */
    void reset();
    /** Returns the mean loudness */
    uint16_t getLoudness();
private:
    uint8_t _pin;
    uint16_t _loudness;
    uint8_t _n;
};

#endif
