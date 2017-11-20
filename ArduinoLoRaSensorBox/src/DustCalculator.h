#ifndef DustCalculator_H
#define DustCalculator_H

#include <Arduino.h>
#include <RunningMedian.h>

/**
* A dust calculator can be used to measure dust concentration in the air.
* Based on [Groove dust sensor](http://wiki.seeed.cc/Grove-Dust_Sensor/) and [Grove - Dust Sensor Demo v1.0](https://github.com/Seeed-Studio/Grove_Dust_Sensor/blob/master/Grove_Dust_Sensor.ino)
* @param sampletimeMs time in millis how long a measurement cycle should last
* @param srcPin       pin where the sensor is connected
* @param minCount     minimum value count that must be measured to get a result
* @param capacity     maximum value count that should be calculated
*/
class DustCalculator {

public:
    /** Create an instance of this calculator. */
    DustCalculator(uint32_t sampletimeMs, uint8_t srcPin, uint8_t minCount, uint8_t capacity);
    /** Initialize this instance. Must be called after the instance was created for example on main::setup(). */
    void init();
    /**
     * Returns the median of all sampled data.
     * @return pcs/0.01cf
     */
    float getConcentration();
    /**
     * Call this method on main program loop to measure data
     * @return `true` if one value was calculated, `false` otherwise
     */
    void loop();
    /**
     * Check if a value can be retrieved with `DustCalculator::getConcentration`.
     * @return `true` if median can be returned.
     */
    boolean isCalculated();
    /** Print instance data onto the serial output */
    void print();
    /** Resets all measurement data of this instance. */
    void reset();
private:
    uint32_t _duration;
    uint32_t _startTime;
    uint32_t _lowPulseOccupancy;
    uint32_t _sampleTimeMs;
    uint8_t _srcPin;
    float _ratio;
    float _concentration;
    RunningMedian *_median;
    uint8_t _minCount;
    uint8_t _capacity;
};

#endif
