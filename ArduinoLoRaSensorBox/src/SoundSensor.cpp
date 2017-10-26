#include <Arduino.h>
#include "SoundSensor.h"

SoundSensor::SoundSensor(uint8_t pin) {
    _pin = pin;
    _n = 0;
    _loudness = 0;
}

void SoundSensor::print() {

    Serial.print("Loudness: ");
    Serial.println(_loudness);
}

void SoundSensor::read() {
    long sum = 0;
    for(int i = 0; i < 32; i++) {
        sum += analogRead(_pin);
    }
    sum >>= 5;
    /*
                   (n - 1)     new_value
    current mean * -------  +  ---------
                      n            n
     */
     _n += 1;
    _loudness = (_loudness * (_n - 1) / _n) + (sum * 1.0 / _n);
}

void SoundSensor::reset() {
    _n = 0;
}

uint16_t SoundSensor::getLoudness() {
    return _loudness;
}
