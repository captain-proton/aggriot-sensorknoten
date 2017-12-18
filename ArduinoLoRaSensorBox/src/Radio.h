#ifndef Radio_H
#define Radio_H

#define HANDSHAKE_PAYLOAD_TYPE      3

#include <Arduino.h>
#include <RH_RF95.h>

/**
* Transmitter and receiver instance that is able to send and receive messages over the network.
*
* Examples from RadioHead:
*
* - [Client (driver)](http://www.airspayce.com/mikem/arduino/RadioHead/rf95_client_8pde-example.html)
* - [Server (Driver)](http://www.airspayce.com/mikem/arduino/RadioHead/rf95_server_8pde-example.html)
*
* @param driver RadioHead driver instance that should be used for communication.
*/
class Radio {

public:
    /** Create a new instance of this radio, using given driver */
    Radio(RH_RF95 *driver);
    /**
     * Send a message to the network and wait for successful send.
     * @param data raw data to send
     * @param len  length of the data
     */
    void send(uint8_t *data, uint8_t len);
    /** Retry to send the last saved message. */
    void retry();
    /** Run handshake with an aggregator. Handshake data must be set! \ref Radio::setHandshakeData */
    void handshake();
    /**
     * Try to handle an incoming message.
     * @param dataPtr message data
     * @param len     data length
     */
    void handle_message(uint8_t * dataPtr, uint8_t len);
    /** Should be called on program loop in order to receive incoming messages */
    void loop();
    /** Returns `true` if a previous handshake was successful. */
    bool isConnected();
    /**
     * To run a handshake, handshake data must be set.
     * @param handshakePayload        handshake payload
     * @param handshakeLen            payload length
     * @param handshakePayloadType    payload type
     * @param handshakePayloadTypeIdx index of payload type inside the index
     */
    void setHandshakeData(uint8_t *handshakePayload,
        uint8_t *handshakeLen,
        uint8_t *handshakePayloadType
    );
private:
    uint8_t _address;
    RH_RF95 *_driver;
    bool _isConnected;
    uint8_t *_handshakePayload;
    uint8_t *_handshakePayloadType;
    uint8_t *_handshakeLen;

    void highlightHandshake();
    void printPacket(uint8_t * data, uint8_t * len);
};

#endif
