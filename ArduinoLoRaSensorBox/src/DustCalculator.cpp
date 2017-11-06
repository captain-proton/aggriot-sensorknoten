#include "DustCalculator.h"

DustCalculator::DustCalculator(uint32_t sampletimeMs, uint8_t srcPin, uint8_t minCount, uint8_t capacity) {
    _srcPin = srcPin;
    _sampleTimeMs = sampletimeMs;
    _concentration = 0;
    _minCount = minCount;
    _capacity = capacity;
}

void DustCalculator::init() {

    pinMode(_srcPin, INPUT);

    _median = new RunningMedian(_capacity);

    _startTime = millis();
}

float DustCalculator::getConcentration() {

    return _median->getMedian();
}

boolean DustCalculator::loop() {

    // how long the pin had a low pulse block until it got HIGH (microseconds!)
    _duration = pulseIn(_srcPin, LOW);

    // add duration to the current occupancy
    _lowPulseOccupancy = _lowPulseOccupancy + _duration;

    // low pulse is measured until sample time is reached
    if ((millis() - _startTime) >= _sampleTimeMs) {
        // Integer percentage 0=>100
        _ratio = _lowPulseOccupancy / (_sampleTimeMs * 10.0);

        // using spec sheet curve
        _concentration = 1.1 * pow(_ratio, 3) - 3.8 * pow(_ratio, 2) + 520 * _ratio + 0.62;

        // Serial.println(_concentration);
        _median->add(_concentration);

        // reset values to start sampling again
        _lowPulseOccupancy = 0;
        _startTime = millis();
        return true;
    }
    return false;
}

boolean DustCalculator::isCalculated() {
    uint8_t count = _median->getCount();
    return count >= _minCount
        || count >= _capacity;
}

void DustCalculator::print() {

    Serial.print("concentration (median) = ");
    Serial.print(getConcentration());
    Serial.println(" pcs/0.01cf");
}

void DustCalculator::reset() {

    _duration = 0;
    _startTime = millis();
    _lowPulseOccupancy = 0;
    _ratio = 0.0;
    _median->clear();
}
