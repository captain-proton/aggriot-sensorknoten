#ifndef TemperatureSensor_H
#define TemperatureSensor_H

#include <Arduino.h>

/**
* Measures the mean temperature with the use of an temperature sensor until a reset. Base on [Grove Temperature Sensor](http://wiki.seeed.cc/Grove-Temperature_Sensor/).
* @param pin Pin that the sensor is connected to.
*/
class TemperatureSensor {
public:
    /** Create an instance of this sensor */
    TemperatureSensor(uint8_t pin);
    /** Print instance data onto the serial output */
    void print();
    /** Call this method on main program loop to measure data */
    void loop();
    /** Resets all measurement data of this instance. */
    void reset();
    /** Returns the mean loudness */
    float getTemperature();
private:
    uint8_t _pin;
    float _temperature;
    uint8_t _n;
};

#endif
