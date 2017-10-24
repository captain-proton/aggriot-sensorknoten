#include <Arduino.h>
#include "LightSensor.h"

LightSensor::LightSensor(uint8_t pin) {
    _pin = pin;
}

void LightSensor::read() {
    _raw = analogRead(_pin);
    _rsensor = _raw > 0
        ? (float) (1023 - _raw) * 10 / _raw
        : 0;
}

void LightSensor::print() {
    Serial.print("Sensor value: ");
    Serial.print(_raw);
    Serial.print("\tresistance: ");
    Serial.println((uint16_t)_rsensor, DEC);
}

uint16_t LightSensor::getSensorData() {
    return _raw;
}

uint16_t LightSensor::getResistance() {
    return _rsensor;
}
