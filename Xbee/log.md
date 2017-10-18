# Entry 1 - 18.10.2017 17:00

## XBee Communication over serial with PC / Raspberry

Simple Sender and Receiver (Xbee communication)
The Xbee receiver does receive data from the sender and drives a simple onboard LED.
Furthermore the receiver sends every incoming byte over a software Serial (Pins 10 and 11).
A second Arduino is currently used (also with a second software serial) in order to receive
the data and transmit it over the hardware (USB) serial to the PC.

TBD: Attach the receiver directly to the Raspberry PI.




