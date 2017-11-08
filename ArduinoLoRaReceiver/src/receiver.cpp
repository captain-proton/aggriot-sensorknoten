/*

Based on:

- RadioHead rf95_reliable_datagram_server.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
 */

#include <SPI.h>
#include <RH_RF95.h>

#define SERIAL_MAX_READ             256

// Singleton instance of the radio driver
RH_RF95 driver;

uint8_t input[SERIAL_MAX_READ];
int inputLen;

/* the led is used to highlight if a message is received. */
uint8_t led = LED_BUILTIN;

void checkRadio() {

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

void clearInput() {
    for (int i = 0; i < SERIAL_MAX_READ; i++) {
        input[i] = 0;
    }
    inputLen = 0;
}

bool checkInput() {
    uint8_t available = Serial.available();
    if (available > 0) {
        if (inputLen + available >= SERIAL_MAX_READ)
            clearInput();

        input[inputLen] = Serial.read();
        inputLen++;

        if (input[inputLen - 1] == '\n') {
            return true;
        }
    }
    return false;
}

void sendResponse() {

    digitalWrite(led, HIGH);

    driver.send(input, inputLen);
    driver.waitPacketSent();
    driver.setModeRx();

    clearInput();

    digitalWrite(led, LOW);
}

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

    checkRadio();
    if (checkInput()) {
        sendResponse();
    }
}
