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
        /*
         set to rx mode so that in loop available messages can
         be received.
         */
        _driver->setModeRx();
        Serial.println(F("Message sent"));
    } else {
        Serial.println(F("sendtoWait failed"));
    }
    delay(20);
}

void Radio::retry() {
    Serial.println(F("retrying"));
}

void Radio::handshake() {
    Serial.println(F("Radio::handshake()"));
    com_sendMessage(_handshakePayload, *_handshakeLen);
}

bool Radio::isConnected() {
    return _isConnected;
}

void Radio::loop() {
    if (_driver->available()) {
        Serial.println(F("Message available"));
        uint8_t len = 255;
        uint8_t buf[len];

        if (_driver->recv(buf, &len)) {
            communication_dataIn(buf, len);
        }
    }
}

void Radio::handle_message(uint8_t * dataPtr, uint8_t len)
{
    Serial.println("Radio::handle_message");

    if (!_isConnected
        && len > 0
        && len == *_handshakeLen
        && dataPtr[*_handshakePayloadTypeIdx] == *_handshakePayloadType)
    {
        bool b = true;
        for (uint8_t i = 0; i < len; i++) {
            b = b // if b is true compare next value
                ? dataPtr[i] == _handshakePayload[i]
                : b; // otherwise keep b as false
        }
        _isConnected = b;
        if (_isConnected) {
            Serial.println(F("Handshake successful"));
        } else {
            Serial.println(F("Message looks like handshake response, but was not equal"));
        }
    }
}

void Radio::setHandshakeData(uint8_t *handshakePayload,
    uint8_t *handshakeLen,
    uint8_t *handshakePayloadType,
    uint8_t *handshakePayloadTypeIdx) {

    _handshakePayload = handshakePayload;
    _handshakeLen = handshakeLen;
    _handshakePayloadType = handshakePayloadType;
    _handshakePayloadTypeIdx = handshakePayloadTypeIdx;
}
