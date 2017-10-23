#ifndef LoRaTransmitter_H
#define LoRaTransmitter_H

#include <Arduino.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "SensorReadings.h"

class LoRaTransmitter {

public:
    LoRaTransmitter(uint8_t address);
    boolean init();
    void send(uint16_t receiver, SensorReadings *readings);
private:
    uint8_t _address;
    uint8_t _led;
    RHReliableDatagram *_manager;
};

#endif
