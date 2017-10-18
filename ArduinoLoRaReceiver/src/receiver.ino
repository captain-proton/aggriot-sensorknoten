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

#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

// Singleton instance of the radio driver
RH_RF95 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, RECEIVER_ADDRESS);

uint8_t led = LED_BUILTIN;

// define what data is send
struct datagram {
    uint32_t dustConcentration;
    uint8_t concentrationNormalizer;
    uint32_t counter;

} SensorReadings;

void setup() {

    pinMode(led, OUTPUT);
    Serial.begin(9600);
    // Wait for serial port to be available
    while (!Serial) ;

    if (!manager.init()) {
        Serial.println("init failed");
    }
}

void loop() {
    if (manager.available()) {

        // Dont put this on the stack:
        uint8_t buf[sizeof(SensorReadings)];
        uint8_t from;
        uint8_t len = sizeof(buf);

        if (manager.available()) {
            // Wait for a message addressed to us from the transmitter
            if (manager.recvfromAck(buf, &len, &from))
            {
                digitalWrite(led, HIGH);
                memcpy(&SensorReadings, buf, sizeof(SensorReadings));
                Serial.println("--------------------------------------------");
                Serial.print("Got message from unit: ");
                Serial.println(from, HEX);
                Serial.print("Transmission number: ");
                Serial.println(SensorReadings.counter);
                Serial.println("");

                uint8_t normalizer = SensorReadings.concentrationNormalizer == 0
                    ? 1
                    : SensorReadings.concentrationNormalizer;
                float concentration = SensorReadings.dustConcentration / (float) normalizer;
                Serial.print("Dust concentration: ");
                Serial.println(concentration);

                Serial.println("--------------------------------------------");
                digitalWrite(led, LOW);
            }
        }
    }
}
