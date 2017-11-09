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
#include "Radio.h"
#include "TemperatureHumiditySensor.h"
#include "LightSensor.h"
#include "SoundSensor.h"
#include "SensorReadings.h"

extern "C" {
    #include "communication.h"
    #include "aes.h"
}

// #define _TASK_SLEEP_ON_IDLE_RUN

/*
Do not send data on every loop. the delay is used to calculate if data
should be send or not.
*/
#define SEND_DELAY_MS           5000L
#define HANDSHAKE_DELAY_MS      300000L

#define DEFAULT_SENSOR_INTERVAL 1000

/* Time dust values are measured */
#define DUST_MEASURING_TIME     20000L
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
void handshake();

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

Task tHandshake(HANDSHAKE_DELAY_MS, TASK_FOREVER, &handshake);

/* SENSORS */
DustCalculator dustCalculator(DUST_MEASURING_TIME, 8, DUST_MIN_MEDIAN_COUNT, DUST_MEDIAN_CAPACITY);
TemperatureHumiditySensor tempSensor(A0);
LightSensor lightSensor(A1);
SoundSensor soundSensor(A2);

enum PayloadType {
    PayloadTypeFull = 1,
    PayloadTypeSmall,
    PayloadTypeHandshake
};

/* Transmission via LoRa */
RH_RF95 driver;
Radio radio(&driver);
struct {
    uint8_t Payload[8];
    uint8_t PayloadType = PayloadTypeHandshake;
    uint8_t PayloadLen = sizeof(Payload);
} HandshakeData;

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

// boolean zeros = false;

void sendData() {

    Serial.println(F("sending data"));

    readings.data.floatNormalizer = FLOAT_NORMALIZER;

    readings.data.temperature_f = (uint16_t) (tempSensor.getTemperature() * readings.data.floatNormalizer);
    readings.data.humidity_f = (uint16_t) (tempSensor.getHumidity() * readings.data.floatNormalizer);

    if (dustCalculator.isCalculated()) {
        readings.data.dustConcentration_f = (uint32_t) (dustCalculator.getConcentration() * readings.data.floatNormalizer);
        readings.data.payloadType = PayloadTypeFull;
    } else {
        readings.data.dustConcentration_f = 0;
        readings.data.payloadType = PayloadTypeSmall;
    }

    readings.data.lightSensorValue = lightSensor.getSensorData();
    readings.data.lightResistance = lightSensor.getResistance();

    readings.data.loudness = soundSensor.getLoudness();
    readings.print();

    uint8_t len = readings.size();
    uint8_t data[len];
    readings.serialize(data);

    // dummy data
    // uint8_t len = 8;
    // uint8_t data[len];
    // memset(data, zeros ? 0 : 1, len);
    // zeros = !zeros;

    com_sendMessage(data, len);

    reset();
}

void handshake() {
    if (!radio.isConnected()) {
        radio.handshake();
    } else {
        tHandshake.disable();
    }
}

// start of the sketch
void setup()
{
    // Wait for serial port to be available
    while (!Serial) ;

    // data rate in bits per second for serial transmission
    Serial.begin(9600);

    if (driver.init()) {
        driver.setFrequency(433);
    } else {
        return;
    }

    // if analog input pin 3 is unconnected, random analog
    // noise will cause the call to randomSeed() to generate
    // different seed numbers each time the sketch runs.
    // randomSeed() will then shuffle the random function.
    randomSeed(analogRead(3));

    for (uint8_t i = 1; i < sizeof(HandshakeData.PayloadLen); i++) {
        HandshakeData.Payload[i] = random(0, 255);
    }
    radio.setHandshakeData(HandshakeData.Payload, &(HandshakeData.PayloadLen), &(HandshakeData.PayloadType));

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

    scheduler.addTask(tHandshake);
    tHandshake.enable();
    Serial.println(F("Enabled radio handshake"));

    scheduler.addTask(tSendData);
    tSendData.enableDelayed(SEND_DELAY_MS);
    Serial.println(F("Enabled data send"));

    communication_init(0x11223344);
    uint8_t key[] = {0xAB, 0xCD, 0xEF, 0x91,
        0x34, 0xEF, 0xAB, 0xCD,
        0xEF, 0x91, 0x34, 0xEF,
        0xAB, 0xCD, 0xEF, 0x91};
    aes_init(&key[0], 16);
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    scheduler.execute();

    radio.loop();

    dustCalculator.loop();

    communication_poll();
}

uint32_t com_getMillis() {
    return millis();
}

/**
 * transmit data via radio
 * @param ptr    dat to send
 * @param length length of the message
 */
void com_sendOutgoingData(uint8_t * ptr, uint8_t length) {
    radio.send(ptr, length);
}

void com_processValidMessage(uint8_t * payload, uint8_t payloadLength) {
    Serial.println("com_processValidMessage()");
    // data from aggregator to this instance
    radio.handle_message(payload, payloadLength);
}

void com_messageTimeout() {
    radio.retry();
}

void com_messageAcked() {
    // is mir equal
}
