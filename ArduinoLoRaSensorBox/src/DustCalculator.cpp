#include <Arduino.h>
#include "DustCalculator.h"

DustCalculator::DustCalculator(uint32_t sampletimeMs, uint8_t srcPin) {
    _srcPin = srcPin;
    _sampleTimeMs = sampletimeMs;
    _concentration = 0;
    reset();
}

void DustCalculator::init() {

    pinMode(_srcPin, INPUT);
}

float DustCalculator::getConcentration() {

    return _concentration;
}

boolean DustCalculator::calculate() {

    // how long the pin had a low pulse block until it got HIGH (microseconds!)
    _duration = pulseIn(_srcPin, LOW);

    // add duration to the current occupancy
    _lowPulseOccupancy = _lowPulseOccupancy + _duration;

    // low pulse is measured until sample time is reached
    if ((millis() - _startTime) >= _sampleTimeMs) {
        // Integer percentage 0=>100
        _ratio = _lowPulseOccupancy / (_sampleTimeMs * 10.0);

        // using spec sheet curve
        float c = 1.1 * pow(_ratio, 3) - 3.8 * pow(_ratio, 2) + 520 * _ratio + 0.62;

        // calculate mean
        _n += 1;
        _concentration = _concentration * (_n - 1) / _n + c / _n;

        // reset values to start sampling again
        _lowPulseOccupancy = 0;
        _startTime = millis();
        return true;
    }
    return false;
}

void DustCalculator::print() {

    Serial.print("concentration = ");
    Serial.print(_concentration);
    Serial.println(" pcs/0.01cf");
}

void DustCalculator::reset() {

    _duration = 0;
    _startTime = millis();
    _lowPulseOccupancy = 0;
    _ratio = 0.0;
    _n = 0;
}
