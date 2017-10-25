#include <Arduino.h>
#include "SoundSensor.h"

SoundSensor::SoundSensor(uint8_t pin) {
    _pin = pin;
    _n = 0;
}

void SoundSensor::print() {

    Serial.print("Loudness (mean): ");
    Serial.println(_loudnessMean);
}

void SoundSensor::read() {
    long sum = 0;
    for(int i = 0; i < 32; i++) {
        sum += analogRead(_pin);
    }
    sum >>= 5;
    /*
    loudness contains the first value if none was given. otherwise
    the calculation rule is:

                   (n - 1)     new_value
    current mean * -------  +  ---------
                      n            n
     */
    _loudnessMean = _n == 0
        ? sum
        : _loudnessMean * (_n - 1) / _n + sum / _n;
}

void SoundSensor::reset() {
    _n = 0;
    _loudnessMean = 0;
}

uint16_t SoundSensor::getLoudnessMean() {
    return _loudnessMean;
}
