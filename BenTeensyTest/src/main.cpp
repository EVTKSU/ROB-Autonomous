#include <Arduino.h>
#include "ODriveTeensyCAN.h"

// Instantiate the ODrive CAN object
ODriveTeensyCAN odrive;

// Debug method to check for any CAN messages and print confirmation via Serial
void debugCANInputs() {
  bool canReceived = odrive.readAsyncMessages();
  if (canReceived) {
    Serial.println("DEBUG: CAN message received.");
  } else {
    Serial.println("DEBUG: No CAN messages received.");
  }
}

void setup() {
  // Initialize the serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect (if needed)
  }
  Serial.println("ODrive Voltage Monitor Started");
}

void loop() {
  // Request bus voltage and current data from axis 0.
  // (Change the axis id if you have multiple axes)
  odrive.RequestBusVoltageCurrent(0);
  
  // Call the debug method to check for any CAN inputs.
  debugCANInputs();
  
  // Retrieve the last bus voltage measurement for axis 0.
  float voltage = odrive.GetLastBusVoltage(0);
  
  // Print the bus voltage to the serial monitor
  Serial.print("Bus Voltage: ");
  Serial.print(voltage, 4);  // print with 4 decimal places
  Serial.println(" V");
  
  // Update every second (adjust as needed)
  delay(1000);
}
