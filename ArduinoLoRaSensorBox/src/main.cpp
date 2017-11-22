/*! \file main.cpp
 *
 * Main arduino sketch, that contains setup and loop.
 *
 * Warning! Do not use delay() in any class, as some calculations block the application to sample data (ex. dust). It may cause unexpected values. Sensor data may be in the form of float values. On transmission these values are normalized. They are marked with the suffix _f.
 *
 * \anchor communicationmsc
 * \msc
 * arcgradient = 4;
 * Main,Dust,Sender,Scheduler,aggriotlib,Network,Receiver,Aggregator;
 * --- [label="main::setup()"];
 * Main => Dust [label="init()", URL="\ref DustCalculator::init()"];
 * Main => Scheduler [label="init()"];
 * Main => aggriotlib [label="communication_init(address)"];
 * Main => aggriotlib [label="aes_init(key, keylen)"];
 * --- [label="main::loop()"];
 * Main => Scheduler [label="execute()"];
 * Scheduler => Main [label="handshake()"];
 * Scheduler => Main [label="runSensorRecord()", URL="\ref runSensorRecord()"];
 * Scheduler => Main [label="sendData()", URL="\ref sendData()"];
 * Main => Sender [label="isConnected()"];
 * Sender >> Main [label="true|false"];
 * Main abox Sender [label="not connected", textbgcolor="#ef5350"];
 * Main => Main [label="setupRandomHandshakeData()"];
 * Main => Sender [label="handshake()", URL="\ref Radio::handshake()"];
 * Main => Sender [label="isConnected()", URL="\ref Radio::isConnected()"];
 * Sender >> Main [label="true|false"];
 * Main abox Sender [label="handshake failed", textbgcolor="#ff7043"];
 * Main => Main [label="setupRandomHandshakeData()"];
 * Main => Sender [label="handshake()", URL="\ref Radio::handshake()"];
 * Main => Sender [label="isConnected()", URL="\ref Radio::isConnected()"];
 * Main abox Sender [label="connected", textbgcolor="#66bb6a"];
 * Main => Dust [label="getConcentration()", URL="\ref DustCalculator::getConcentration()"];
 * Dust >> Main [label="concentration (float)"];
 * Main note Dust [label="every sensor is called that way", textbgcolor="#ffe082"];
 * Main => aggriotlib [label="com_sendMessage(data, len)"];
 * aggriotlib => Main [label="com_sendOutgoingData(data, len)"];
 * Main => Sender [label="send(data, len)"];
 * Sender => Network [label="send(data, len)"];
 * Sender => Network [label="waitPacketSent()"];
 * Sender => Network [label="setModeRx()"];
 * Receiver => Network [label="available()"];
 * Receiver << Network [label="true|false"];
 * Network abox Receiver [label="message is available", textbgcolor="#66bb6a"];
 * --- [label="leaving sensor box context"];
 * Receiver => Network [label="recv(data, len)"];
 * Receiver => Aggregator [label="onMessageReceived(data, len)"];
 * Aggregator => Aggregator [label="validateMessage(data, len)"];
 * Aggregator => Aggregator [label="consumeMessage(data, len)"];
 * Aggregator => Receiver [label="ack(data, len)"];
 * Receiver => Network [label="send(data, len)"];
 * --- [label="again somwhere on main::loop()"];
 * Main => Sender [label="loop()", URL="\ref Radio::loop()"];
 * Sender => Network [label="available()"];
 * Sender << Network [label="true|false"];
 * Sender => Network [label="recv(data, len)"];
 * Sender => aggriotlib [label="communication_dataIn(data, len)"];
 * aggriotlib => Main [label="com_processValidMessage(data, len)"];
 * Main => Sender [label="handle_message(data, len)"];
 * Sender => Sender [label="handle ack"];
 * Sender note Sender [label="maybe handshake?!", textbgcolor="#ffe082"];
 * \endmsc
 */
#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <TaskScheduler.h>

#include "Radio.h"
#include "SensorReadings.h"
#ifdef DUST
    #include "DustCalculator.h"

    /** Time dust values are measured */
    #define DUST_MEASURING_TIME         20000L

    /** How many values must at least be read to get the median from dust measurement */
    #define DUST_MIN_MEDIAN_COUNT       5

    /** Max values that are measured on dust sensor */
    #define DUST_MEDIAN_CAPACITY        10
#endif
#ifdef TEMP_HUM
    #include "TemperatureHumiditySensor.h"
#endif

#ifdef LIGHT
    #include "LightSensor.h"
#endif
#ifdef SOUND
    #include "SoundSensor.h"
#endif
#ifdef BARO
    #include "BarometerSensor.h"
#endif
#ifdef GPS
    #include "GPSSensor.h"
#endif

extern "C" {
    #include "communication.h"
    #include "aes.h"
}

// #define _TASK_SLEEP_ON_IDLE_RUN

/** Defines the interval in milliseconds after new messages should be send */
#define SEND_DELAY_MS               5000L

/** Defines the interval in milliseconds after an unsuccessful handshake should be executed again */
#define HANDSHAKE_DELAY_MS          300000L

/** Minimum size of payload used on handshake */
#define MIN_HANDSHAKE_PAYLOAD_LEN   8

/** Maximum size of payload used on handshake */
#define MAX_HANDSHAKE_PAYLOAD_LEN   16

/** Default interval when sensor data is measured */
#define DEFAULT_SENSOR_INTERVAL     1000

/** When data is send on the radio, float values are normalized so that values can be recalculated with floating points. */
#define FLOAT_NORMALIZER            100

#define DIVIDER                     F("-------------------------\n")

/**
 * Instance with which tasks are executed on specific intervals
 * \msc
 * arcgradient = 4;
 * Main,Scheduler,Wrapper,Weather,Sound,Light,SendData,Handshake;
 * --- [label="main::setup()"];
 * Main => Scheduler [label="init()"];
 * Main => Scheduler [label="addTask(Wrapper)"];
 * Main => Wrapper [label="enable()"];
 * Main => Scheduler [label="addTask(Weather)"];
 * Main => Weather [label="enable()"];
 * Main => Scheduler [label="addTask(Sound)"];
 * Main => Sound [label="enable()"];
 * Main => Scheduler [label="addTask(Light)"];
 * Main => Light [label="enable()"];
 * Main => Scheduler [label="addTask(SendData)"];
 * Main => SendData [label="enable()"];
 * Main => Scheduler [label="addTask(Handshake)"];
 * Main => Handshake [label="enable()"];
 * --- [label="main::loop()"];
 * Main => Scheduler [label="execute()"];
 * Scheduler => Wrapper [label="runSensorRecord()"];
 * Wrapper => Weather [label="restart()"];
 * Wrapper => Sound [label="restart()"];
 * Wrapper => Light [label="restart()"];
 * Scheduler abox Light [label="run once", textbgcolor="#66bb6a"];
 * Scheduler => Weather [label="readWeather()"];
 * Scheduler => Sound [label="readSound()"];
 * Scheduler => Light [label="readLight()"];
 * Scheduler abox SendData [label="infinite", textbgcolor="#29b6f6"];
 * Scheduler => SendData [label="sendData()"];
 * Scheduler => Handshake [label="handshake()"];
 * Handshake => Handshake [label="disable()"];
 * \endmsc
 */
Scheduler scheduler;

/** Task callback when standard sensor measurements are going to be executed. */
void runSensorRecord();

/** Task callback when sensor data should be send. */
void sendData();
/** Task callback when a handshake should be done. */
void handshake();
/** Task callback to check if a handshake was successful. */
void checkConnection();

/** Task wrapper that calls loop() of sensors that just read one single value and do not depend on multiple calls as for example dust calculation */
Task tSensors(DEFAULT_SENSOR_INTERVAL, TASK_FOREVER, &runSensorRecord);

/** Task that is going to send data on a interval. */
Task tSendData(SEND_DELAY_MS, TASK_FOREVER, &sendData);

/** Task that is going to run a handshake. */
Task tHandshake(HANDSHAKE_DELAY_MS, TASK_FOREVER, &handshake);
/** Task that checks if a connection was successfully intiated. */
Task tCheckConnection(TASK_IMMEDIATE, TASK_FOREVER, &checkConnection);

/* SENSORS */

#ifdef DUST
    /** Instance with which dust data can be calculated */
    DustCalculator dustCalculator(DUST_MEASURING_TIME, DUST, DUST_MIN_MEDIAN_COUNT, DUST_MEDIAN_CAPACITY);
#endif
#ifdef TEMP_HUM
    /** Instance with which temperature and humidity can be calculated */
    TemperatureHumiditySensor tempSensor(TEMP_HUM);
#endif
#ifdef LIGHT
    /** Instance with which light can be calculated */
    LightSensor lightSensor(LIGHT);
#endif
#ifdef SOUND
    /** Instance with which sound can be calculated */
    SoundSensor soundSensor(SOUND);
#endif
#ifdef BARO
    BarometerSensor baroSensor;
#endif
#ifdef GPS
    TinyGPS tinyGPS;
    SoftwareSerial sose(3, 4);
    GPSSensor gps(&tinyGPS, &sose);
#endif

/**
 * Every packet send to the aggregator contains the type so that the number of bytes and the order of data inside the packet is known.
 */
enum PayloadType {
    PayloadTypeOfficeFull = 1,
    PayloadTypeOfficeSmall,
    PayloadTypeHandshake,
    PayloadTypeMobile
};

/* Transmission via LoRa */
/** Driver instance with which data is send onto the network */
RH_RF95 driver;
/**
 * Radio instance that uses the radio to send and receives data from the network.
 * @param  driver driver to use
 */
Radio radio(&driver);

/**
 * Contains random handshake data, that is used to run a handshake with an aggregator instance.
 * @param Payload random data.
 */
struct {
    uint8_t Payload[MAX_HANDSHAKE_PAYLOAD_LEN];
    uint8_t PayloadType = PayloadTypeHandshake;
    uint8_t PayloadLen = sizeof(Payload);
    uint8_t PayloadTypeIdx;
} HandshakeData;

/** Data container that is delivered over the network */
SensorReadings readings;

void runSensorRecord() {

    #ifdef TEMP_HUM
        tempSensor.loop();
    #endif
    #ifdef LIGHT
        lightSensor.loop();
    #endif
    #ifdef SOUND
        soundSensor.loop();
    #endif
    #ifdef BARO
        baroSensor.loop();
    #endif
}

/**
 * Resets the measurement data of all sensors.
 */
void reset() {
    #ifdef DUST
        if (dustCalculator.isCalculated()) {
            dustCalculator.reset();
        }
    #endif
    #ifdef TEMP_HUM
        tempSensor.reset();
    #endif
    #ifdef LIGHT
        lightSensor.reset();
    #endif
    #ifdef SOUND
        soundSensor.reset();
    #endif
    #ifdef BARO
        baroSensor.reset();
    #endif

    readings.reset();
}

/**
 * Creates a \ref SensorReadings object that is going to be serialized and send via radio.
 */
void sendData() {

    Serial.println(DIVIDER);
    Serial.println(F("main::sendData()"));

    readings.data.floatNormalizer = FLOAT_NORMALIZER;

    #ifdef TEMP_HUM
        readings.data.temperature_f = (uint16_t) (tempSensor.getTemperature() * readings.data.floatNormalizer);
        readings.data.humidity_f = (uint16_t) (tempSensor.getHumidity() * readings.data.floatNormalizer);
    #endif
    #ifdef DUST
    if (dustCalculator.isCalculated()) {
        readings.data.dustConcentration_f = (uint32_t) (dustCalculator.getConcentration() * readings.data.floatNormalizer);
        readings.data.payloadType = PayloadTypeOfficeFull;
    } else {
        readings.data.dustConcentration_f = 0;
        readings.data.payloadType = PayloadTypeOfficeSmall;
    }
    #endif
    #ifdef LIGHT
        readings.data.lightSensorValue = lightSensor.getSensorData();
        readings.data.lightResistance = lightSensor.getResistance();
    #endif
    #ifdef SOUND
        readings.data.loudness = soundSensor.getLoudness();
    #endif
    #ifdef BARO
        readings.data.temperature_f = (uint16_t) (baroSensor.getTemperature() * FLOAT_NORMALIZER);
        readings.data.pressure = (uint32_t) (baroSensor.getPressure());
    #endif
    #ifdef GPS
        gps.calculatePosition();
        readings.data.longitude = gps.getLongitude();
        readings.data.latitude = gps.getLatitude();
        readings.data.payloadType = PayloadTypeMobile;
    #endif

    readings.print();
    Serial.println(DIVIDER);

    uint8_t len = readings.size();
    uint8_t data[len];
    readings.serialize(data);
    reset();

    if (radio.isConnected()) {
        Serial.println(F("Not connected -> do not send data, returning"));
        com_sendMessage(data, len);
    }

}

/**
* Creates and sets random handshake data onto the radio that the next handshake uses.
*/
void setupRandomHandshakeData() {

    Serial.println(F("main::setupRandomHandshakeData()"));

    // if analog input pin 3 is unconnected, random analog
    // noise will cause the call to randomSeed() to generate
    // different seed numbers each time the sketch runs.
    // randomSeed() will then shuffle the random function.
    randomSeed(analogRead(3));

    uint8_t payloadLen = random(MIN_HANDSHAKE_PAYLOAD_LEN, MAX_HANDSHAKE_PAYLOAD_LEN + 1);
    HandshakeData.PayloadLen = payloadLen;

    uint8_t payloadTypeIdx = random(HandshakeData.PayloadLen);
    HandshakeData.PayloadTypeIdx = payloadTypeIdx;

    for (uint8_t i = 0; i < HandshakeData.PayloadLen; i++) {
        HandshakeData.Payload[i] = i == payloadTypeIdx
            ? PayloadTypeHandshake
            : random(0, 256);
    }
    radio.setHandshakeData(HandshakeData.Payload,
        &(HandshakeData.PayloadLen),
        &(HandshakeData.PayloadType),
        &(HandshakeData.PayloadTypeIdx));
    }

void handshake() {
    if (!radio.isConnected()) {
        Serial.println(F("main::handshake() - Not connected -> init handshake"));
        setupRandomHandshakeData();
        radio.handshake();
    }
}

void checkConnection() {
    if (radio.isConnected()) {

        Serial.println(F("main::checkConnection() - Connected -> disabling handshake, enable data send"));

        tHandshake.disable();
        tCheckConnection.disable();

        scheduler.addTask(tSendData);
        tSendData.enable();
        Serial.println(F("Enabled data send"));
    }
}

/**
 * start of the sketch
 */
void setup()
{
    // Wait for serial port to be available
    while (!Serial) ;

    // data rate in bits per second for serial transmission
    Serial.begin(9600);

    if (driver.init()) {
        driver.setFrequency(433);
        driver.setTxPower(20);
    } else {
        return;
    }

    #ifdef DUST
        dustCalculator.init();
    #endif
    #ifdef TEMP_HUM
        tempSensor.init();
    #endif
    #ifdef BARO
        baroSensor.init();
    #endif
    #ifdef GPS
        gps.init();
        Serial.println(F("GPS initialized"));
    #endif

    scheduler.init();
    Serial.println(F("Initialized scheduler"));

    scheduler.addTask(tSensors);
    tSensors.enable();
    Serial.println(F("Enabled sensor readings task"));

    scheduler.addTask(tHandshake);
    tHandshake.enable();
    Serial.println(F("Enabled radio handshake"));

    scheduler.addTask(tCheckConnection);
    tCheckConnection.enable();
    Serial.println(F("Enabled connection check"));


    #ifdef ADDRESS
        communication_init(ADDRESS);
    #endif
    #ifdef PRE_SHARED_KEY
        #ifdef PSK_LEN
            uint8_t psk[PSK_LEN] = { PRE_SHARED_KEY };
            aes_init(&psk[0], PSK_LEN);
        #endif
    #endif
}

/// called after setup(). loops consecutively. there is not guarantee that
/// it is called in constant gaps
void loop()
{
    scheduler.execute();

    #ifdef GPS
        gps.loop();
    #endif
    #ifdef DUST
    dustCalculator.loop();
    #endif

    radio.loop();

    communication_poll();
}

/**
 * Callback called from aggriotlib to get the current millis
 * @return millis after start of this sketch
 */
uint32_t com_getMillis() {
    return millis();
}

/**
 * Callback method used by aggriotlib when a data packet is build, encrypted and ready to be send.
 * @param ptr    data to send
 * @param length length of the message
 */
void com_sendOutgoingData(uint8_t * ptr, uint8_t length) {
    radio.send(ptr, length);
}

/**
 * Callback method used by aggriotlib after a received message was successful decrypted and validated.
 * @param payload       data that should be processed
 * @param payloadLength length of the payload
 */
void com_processValidMessage(uint8_t * payload, uint8_t payloadLength) {
    // data from aggregator to this instance
    radio.handle_message(payload, payloadLength);
}

/**
 * Callback method used by aggriotlib when a packet was not acked
 */
void com_messageTimeout() {
    radio.retry();
}

/**
* Callback method used by aggriotlib when a packet was acked
*/
void com_messageAcked() {
    // is mir equal
}
