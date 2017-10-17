/*
Based on:

- Grove - Dust Sensor Demo v1.0
    https://github.com/Seeed-Studio/Grove_Dust_Sensor/blob/master/Grove_Dust_Sensor.ino
- RadioHead rf95_server.pde sample
    http://www.airspayce.com/mikem/arduino/RadioHead/
- Sending sensor readings
    https://forum.arduino.cc/index.php?topic=355434.0
 */

#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>

/* Must be defined if RHReliableDatagram is used */
#define TRANSMITTER_ADDRESS     1
#define RECEIVER_ADDRESS        2

int srcpin = 8;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 2000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

// Singleton instance of the radio driver
RH_RF95 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, TRANSMITTER_ADDRESS);

// define what data is send
struct datagram {
    float dust_concentration;
    unsigned long counter;
     
} SensorReadings;

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

        // dummy
        SensorReadings.dust_concentration = 47.11;

        Serial.print(" concentration = ");
        Serial.print(concentration);
        Serial.println(" pcs/0.01cf");

        sendSensorReadings();

        // reset values to start sampling again
        lowpulseoccupancy = 0;
        starttime = millis();
    }
}

/*
Sends all data given in SensorReadings to the receiver defined by
RECEIVER_ADDRESS
*/
void sendSensorReadings() {

    uint8_t buf[sizeof(SensorReadings)];

    uint8_t dataFrameSize = sizeof(SensorReadings);
    memcpy(buf, &SensorReadings, dataFrameSize);

    Serial.println("Sending to rf95_reliable_datagram_server");

    if (manager.sendtoWait(buf, dataFrameSize, RECEIVER_ADDRESS)) {
        Serial.println("Message sent");
    } else {
        Serial.println("sendto failed");
    }
    SensorReadings.counter = SensorReadings.counter + 1;
}
