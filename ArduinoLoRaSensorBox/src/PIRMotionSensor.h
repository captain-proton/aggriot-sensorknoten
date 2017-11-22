#ifndef PIRMotionSensor_H
#define PIRMotionSensor_H

#include <Arduino.h>

/**
* Measures if people were detected until reset.
* @param pin Pin that the sensor is connected to.
*/
class PIRMotionSensor {
public:
    /** Create an instance of this sensor */
    PIRMotionSensor(uint8_t pin);
    void init();
    /** Print instance data onto the serial output */
    void print();
    /** Call this method on main program loop to measure data */
    void loop();
    /** Resets all measurement data of this instance. */
    void reset();
    /** Returns true if people were detected */
    bool isPeopleDetected();
private:
    uint8_t _pin;
    bool _detected;
};

#endif
