/*
Based on:

- RadioHead rf95_reliable_datagram_client.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
- Sending sensor readings
    https://forum.arduino.cc/index.php?topic=355434.0
 */
#ifndef LoRaTransmitter_H
#define LoRaTransmitter_H

#include <Arduino.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

class LoRaTransmitter {

public:
    LoRaTransmitter(uint8_t address);
    boolean init();
    void send(uint16_t receiver, uint8_t *data);
private:
    uint8_t _address;
    uint8_t _led;
    RHReliableDatagram *_manager;
};

#endif
