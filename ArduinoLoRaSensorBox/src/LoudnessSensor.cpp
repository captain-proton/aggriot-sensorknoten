#include <Arduino.h>
#include "LoudnessSensor.h"

LoudnessSensor::LoudnessSensor(uint8_t pin) {
    _pin = pin;
    _n = 0;
    _loudness = 0;
}

void LoudnessSensor::print() {

    Serial.print(F("Loudness: "));
    Serial.println(_loudness);
}

void LoudnessSensor::loop() {
    uint16_t value = analogRead(_pin);
    /*
                   (n - 1)     new_value
    current mean * -------  +  ---------
                      n            n
     */
     _n += 1;
    _loudness = (_loudness * (_n - 1) / _n) + (value * 1.0 / _n);
}

void LoudnessSensor::reset() {
    _n = 0;
}

uint16_t LoudnessSensor::getLoudness() {
    return _loudness;
}
