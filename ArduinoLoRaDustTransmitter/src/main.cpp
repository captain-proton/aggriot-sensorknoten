/*
Based on:

- Grove - Dust Sensor Demo v1.0
    https://github.com/Seeed-Studio/Grove_Dust_Sensor/blob/master/Grove_Dust_Sensor.ino
- RadioHead rf95_reliable_datagram_client.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
- Sending sensor readings
    https://forum.arduino.cc/index.php?topic=355434.0
 */

#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

#include "DustCalculator.h"
#include "LoRaTransmitter.h"
#include "SensorReadings.h"

/* Must be defined if RHReliableDatagram is used */
#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

uint8_t srcpin = 8;
uint8_t led = LED_BUILTIN;

DustCalculator dustCalculator(30000);
LoRaTransmitter transmitter(TRANSMITTER_ADDRESS);
SensorReadings readings;

/*
Take a look at the arduino reference page for detailed function
descriptions.

https://www.arduino.cc/en/Reference/HomePage
*/

// start of the sketch
void setup()
{
    // data rate in bits per second for serial transmission
    Serial.begin(9600);

    // Wait for serial port to be available
    while (!Serial) ;

    if (!transmitter.init())
        Serial.println("init failed");

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST
    // transmitter pin, then you can set transmitter powers from 5 to 23 dBm:
    // driver.setTxPower(23, false);

    // read from pin 8
    pinMode(srcpin, INPUT);

    readings.concentrationNormalizer = 100;
    readings.counter = 0;
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    if (dustCalculator.isCalculated()) {
        dustCalculator.print();
        float concentration = dustCalculator.getConcentration();

        readings.dustConcentration = (uint32_t) (concentration * readings.concentrationNormalizer);
        transmitter.send(RECEIVER_ADDRESS, &readings);
    }
}
