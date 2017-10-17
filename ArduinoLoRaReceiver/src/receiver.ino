// rf95_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing server
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_client
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with
// the RFM95W, Adafruit Feather M0 with RFM95

#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

// Singleton instance of the radio driver
RH_RF95 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, RECEIVER_ADDRESS);

int led = 13;

struct datagram {
    float dust_concentration;
    unsigned long counter;
     
} SensorReadings;

void setup() {

    pinMode(led, OUTPUT);
    Serial.begin(9600);
    // Wait for serial port to be available
    while (!Serial) ;
    
    if (!manager.init()) {
        Serial.println("init failed");
    }
}

void loop() {
    if (manager.available()) {

        // Dont put this on the stack:
        uint8_t buf[sizeof(SensorReadings)];
        uint8_t from;
        uint8_t len = sizeof(buf);

        if (manager.available()) {
            // Wait for a message addressed to us from the client
            if (manager.recvfromAck(buf, &len, &from))
            {
                memcpy(&SensorReadings, buf, sizeof(SensorReadings));
                Serial.println("--------------------------------------------");
                Serial.print("Got message from unit: ");
                Serial.println(from, HEX);
                Serial.print("Transmission number: ");
                Serial.println(SensorReadings.counter);
                Serial.println("");
                
                Serial.print("Dust concentration: ");
                Serial.println(SensorReadings.dust_concentration);
                
                Serial.println("--------------------------------------------");
            }
        }
    }
}

