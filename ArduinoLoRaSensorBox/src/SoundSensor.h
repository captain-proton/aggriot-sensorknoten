#ifndef SoundSensor_H
#define SoundSensor_H

#include <Arduino.h>

/**
* Measures the mean loudness with the use of an sound sensor until a reset. Base on [Grove Sound Sensor](http://wiki.seeed.cc/Grove-Sound_Sensor/).
* @param pin Pin that the sensor is connected to.
*/
class SoundSensor {
public:
    /** Create an instance of this sensor */
    SoundSensor(uint8_t pin);
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
    float _loudness;
    uint32_t _n;
};

#endif
