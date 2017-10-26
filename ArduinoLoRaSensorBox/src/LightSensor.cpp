#include <Arduino.h>
#include "LightSensor.h"

LightSensor::LightSensor(uint8_t pin) {
    _pin = pin;
    _raw = 0;
    _rsensor = 0.0;
    reset();
}

void LightSensor::read() {
    uint16_t newRaw = analogRead(_pin);
    float newRSensor = newRaw > 0
        ? (float) (1023 - newRaw) * 10 / newRaw
        : 0;

    // calculate mean
    _n += 1;
    _raw = _raw * (_n - 1) / _n + newRaw / _n;
    _rsensor = _rsensor * (_n - 1) / _n + newRSensor / _n;
}

void LightSensor::print() {
    Serial.print("Sensor value: ");
    Serial.print(_raw);
    Serial.print("\tresistance: ");
    Serial.println((uint16_t)_rsensor, DEC);
}

void LightSensor::reset() {

    _n = 0;
}

uint16_t LightSensor::getSensorData() {
    return _raw;
}

uint16_t LightSensor::getResistance() {
    return _rsensor;
}
