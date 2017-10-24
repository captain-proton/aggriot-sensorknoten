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

#define DUST_MEASURING_TIME     10000
#define SEND_DELAY_MS           10000

DustCalculator dustCalculator(DUST_MEASURING_TIME, 8);
TemperatureHumiditySensor tempSensor(A0);
LightSensor lightSensor(A1);
LoRaTransmitter transmitter(TRANSMITTER_ADDRESS);
SensorReadings readings;

uint32_t lastSendTime;

/*
Take a look at the arduino reference page for detailed function
descriptions.

https://www.arduino.cc/en/Reference/HomePage
*/

void readDust() {

    if (dustCalculator.calculate()) {
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

void readLight() {

    lightSensor.read();
    lightSensor.print();
    readings.lightSensorValue = lightSensor.getSensorData();
    readings.lightResistance = lightSensor.getResistance();
}

void sendData() {

    if ((millis() - lastSendTime) >= SEND_DELAY_MS) {
        lastSendTime = millis();
        transmitter.send(RECEIVER_ADDRESS, &readings);
    }
}

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
    dustCalculator.init();

    lastSendTime = millis();
    readings.floatNormalizer = 100;
    readings.counter = 0;
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    readDust();
    readWeather();
    readLight();

    sendData();
}
