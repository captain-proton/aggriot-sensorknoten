#include <Arduino.h>
#include <dht.h>

#include "TemperatureHumiditySensor.h"

TemperatureHumiditySensor::TemperatureHumiditySensor(uint8_t pin) {
    _pin = pin;
    reset();
}

void TemperatureHumiditySensor::init() {
    Serial.print("LIBRARY VERSION: ");
    Serial.println(DHT_LIB_VERSION);
    Serial.println();
    Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
    dht DHT;
    _dht = &DHT;
}

boolean TemperatureHumiditySensor::read() {
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
    	Serial.println("DHT Checksum error");
    	break;
    case DHTLIB_ERROR_TIMEOUT:
    	Serial.println("DHT Time out error");
    	break;
    default:
    	Serial.println("DHT Unknown error");
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

    Serial.print("Humidity: ");
    Serial.print(_humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(_temperature);
    Serial.println(" *C");
}

void TemperatureHumiditySensor::reset() {

    _n = 0;
}

float TemperatureHumiditySensor::getHumidity() {
    return _humidity;
}


float TemperatureHumiditySensor::getTemperature() {
    return _temperature;
}
