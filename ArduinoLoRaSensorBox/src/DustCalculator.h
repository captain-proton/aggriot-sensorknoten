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
