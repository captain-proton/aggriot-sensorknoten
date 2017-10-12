/*
Grove - Dust Sensor Demo v1.0
 Interface to Shinyei Model PPD42NS Particle Sensor
 Program by Christopher Nafis
 Written April 2012

 http://www.seeedstudio.com/depot/grove-dust-sensor-p-1050.html
 http://www.sca-shinyei.com/pdf/PPD42NS.pdf

 JST Pin 1 (Black Wire)  =&gt; //Arduino GND
 JST Pin 3 (Red wire)    =&gt; //Arduino 5VDC
 JST Pin 4 (Yellow wire) =&gt; //Arduino Digital Pin 8
 */

int pin = 8;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

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

    // read from pin 8
    pinMode(pin, INPUT);

    // millis() returns 0 for arduino start time
    starttime = millis();
}

// called after setup(). loops consecutively. there is not guarantee that
// it is called in constant gaps
void loop()
{
    // how long the pin had a low pulse block until it got HIGH (microseconds!)
    duration = pulseIn(pin, LOW);

    // add duration to the current occupancy
    lowpulseoccupancy = lowpulseoccupancy + duration;

    // low pulse is measured until sample time is reached
    if ((millis() - starttime) >= sampletime_ms)
    {
        // Integer percentage 0=>100
        ratio = lowpulseoccupancy / (sampletime_ms * 10.0);

        // using spec sheet curve
        concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;

        Serial.print("concentration = ");
        Serial.print(concentration);
        Serial.println(" pcs/0.01cf");
        Serial.println("\n");

        // reset values to start sampling again
        lowpulseoccupancy = 0;
        starttime = millis();
    }
}
