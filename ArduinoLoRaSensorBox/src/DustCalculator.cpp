#include <Arduino.h>
#include "DustCalculator.h"

DustCalculator::DustCalculator(uint32_t sampletimeMs) {
    _srcPin = 8;
    _sampleTimeMs = sampletimeMs;
    _duration = 0;
    _startTime = millis();
    _lowPulseOccupancy = 0;
    _ratio = 0.0;
}

float DustCalculator::getConcentration() {
    // Integer percentage 0=>100
    _ratio = _lowPulseOccupancy / (_sampleTimeMs * 10.0);

    // using spec sheet curve
    float concentration = 1.1 * pow(_ratio, 3) - 3.8 * pow(_ratio, 2) + 520 * _ratio + 0.62;

    return concentration;
}

boolean DustCalculator::isCalculated() {
    // how long the pin had a low pulse block until it got HIGH (microseconds!)
    _duration = pulseIn(_srcPin, LOW);

    // add duration to the current occupancy
    _lowPulseOccupancy = _lowPulseOccupancy + _duration;

    // low pulse is measured until sample time is reached
    if ((millis() - _startTime) >= _sampleTimeMs)
    {
        // reset values to start sampling again
        _lowPulseOccupancy = 0;
        _startTime = millis();
        return false;
    } else {
        return true;
    }
}

void DustCalculator::print() {

    Serial.print("lpo = ");
    Serial.print(_lowPulseOccupancy);

    Serial.print(" ratio = ");
    Serial.print(_ratio);

    Serial.print(" concentration = ");
    Serial.print(getConcentration());
    Serial.println(" pcs/0.01cf");
}
