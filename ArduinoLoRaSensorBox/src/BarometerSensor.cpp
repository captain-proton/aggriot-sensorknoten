#include "BarometerSensor.h"

BarometerSensor::BarometerSensor(BMP280 * bmp280)
{
    _bmp280 = bmp280;
}

void BarometerSensor::init()
{
    if (!_bmp280->init())
    {
        Serial.println(F("BMP280 could not be initialized"));
    }
}

void BarometerSensor::loop()
{
    _n += 1;

    float t = _bmp280->getTemperature();
    float p = _bmp280->getPressure() * 1.0;

    _temperature = (_temperature * (_n - 1) / _n) + (t * 1.0 / _n);
    _pressure = (_pressure * (_n - 1) / _n) + (p / _n);
}

void BarometerSensor::reset()
{
    _n = 0;
    _temperature = 0.0;
    _pressure = 0.0;
}

float BarometerSensor::getTemperature() {
    return _temperature;
}

float BarometerSensor::getPressure() {
    return _pressure;
}

float BarometerSensor::getAltitude() {
    return _bmp280->calcAltitude(_pressure);
}
