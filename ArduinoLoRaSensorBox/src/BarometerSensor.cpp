#include "BarometerSensor.h"

void BarometerSensor::init()
{
    _bmp280 = new BMP280();

    if (!_bmp280->init())
    {
        Serial.println(F("BMP280 could not be initialized"));
    }
}

void BarometerSensor::loop()
{
    _n += 1;

    float t = _bmp280->getTemperature();
    uint32_t p = _bmp280->getPressure();

    _temperature = (_temperature * (_n - 1) / _n) + (t * 1.0 / _n);
    _pressure = (_pressure * (_n - 1) / _n) + (p * 1.0 / _n);
}

void BarometerSensor::reset()
{
    _n = 0;
}

float BarometerSensor::getTemperature() {
    return _temperature;
}

uint32_t BarometerSensor::getPressure() {
    return _pressure;
}

float BarometerSensor::getAltitude() {
    return _bmp280->calcAltitude(_pressure);
}
