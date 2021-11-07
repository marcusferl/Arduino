#include <Arduino.h>

#define BUTTON_INPUT 14
#define RELAY 4
int val;

void setup() {
  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON_INPUT, INPUT);
}

void loop() {
  val = digitalRead(BUTTON_INPUT);
  if (val == HIGH)
  {
    digitalWrite(RELAY, LOW);
  }
  else
  {
    digitalWrite(RELAY, HIGH);
  }
  
  
}