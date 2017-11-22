#include <Arduino.h>
#include "TemperatureSensor.h"
#include "math.h"

#define TEMPERATURE_SENS_B      4275        // B value of the thermistor
#define TEMPERATURE_SENS_R0     100000      // R0 = 100k

TemperatureSensor::TemperatureSensor(uint8_t pin) {
    _pin = pin;
    _n = 0;
    _temperature = 0;
}

void TemperatureSensor::print() {

    Serial.print(F("Temperature: "));
    Serial.println(_temperature);
}

void TemperatureSensor::loop() {
    int a = analogRead(_pin);

    float R = a > 0
        ? 1023.0 / a - 1.0
        : 0;
    R = TEMPERATURE_SENS_R0 * R;

    float t = R != 0
        ? 1.0 / (log(R / TEMPERATURE_SENS_R0) / TEMPERATURE_SENS_B + 1 / 298.15) - 273.15 // convert to temperature via datasheet
        : -300;

    if (t >= -273.15)
    {
        _n += 1;
        _temperature = _temperature * (_n - 1) / _n + t / _n;
    }
}

void TemperatureSensor::reset() {
    _n = 0;
    _temperature = 0;
}

float TemperatureSensor::getTemperature() {
    return _temperature;
}
