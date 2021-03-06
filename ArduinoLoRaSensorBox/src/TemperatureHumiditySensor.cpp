#include <Arduino.h>
#include <dht.h>

#include "TemperatureHumiditySensor.h"

TemperatureHumiditySensor::TemperatureHumiditySensor(uint8_t pin) {
    _pin = pin;
    reset();
}

void TemperatureHumiditySensor::init() {
    dht DHT;
    _dht = &DHT;
}

boolean TemperatureHumiditySensor::loop() {
    // READ DATA
    int chk = _dht->read11(_pin);
    float h = 0.0;
    float t = 0.0;
    switch (chk)
    {
    case DHTLIB_OK:
        // DISPLAY DATA
        h = _dht->humidity;
        t = _dht->temperature;

        _n += 1;
        _humidity = _humidity * (_n - 1) / _n + h / _n;
        _temperature = _temperature * (_n - 1) / _n + t / _n;
        _errors = 0;

    	return true;
    case DHTLIB_ERROR_CHECKSUM:
    	Serial.println(F("DHT Checksum error"));
    	break;
    case DHTLIB_ERROR_TIMEOUT:
    	Serial.println(F("DHT Time out error"));
    	break;
    default:
    	Serial.println(F("DHT Unknown error"));
    	break;
    }
    _errors += 1;
    if (_errors > MAX_ERRORS) {
        _humidity = -1;
        _temperature = -1;
    }
    return false;
}

void TemperatureHumiditySensor::print() {

    Serial.print(F("Humidity: "));
    Serial.print(_humidity);
    Serial.print(F(" %\t"));
    Serial.print(F("Temperature: "));
    Serial.print(_temperature);
    Serial.println(" *C");
}

void TemperatureHumiditySensor::reset() {
    _n = 0;
    _humidity = 0;
    _temperature = 0;
}

float TemperatureHumiditySensor::getHumidity() {
    return _humidity;
}


float TemperatureHumiditySensor::getTemperature() {
    return _temperature;
}
