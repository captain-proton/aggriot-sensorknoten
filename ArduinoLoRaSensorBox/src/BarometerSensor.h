#ifndef BarometerSensor_H
#define BarometerSensor_H

#include <Arduino.h>
#include "Seeed_BMP280.h"

/**
 * With a barometer sensor temperature, pressure and altitude can be  measured. An instance is based upon [Grove Barometer BMP280](http://wiki.seeed.cc/Grove-Barometer_Sensor-BMP280/) and the [sample sketch](https://github.com/Seeed-Studio/Grove_BMP280/blob/master/example/bmp280_example/bmp280_example.ino).
 */
class BarometerSensor {
public:
    void init();
    void loop();
    void reset();
    float getTemperature();
    uint32_t getPressure();
    float getAltitude();
private:
    BMP280 *_bmp280;
    uint8_t _n;
    float _temperature;
    uint32_t _pressure;
};

#endif
