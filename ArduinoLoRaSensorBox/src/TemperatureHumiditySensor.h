/*
Based on:

- Seeed Wiki
    http://wiki.seeed.cc/Grove-TemperatureAndHumidity_Sensor/
- DHTstable library
    https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable
 */
#ifndef TempHum_H
#define TempHum_H

#include <Arduino.h>
#include <dht.h>

class TemperatureHumiditySensor {
public:
    TemperatureHumiditySensor(uint8_t pin);
    void init();
    boolean read();
    void print();
    float getTemperature();
    float getHumidity();
private:
    uint8_t _pin;
    dht *_dht;
    float _humidity;
    float _temperature;
};

#endif
