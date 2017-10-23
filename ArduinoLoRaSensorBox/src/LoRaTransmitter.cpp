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

void LoRaTransmitter::send(uint16_t receiver, SensorReadings *readings) {
    uint8_t dataFrameSize = sizeof(*readings);
    /*
    sensor readings are going to be send, therefor the buffer
    have to has the size of the struct
    */
    uint8_t buf[dataFrameSize];
    // copy all data from sensor readings into the buffer
    memcpy(buf, readings, dataFrameSize);

    Serial.print("Sending to ");
    Serial.println(receiver);

    digitalWrite(_led, HIGH);
    if (_manager->sendtoWait(buf, dataFrameSize, receiver)) {
        Serial.println("Message sent");
        readings->counter = readings->counter + 1;
    } else {
        Serial.println("sendtoWait failed");
    }
    digitalWrite(_led, LOW);
}
