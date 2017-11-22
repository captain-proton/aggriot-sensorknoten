#include <Arduino.h>
#include "PIRMotionSensor.h"

PIRMotionSensor::PIRMotionSensor(uint8_t pin) {
    _pin = pin;
    _detected = false;
}

void PIRMotionSensor::init()
{
    pinMode(_pin, INPUT);
}

void PIRMotionSensor::print() {

    Serial.print(F("People detected: "));
    Serial.println(_detected);
}

void PIRMotionSensor::loop() {
    int sensorValue = digitalRead(_pin);
    _detected = _detected || sensorValue == HIGH;
}

void PIRMotionSensor::reset() {
    _detected = false;
}

bool PIRMotionSensor::isPeopleDetected() {
    return _detected;
}
