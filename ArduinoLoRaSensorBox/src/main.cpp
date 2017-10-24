/*
Based on:

- Grove - Dust Sensor Demo v1.0
    https://github.com/Seeed-Studio/Grove_Dust_Sensor/blob/master/Grove_Dust_Sensor.ino
- RadioHead rf95_reliable_datagram_client.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
- Sending sensor readings
    https://forum.arduino.cc/index.php?topic=355434.0
 */

#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "DustCalculator.h"
#include "LoRaTransmitter.h"
#include "TemperatureHumiditySensor.h"
#include "LightSensor.h"
#include "SensorReadings.h"

/* Must be defined if RHReliableDatagram is used */
#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

#define SENSOR_DELAY_MS         2000

uint8_t srcpin = 8;
uint8_t led = LED_BUILTIN;

DustCalculator dustCalculator(30000);
TemperatureHumiditySensor tempSensor(A0);
LightSensor lightSensor(A1);
LoRaTransmitter transmitter(TRANSMITTER_ADDRESS);
SensorReadings readings;

/*
Take a look at the arduino reference page for detailed function
descriptions.

https://www.arduino.cc/en/Reference/HomePage
*/

// start of the sketch
void setup()
{
    // data rate in bits per second for serial transmission
    Serial.begin(9600);

    // Wait for serial port to be available
    while (!Serial) ;

    if (!transmitter.init())
        Serial.println("init failed");

    tempSensor.init();

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST
    // transmitter pin, then you can set transmitter powers from 5 to 23 dBm:
    // driver.setTxPower(23, false);

    // read from pin 8
    pinMode(srcpin, INPUT);

    readings.floatNormalizer = 100;
    readings.counter = 0;
}

void readDust() {

    if (dustCalculator.isCalculated()) {
        dustCalculator.print();
        float concentration = dustCalculator.getConcentration();

        readings.dustConcentration = (uint32_t) (concentration * readings.floatNormalizer);
    }
}

void readWeather() {

    if (tempSensor.read()) {
        tempSensor.print();
        float temperature = tempSensor.getTemperature();
        float humidity = tempSensor.getHumidity();

        readings.temperature = (uint32_t) (temperature * readings.floatNormalizer);
        readings.humidity = (uint32_t) (humidity * readings.floatNormalizer);
    }
}

void readLight(boolean print = false) {

    lightSensor.read();
    if (print)
        lightSensor.print();
    readings.lightSensorValue = lightSensor.getSensorData();
    readings.lightResistance = lightSensor.getResistance();
}

void sendData() {

    transmitter.send(RECEIVER_ADDRESS, &readings);
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    readDust();
    readWeather();
    readLight(true);

    sendData();

    delay(SENSOR_DELAY_MS);
}
