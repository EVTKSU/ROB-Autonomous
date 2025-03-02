#include <Arduino.h>
#include <vesc_can.h>  // Make sure this header is in your lib/vesc_can folder

// Define the VESC controller ID to monitor (must be < MAX_NODE_ID as defined in vesc_can.h)
#define VESC_CONTROLLER_ID 1

// Define the number of motor poles for this controller.
// This is used for converting between erpm and rpm.
#define MOTOR_POLES 14

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for the serial port to initialize
  }
  Serial.println("Starting VESC CAN Read Demo");

  // Initialize the CAN bus for VESC communication.
  vesc_can_begin();
  Serial.println("CAN bus initialized.");

  // Set the number of motor poles for the controller.
  vesc_can_set_motor_poles(VESC_CONTROLLER_ID, MOTOR_POLES);
}

void loop() {
  // Poll the CAN bus for incoming messages.
  // This updates the global status for the VESC(s) connected.
  vesc_can_read();

  // Read telemetry data from the VESC.
  float erpm = vesc_can_get_erpm(VESC_CONTROLLER_ID);
  float rpm = vesc_can_get_rpm(VESC_CONTROLLER_ID);
  float current = vesc_can_get_current(VESC_CONTROLLER_ID);
  float voltage = vesc_can_get_voltage(VESC_CONTROLLER_ID);

  // Print the telemetry data to the Serial Monitor.
  Serial.print("Controller ");
  Serial.print(VESC_CONTROLLER_ID);
  Serial.print(" - eRPM: ");
  Serial.print(erpm);
  Serial.print(" | RPM: ");
  Serial.print(rpm);
  Serial.print(" | Current: ");
  Serial.print(current);
  Serial.print(" A | Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");

  // Wait 1 second before polling again.
  delay(1000);
}
