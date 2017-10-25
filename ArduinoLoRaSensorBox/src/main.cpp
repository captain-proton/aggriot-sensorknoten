/*
Arduino LoRa Sensor Box

Uses various sensors to read environment data and sends it
via lora to an aggregator. At this moment all sensors have to
be connected to specific ports:

Dust                D8
Light               A1
Temp and Humidity   A0

Warning! Do not use delay() in any class, as some calculations block the
application to sample data (ex. dust). It may cause unexpected values.

Sensor data may be in the form of float values. On transmission these values
are normalized. They are marked with the suffix _f.
 */

#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "DustCalculator.h"
#include "LoRaTransmitter.h"
#include "TemperatureHumiditySensor.h"
#include "LightSensor.h"
#include "SoundSensor.h"
#include "SensorReadings.h"

/* Must be defined if RHReliableDatagram is used */
#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

/* Time dust values are measured */
#define DUST_MEASURING_TIME     10000

/*
Do not send data on every loop. the delay is used to calculate if data
should be send or not.
*/
#define SEND_DELAY_MS           10000

/* SENSORS */
DustCalculator dustCalculator(DUST_MEASURING_TIME, 8);
TemperatureHumiditySensor tempSensor(A0);
LightSensor lightSensor(A1);
SoundSensor soundSensor(A2);

/* Transmission via LoRa */
LoRaTransmitter transmitter(TRANSMITTER_ADDRESS);

/* Data container that is delivered over the network */
SensorReadings readings;

/* Contains the 'time' a message was transmitted */
uint32_t lastSendTime;

void readDust() {

    if (dustCalculator.calculate()) {

        dustCalculator.print();
        float concentration = dustCalculator.getConcentration();

        readings.dustConcentration_f = (uint32_t) (concentration * readings.floatNormalizer);
    }
}

void readWeather() {

    if (tempSensor.read()) {

        tempSensor.print();
        float temperature = tempSensor.getTemperature();
        float humidity = tempSensor.getHumidity();

        readings.temperature_f = (uint16_t) (temperature * readings.floatNormalizer);
        readings.humidity_f = (uint16_t) (humidity * readings.floatNormalizer);
    }
}

void readLight() {

    lightSensor.read();
    lightSensor.print();
    readings.lightSensorValue = lightSensor.getSensorData();
    readings.lightResistance = lightSensor.getResistance();
}

void readSound() {

    soundSensor.read();
    soundSensor.print();
    readings.loudnessMean = soundSensor.getLoudnessMean();
}

boolean sendData() {

    // wait until delay is reached
    if ((millis() - lastSendTime) >= SEND_DELAY_MS) {

        lastSendTime = millis();
        transmitter.send(RECEIVER_ADDRESS, &readings);
        return true;
    }
    return false;
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
    readSound();

    if (sendData()) {
        soundSensor.reset();
    }
}
