/*
Based on:

- RadioHead rf95_reliable_datagram_client.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
 */
#ifndef Radio_H
#define Radio_H

#include <Arduino.h>
#include <RH_RF95.h>

class Radio {

public:
    Radio(RH_RF95 *driver);
    void send(uint8_t *data, uint8_t len);
    void retry();
    void handle_message(uint8_t * dataPtr, uint8_t len);
    void loop();
private:
    uint8_t _address;
    RH_RF95 *_driver;
};

#endif
