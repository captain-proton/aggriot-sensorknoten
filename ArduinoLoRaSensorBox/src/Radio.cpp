#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "Radio.h"

extern "C" {
    #include "communication.h"
}


Radio::Radio(RH_RF95 *driver) {
    _driver = driver;
}

void Radio::send(uint8_t *data, uint8_t len) {
    /*
    sensor readings are going to be send, therefor the buffer
    have to has the size of the struct
    */
    uint8_t buf[len];
    // copy all data from sensor readings into the buffer
    memcpy(buf, data, len);

    if (_driver->send(data, len)) {
        _driver->waitPacketSent();
        _driver->setModeRx();
        Serial.println(F("Message sent"));

        // _driver->waitAvailableTimeout(1000);
        // uint8_t answer[RH_RF95_MAX_MESSAGE_LEN];
        // uint8_t answer_len = sizeof(buf);
        // _driver->recv(answer, &answer_len);
        // Serial.println((char*)answer);
    } else {
        Serial.println(F("sendtoWait failed"));
    }
    delay(20);
}

void Radio::retry() {
    Serial.println(F("retrying"));
}

void Radio::loop() {
    if (_driver->available()) {
        Serial.println(F("receiving message"));
        uint8_t buf[256];
        uint8_t len;

        if (_driver->recv(buf, &len)) {
            communication_dataIn(buf, len);
        }
    }
}

void Radio::handle_message(uint8_t * dataPtr, uint8_t len)
{
    Serial.write(dataPtr, len);
}
