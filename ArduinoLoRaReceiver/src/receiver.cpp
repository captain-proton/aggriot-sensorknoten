/*

Based on:

- RadioHead rf95_reliable_datagram_server.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
- Sending sensor readings
    https://forum.arduino.cc/index.php?topic=355434.0
 */

#include <SPI.h>
#include <RH_RF95.h>

#include "SensorReadings.h"

#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

#define DIVIDER "--------------------------------------------"

// Singleton instance of the radio driver
RH_RF95 driver;

/* the led is used to highlight if a message is received. */
uint8_t led = LED_BUILTIN;

/* data container */
SensorReadings readings;

void setup() {

    pinMode(led, OUTPUT);
    Serial.begin(9600);
    // Wait for serial port to be available
    while (!Serial) ;

    if (!driver.init()) {
        Serial.println(F("Init failed"));
    } else {
        driver.setFrequency(433);
    }

}

void printMessageHeader(uint8_t from, uint32_t number) {

    Serial.println(DIVIDER);
    Serial.print(F("Got message from unit: "));
    Serial.println(from, HEX);
    Serial.print(F("Transmission number: "));
    Serial.println(number);
}

void printDustConcentration(uint8_t normalizer, SensorReadings *readings) {
    float concentration = readings->data.dustConcentration_f / (float) normalizer;
    Serial.print(F("Dust concentration: "));
    Serial.println(concentration);
}

void printWeather(uint8_t normalizer, SensorReadings *readings) {

    float humidity = readings->data.humidity_f / (float) normalizer;
    float temperature = readings->data.temperature_f / (float) normalizer;
    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.print(F(" %\t"));
    Serial.print(F("Temperature: "));
    Serial.println(temperature);
}

void printLight(SensorReadings *readings) {

    uint16_t lightSensorValue = readings->data.lightSensorValue;
    uint16_t lightResistance = readings->data.lightResistance;
    Serial.print(F("Light intensity: "));
    Serial.print(lightSensorValue);
    Serial.print(F("\t"));
    Serial.print(F("resistance: "));
    Serial.println(lightResistance);
}

void printLoudness(SensorReadings * readings) {
    uint16_t loudness = readings->data.loudness;
    Serial.print(F("Loudness: "));
    Serial.println(loudness);
}

void loop() {

    if (driver.available()) {
        // Dont put this on the stack:
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        // Wait for a message addressed to us from the transmitter
        if (driver.recv(buf, &len))
        {
            digitalWrite(led, HIGH);
            // memcpy(&readings, buf, 17);

            // uint8_t normalizer = readings.data.floatNormalizer == 0
            //     ? 1
            //     : readings.data.floatNormalizer;
            // printMessageHeader(from, readings.data.counter);
            // printDustConcentration(normalizer, &readings);
            // printWeather(normalizer, &readings);
            // printLight(&readings);
            // printLoudness(&readings);

            Serial.write(buf, len);

            digitalWrite(led, LOW);

            // Send a reply
            uint8_t data[] = "And hello back to you";
            driver.send(data, sizeof(data));
            driver.waitPacketSent();
        }
    }
}
