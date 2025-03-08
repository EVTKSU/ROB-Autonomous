
//this program reads SBUS channel packets and outputs the values to the serial monitor.
//it's shrimple and easy to understand.


#include <Arduino.h>
#include <SBUS.h>
#include "ODriveTeensyCAN.h"


// SBUS Configuration:

SBUS sbus(Serial2);
uint16_t channels[10]; // define quantity of SBUS channels, based on how many channels transmitter has. ours has 10.
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// Setup
void setup() {
  Serial.begin(115200);

  // Initialize Serial1 for SBUS (handled by library).
  sbus.begin();

  // Wait a moment to ensure SBUS initialization is stable.
  delay(500);
}
  
// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
void loop() {
  // Continuously read SBUS frames
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
   

    // Print all SBUS channel values, using carriage return so you can actually see it.
    Serial.print("CH0: ");
    Serial.print(channels[0]);
    Serial.print("  CH1: ");
    Serial.print(channels[1]);
    Serial.print("  CH2: ");
    Serial.print(channels[2]);
    Serial.print("  CH3: ");
    Serial.print(channels[3]);
    Serial.print("  CH4: ");
    Serial.print(channels[4]);
    Serial.print("  CH5: ");
    Serial.print(channels[5]);
    Serial.print("  CH6: ");
    Serial.print(channels[6]);
    Serial.print("  CH7: ");
    Serial.print(channels[7]);
    Serial.print("  CH8: ");
    Serial.print(channels[8]);
    Serial.print("  CH9: ");
    Serial.print(channels[9]);
    Serial.print("\r");   // carriage return for same-line updates
    Serial.flush();       // force output to appear immediately
  }

  // Example refresh rate for steering
  delay(10);
}
