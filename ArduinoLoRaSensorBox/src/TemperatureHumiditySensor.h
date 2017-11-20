#ifndef TempHum_H
#define TempHum_H

#include <Arduino.h>
#include <dht.h>

/** Maximum count of errors that are allowed to occure in a row after measurement is aborted */
#define MAX_ERRORS      5

/**
* Measures the temperature and humidity. Based on [Grove Temperature and Humidity](http://wiki.seeed.cc/Grove-TemperatureAndHumidity_Sensor/) and [DHTstable library](https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable)
* @param pin Pin that the sensor is connected to.
*/
class TemperatureHumiditySensor {
public:
    /** Create an instance of this weather sensor */
    TemperatureHumiditySensor(uint8_t pin);
    /** Initialize this instance. Must be called after the instance was created for example on main::setup(). */
    void init();
    /**
     * Call this method on main program loop to measure data. An error is print on serial output, if no value could be calculated.
     * @return `true` if one value was calculated, `false` otherwise
     */
    boolean loop();
    /** Print instance data onto the serial output */
    void print();
    /** Resets all measurement data of this instance. */
    void reset();
    /** Returns the mean temperature in *C */
    float getTemperature();
    /** Returns the mean humidity in % from 0 to 100 */
    float getHumidity();
private:
    uint8_t _pin;
    dht *_dht;
    float _humidity;
    float _temperature;
    uint8_t _n;
    uint8_t _errors;
};

#endif
