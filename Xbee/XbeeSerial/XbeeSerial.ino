// Example taken from https://www.arduino.cc/en/Tutorial/SoftwareSerialExample

/*
 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.

 The circuit:
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
*/

#include <SoftwareSerial.h>

SoftwareSerial piSerial(10, 11);

void setup()
{
  Serial.begin(9600);
  piSerial.begin(9600);
}

void loop() { // run over and over
    if (piSerial.available()) {
          Serial.write(piSerial.read());
    }
    if (Serial.available()) {
          piSerial.write(Serial.read());
    }
}
