#include <Arduino.h>
#include "EVT_RC.h"
#include "EVT_ODriver.h"

void setup() {
    Serial.begin(9600);
    delay(1000); // Wait for Serial Monitor to open
    Serial.println("Initializing ODrive and SBUS...");
    pinMode(STATUS_LED_PIN, OUTPUT); // Set the status LED pin as output
    digitalWrite(STATUS_LED_PIN, LOW); // Turn on the status LED
    setupOdrv(); // Initialize ODrive communication
    setupSbus();
    delay(1000);
    updateSbusData();
    initCalibration (); // Initialize ODrive calibration
}
void loop() {
    // Call the function that updates steering control.
    updateOdrvControl();
    
    // Read the current feedback from the ODrive encoder.
    ODriveFeedback fb = odrive.getFeedback();
    
    Serial.print(" | Encoder Value: ");
    Serial.println(fb.pos, 2);
    
    delay(100); // Adjust delay as needed for testing.
}