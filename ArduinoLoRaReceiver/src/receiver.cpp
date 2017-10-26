/*

Based on:

- RadioHead rf95_reliable_datagram_server.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
- Sending sensor readings
    https://forum.arduino.cc/index.php?topic=355434.0
 */

#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "SensorReadings.h"

#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

#define DIVIDER "--------------------------------------------"

// Singleton instance of the radio driver
RH_RF95 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, RECEIVER_ADDRESS);

/* the led is used to highlight if a message is received. */
uint8_t led = LED_BUILTIN;

/* data container */
SensorReadings readings;

void setup() {

    pinMode(led, OUTPUT);
    Serial.begin(9600);
    // Wait for serial port to be available
    while (!Serial) ;

    if (!manager.init()) {
        Serial.println("init failed");
    } else {
        driver.setFrequency(433);
    }

}

void printMessageHeader(uint8_t from, uint32_t number) {

    Serial.println(DIVIDER);
    Serial.print("Got message from unit: ");
    Serial.println(from, HEX);
    Serial.print("Transmission number: ");
    Serial.println(number);
    Serial.println("");
}

void printDustConcentration(uint8_t normalizer, SensorReadings *readings) {
    float concentration = readings->dustConcentration_f / (float) normalizer;
    Serial.print("Dust concentration: ");
    Serial.println(concentration);
}

void printWeather(uint8_t normalizer, SensorReadings *readings) {

    float humidity = readings->humidity_f / (float) normalizer;
    float temperature = readings->temperature_f / (float) normalizer;
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.println(temperature);
}

void printLight(SensorReadings *readings) {

    uint16_t lightSensorValue = readings->lightSensorValue;
    uint16_t lightResistance = readings->lightResistance;
    Serial.print("Light intensity: ");
    Serial.print(lightSensorValue);
    Serial.print("\t");
    Serial.print("resistance: ");
    Serial.println(lightResistance);
}

void printLoudness(SensorReadings * readings) {
    uint16_t loudness = readings->loudness;
    Serial.print("Loudness: ");
    Serial.println(loudness);
}

void loop() {

    // Dont put this on the stack:
    uint8_t buf[sizeof(SensorReadings)];
    uint8_t from;
    uint8_t len = sizeof(buf);

    if (manager.available()) {
        // Wait for a message addressed to us from the transmitter
        if (manager.recvfromAck(buf, &len, &from))
        {
            digitalWrite(led, HIGH);
            memcpy(&readings, buf, sizeof(SensorReadings));

            uint8_t normalizer = readings.floatNormalizer == 0
                ? 1
                : readings.floatNormalizer;

            printMessageHeader(from, readings.counter);
            printDustConcentration(normalizer, &readings);
            printWeather(normalizer, &readings);
            printLight(&readings);
            printLoudness(&readings);

            Serial.println(DIVIDER);
            digitalWrite(led, LOW);
        }
    }
}
