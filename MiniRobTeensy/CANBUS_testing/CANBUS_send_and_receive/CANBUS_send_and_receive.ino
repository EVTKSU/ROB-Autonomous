#include <Arduino.h>

void setup() {
  // Initialize pins 21, 22, and 23 as outputs
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(23, OUTPUT);

  // Set the pins HIGH
  digitalWrite(21, HIGH);
  digitalWrite(22, HIGH);
  digitalWrite(23, HIGH);
}

void loop() {
  // The loop is empty since the pins are set once in setup.
}
