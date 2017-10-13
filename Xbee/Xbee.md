# Xbee Raspberry PI and Arduino

1. [Connecting a Raspberry PI with a Xbee](https://dzone.com/articles/connecting-xbee-raspberry-pi)
2. [Python and the Xbee Serial API](https://pypi.python.org/pypi/XBee)
3. [Xbee Arduino API](https://github.com/andrewrapp/xbee-arduino)

## [Xbee Networks](https://learn.sparkfun.com/tutorials/exploring-xbees-and-xctu/configuring-networks)

*Channel*
> This controls the frequency band that your XBee communicates over. Most XBee’s operate on the 2.4GHz 802.15.4 band, and the channel further calibrates the operating frequency within that band.

*personal area network ID (PAN ID)*
> The network ID is some hexadecimal value between 0 and 0xFFFF. XBees can only communicate with each other if they have the same network ID.

*Addresses*
>Each XBee in a network should be assigned a 16-bit address (again between 0 and 0xFFFF), which is referred to as MY address, or the “source” address. Another setting, the destination address, determines which source address an XBee can send data to. For one XBee to be able to send data to another, it must have the same destination address as the other XBee’s source.

__Xbees communicate one way!__
*However*
> Your XBee’s can share the same MY address, they’ll both receive the same data if it’s broadcasted to that address.

## Sample arduino Sketch from Sparkfun

>Double-Check Your XBee Network
>Before continuing with this example, you’ll need to make sure your XBee’s are configured correctly – they need to be on the same network and have compatible destination and MY addresses. By default, XBees will all be compatibly configured, but we recommend setting up unique network ID’s and addresses.

*uses the software Serial Library*

Passthrought to serial Monitor
```C
/*****************************************************************
XBee_Serial_Passthrough.ino

Set up a software serial port to pass data between an XBee Shield
and the serial monitor.

Hardware Hookup:
  The XBee Shield makes all of the connections you'll need
  between Arduino and XBee. If you have the shield make
  sure the SWITCH IS IN THE "DLINE" POSITION. That will connect
  the XBee's DOUT and DIN pins to Arduino pins 2 and 3.

*****************************************************************/
// We'll use SoftwareSerial to communicate with the XBee:
#include <SoftwareSerial.h>
// XBee's DOUT (TX) is connected to pin 2 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 3 (Arduino's Software TX)
SoftwareSerial XBee(2, 3); // RX, TX

void setup()
{
  // Set up both ports at 9600 baud. This value is most important
  // for the XBee. Make sure the baud rate matches the config
  // setting of your XBee.
  XBee.begin(9600);
  Serial.begin(9600);
}

void loop()
{
  if (Serial.available())
  { // If data comes in from serial monitor, send it out to XBee
    XBee.write(Serial.read());
  }
  if (XBee.available())
  { // If data comes in from XBee, send it out to serial monitor
    Serial.write(XBee.read());
  }
}
```

# Xbee Test

Es wurde eine einfache Konfiguration der XBees getestet.
Für die Variante 1 der XBees benötigt man keine gesonderte Konfiguration der einzelnen Module.

[Offizielle Arduino Shield und Xbee 1 Dokumnetatio](https://www.arduino.cc/en/Guide/ArduinoWirelessShield)

Der Taster auf den Shield (Micro und USB) schaltet die Standard-Serial-Schnittstelle zwischen dem Xbee Moudul und dem USB Port (PC) um.
Zum Flashen ist notweendig auf USB umzuschalten!


__TestCode__

```C
void setup()
{
  Serial.begin(9600);
}

void loop()
{
  Serial.print('H');
  delay(1000);
  Serial.print('L');
  delay(1000);
}
```

```C
const int ledPin = 13; // the pin that the LED is attached to
int incomingByte;      // a variable to read incoming serial data into

void setup() {
  // initialize serial communication:
  Serial.begin(9600);
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // see if there's incoming serial data:
  if (Serial.available() > 0) {
    // read the oldest byte in the serial buffer:
    incomingByte = Serial.read();
    // if it's a capital H (ASCII 72), turn on the LED:
    if (incomingByte == 'H') {
      digitalWrite(ledPin, HIGH);
    }
    // if it's an L (ASCII 76) turn off the LED:
    if (incomingByte == 'L') {
      digitalWrite(ledPin, LOW);
    }
  }
}
```

