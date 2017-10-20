#define TRIGGER_PIN  3
#define ECHO_PIN     2
#define MAX_DISTANCE 300
#include "NewPing.h"
    NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
    void setup() {
      Serial.begin(9600);
    }
     
    void loop() {
      delay(50);
      Serial.print("Ping: ");
      Serial.print(sonar.ping_cm());
      Serial.println("cm");
    }