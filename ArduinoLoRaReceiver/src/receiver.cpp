/*

Based on:

- RadioHead rf95_reliable_datagram_server.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
 */

#include <SPI.h>
#include <RH_RF95.h>

// Singleton instance of the radio driver
RH_RF95 driver;

/* the led is used to highlight if a message is received. */
uint8_t led = LED_BUILTIN;

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

void loop() {

    if (driver.available()) {
        // Dont put this on the stack:
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        // Wait for a message addressed to us from the transmitter
        if (driver.recv(buf, &len))
        {
            digitalWrite(led, HIGH);

            Serial.write(buf, len);

            digitalWrite(led, LOW);
        }
    }
}
