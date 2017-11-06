#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "LoRaTransmitter.h"


LoRaTransmitter::LoRaTransmitter(uint8_t address) {
    _address = address;
}

boolean LoRaTransmitter::init() {

    // Singleton instance of the radio driver
    RH_RF95 driver;

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST
    // transmitter pin, then you can set transmitter powers from 5 to 23 dBm:
    // driver.setTxPower(23, false);

    // Class to manage message delivery and receipt, using the driver declared above
    // do NOT move this to the constructor
    RHReliableDatagram manager(driver, _address);
    _manager = &manager;

    if (_manager->init()) {
        driver.setFrequency(433);
        return true;
    } else {
        return false;
    }
}

void LoRaTransmitter::send(uint16_t receiver, uint8_t *data) {
    uint8_t dataFrameSize = sizeof(*data);
    /*
    sensor readings are going to be send, therefor the buffer
    have to has the size of the struct
    */
    uint8_t buf[dataFrameSize];
    // copy all data from sensor readings into the buffer
    memcpy(buf, data, dataFrameSize);

    Serial.print("Sending to ");
    Serial.println(receiver);

    digitalWrite(_led, HIGH);
    if (_manager->sendtoWait(buf, dataFrameSize, receiver)) {
        Serial.println(F("Message sent"));
    } else {
        Serial.println(F("sendtoWait failed"));
    }
    digitalWrite(_led, LOW);
}
