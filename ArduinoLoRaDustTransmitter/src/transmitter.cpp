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

/* Must be defined if RHReliableDatagram is used */
#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

uint8_t srcpin = 8;
uint8_t led = LED_BUILTIN;
uint32_t duration;
uint32_t starttime;
uint32_t sampletime_ms = 30000;
uint32_t lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

// Singleton instance of the radio driver
RH_RF95 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, TRANSMITTER_ADDRESS);

// define what data is send
struct datagram {
    uint32_t dustConcentration;
    uint8_t concentrationNormalizer;
    uint32_t counter;

} SensorReadings;

/*
Sends all data given in SensorReadings to the receiver defined by
RECEIVER_ADDRESS
*/
void sendSensorReadings() {

    uint8_t dataFrameSize = sizeof(SensorReadings);
    /*
    sensor readings are going to be send, therefor the buffer
    have to has the size of the struct
    */
    uint8_t buf[dataFrameSize];
    // copy all data from sensor readings into the buffer
    memcpy(buf, &SensorReadings, dataFrameSize);

    Serial.print("Sending to ");
    Serial.println(RECEIVER_ADDRESS);

    digitalWrite(led, HIGH);
    if (manager.sendtoWait(buf, dataFrameSize, RECEIVER_ADDRESS)) {
        Serial.println("Message sent");
        SensorReadings.counter = SensorReadings.counter + 1;
    } else {
        Serial.println("sendtoWait failed");
    }
    digitalWrite(led, LOW);
}

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

    if (!manager.init())
        Serial.println("init failed");

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST
    // transmitter pin, then you can set transmitter powers from 5 to 23 dBm:
    // driver.setTxPower(23, false);

    // read from pin 8
    pinMode(srcpin, INPUT);

    // millis() returns 0 for arduino start time
    starttime = millis();

    SensorReadings.concentrationNormalizer = 100;
    SensorReadings.counter = 0;
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    // how long the pin had a low pulse block until it got HIGH (microseconds!)
    duration = pulseIn(srcpin, LOW);

    // add duration to the current occupancy
    lowpulseoccupancy = lowpulseoccupancy + duration;

    // low pulse is measured until sample time is reached
    if ((millis() - starttime) >= sampletime_ms)
    {
        Serial.print("lpo = ");
        Serial.print(lowpulseoccupancy);
        // Integer percentage 0=>100
        ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
        Serial.print(" ratio = ");
        Serial.print(ratio);

        // using spec sheet curve
        concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;

        SensorReadings.dustConcentration = (uint32_t) (concentration * SensorReadings.concentrationNormalizer);

        Serial.print(" concentration = ");
        Serial.print(concentration);
        Serial.println(" pcs/0.01cf");

        sendSensorReadings();

        // reset values to start sampling again
        lowpulseoccupancy = 0;
        starttime = millis();
    }
}
