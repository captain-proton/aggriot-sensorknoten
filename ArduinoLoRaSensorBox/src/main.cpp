/*
Arduino LoRa Sensor Box

Uses various sensors to read environment data and sends it
via lora to an aggregator. At this moment all sensors have to
be connected to specific ports:

Dust                D8
Sound               A2
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
#include <TaskScheduler.h>

#include "DustCalculator.h"
#include "LoRaTransmitter.h"
#include "TemperatureHumiditySensor.h"
#include "LightSensor.h"
#include "SoundSensor.h"
#include "SensorReadings.h"

// #define _TASK_SLEEP_ON_IDLE_RUN

/* Must be defined if RHReliableDatagram is used */
#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

/*
Do not send data on every loop. the delay is used to calculate if data
should be send or not.
*/
#define SEND_DELAY_MS           120000L

#define DEFAULT_SENSOR_INTERVAL 1000

/* Time dust values are measured */
#define DUST_MEASURING_TIME     30000L
#define DUST_MIN_MEDIAN_COUNT   5
#define DUST_MEDIAN_CAPACITY    10

#define FLOAT_NORMALIZER        100

Scheduler scheduler;

// definitions of task callbacks
void runSensorRecord();
void readWeather();
void readSound();
void readLight();
void sendData();

// inline Task(unsigned long aInterval=0, long aIterations=0, void (*aCallback)()=NULL, Scheduler* aScheduler=NULL, bool aEnable=false, bool (*aOnEnable)()=NULL, void (*aOnDisable)()=NULL);

/*
Task wrapper that calls loop() of sensors that just read one single value and
do not depend on multiple calls as for example dust calculation
 */
Task taskWrapper(DEFAULT_SENSOR_INTERVAL, TASK_FOREVER, &runSensorRecord);
Task tWeather(TASK_IMMEDIATE, TASK_ONCE, &readWeather);
Task tSound(TASK_IMMEDIATE, TASK_ONCE, &readSound);
Task tLight(TASK_IMMEDIATE, TASK_ONCE, &readLight);

/*
Task that is going to send data in an interval.
 */
Task tSendData(SEND_DELAY_MS, TASK_FOREVER, &sendData);

/* SENSORS */
DustCalculator dustCalculator(DUST_MEASURING_TIME, 8, DUST_MIN_MEDIAN_COUNT, DUST_MEDIAN_CAPACITY);
TemperatureHumiditySensor tempSensor(A0);
LightSensor lightSensor(A1);
SoundSensor soundSensor(A2);

/* Transmission via LoRa */
LoRaTransmitter transmitter(TRANSMITTER_ADDRESS);

/* Data container that is delivered over the network */
SensorReadings readings;

void runSensorRecord() {

    tWeather.restart();
    tLight.restart();
    tSound.restart();
}

void readWeather()  {

    tempSensor.loop();
}

void readLight() {

    lightSensor.loop();
}

void readSound() {

    soundSensor.loop();
}

void reset() {
    if (dustCalculator.isCalculated()) {
        dustCalculator.reset();
    }
    tempSensor.reset();
    lightSensor.reset();
    soundSensor.reset();

    readings.reset();
}

void sendData() {

    Serial.print(millis());
    Serial.println(" - sendData() ");
    readings.floatNormalizer = FLOAT_NORMALIZER;

    readings.temperature_f = (uint16_t) (tempSensor.getTemperature() * readings.floatNormalizer);
    readings.humidity_f = (uint16_t) (tempSensor.getHumidity() * readings.floatNormalizer);

    readings.dustConcentration_f = dustCalculator.isCalculated()
            ? (uint32_t) (dustCalculator.getConcentration() * readings.floatNormalizer)
            : 0;

    readings.lightSensorValue = lightSensor.getSensorData();
    readings.lightResistance = lightSensor.getResistance();

    readings.loudness = soundSensor.getLoudness();
    readings.print();

    uint8_t data[readings.size()];
    readings.serialize(data);
    transmitter.send(RECEIVER_ADDRESS, data);
    reset();
}

// start of the sketch
void setup()
{
    // Wait for serial port to be available
    while (!Serial) ;

    // data rate in bits per second for serial transmission
    Serial.begin(9600);

    if (!transmitter.init())
        Serial.println(F("Init failed"));

    dustCalculator.init();

    scheduler.init();
    Serial.println(F("Initialized scheduler"));

    scheduler.addTask(taskWrapper);
    taskWrapper.enable();

    scheduler.addTask(tWeather);
    tempSensor.init();
    tWeather.enable();
    Serial.println(F("Enabled weather sensor"));

    scheduler.addTask(tSound);
    tSound.enable();
    Serial.println(F("Enabled sound sensor"));

    scheduler.addTask(tLight);
    tLight.enable();
    Serial.println(F("Enabled light sensor"));

    scheduler.addTask(tSendData);
    tSendData.enableDelayed(SEND_DELAY_MS);
    Serial.println(F("Enabled data send"));
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    scheduler.execute();

    dustCalculator.loop();
}
