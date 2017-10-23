#include <Arduino.h>
#include <dht.h>

#include "TemperatureHumiditySensor.h"

TemperatureHumiditySensor::TemperatureHumiditySensor(uint8_t pin) {
    _pin = pin;
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
    Serial.print("DHT11, \t");
    int chk = _dht->read11(_pin);
    switch (chk)
    {
    case DHTLIB_OK:
    	Serial.print("OK,\t");
        // DISPLAY DATA
        _humidity = _dht->humidity;
        _temperature = _dht->temperature;
    	return true;
    case DHTLIB_ERROR_CHECKSUM:
    	Serial.print("Checksum error,\t");
    	break;
    case DHTLIB_ERROR_TIMEOUT:
    	Serial.print("Time out error,\t");
    	break;
    default:
    	Serial.print("Unknown error,\t");
    	break;
    }
    _temperature = -1.0;
    _humidity = -1.0;
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

float TemperatureHumiditySensor::getHumidity() {
    return _humidity;
}


float TemperatureHumiditySensor::getTemperature() {
    return _temperature;
}
