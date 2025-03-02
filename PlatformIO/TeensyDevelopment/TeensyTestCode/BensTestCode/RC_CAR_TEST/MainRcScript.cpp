#include <Arduino.h>
#include <SBUS.h>
#include "ODriveTeensyCAN.h"


// SBUS Configuration:

SBUS sbus(Serial1);
uint16_t channels[10]; // define quantity of SBUS channels, based on how many channels transmitter has. ours has 10.
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// The teensy 4.1 has a built in CAN controller, you only need the transciever
// Connect MCP2551 TX/RX to Teensy pins 23/22, plus 3.3V and GND.
ODriveTeensyCAN odrive;

// The axis ID on the ODrive for your steering motor.
// If you have only one motor (axis0), then axis_id = 0.
const int AXIS_ID = 0;  

// ----------------------------------------------------
// Steering Channel Configuration
// ----------------------------------------------------
// This is the SBUS channel index for steering (0-based).
// Check your actual RC transmitter/receiver channel assignment.
const int STEERING_CHANNEL = 0;

// Raw SBUS range is often ~172 (min) to ~1811 (max).
// Adjust these if your particular radio is different.
const float SBUS_MIN = 172.0f;
const float SBUS_MAX = 1811.0f;

// Desired motor position range in turns BEFORE the gearbox.
// if the gearbox is 25 to 1 ratio, in theory 12 turns will make the output shaft move juust under 1/2 rotation in each direction.
// I genuinely have NO idea why the F is there but if you remove it the code breaks.
const float MOTOR_POS_MIN = -12.0f;     
const float MOTOR_POS_MAX =  12.0f;

// ----------------------------------------------------
// Helper Function: map SBUS to motor position
// ----------------------------------------------------
float mapSBUSToMotorPosition(float sbusValue) {
  float sbusRange = SBUS_MAX - SBUS_MIN;      // nominally ~1639
  float motorRange = MOTOR_POS_MAX - MOTOR_POS_MIN; // (2 - (-2)) = 4
  float normalized = (sbusValue - SBUS_MIN) / sbusRange; // 0.0 to 1.0
  float mappedPos = (normalized * motorRange) + MOTOR_POS_MIN; // -2 to +2
  return mappedPos;
}

// ----------------------------------------------------
// Setup
// ----------------------------------------------------
void setup() {
  // For serial monitoring of channel data
  Serial.begin(115200);

  // Initialize Serial1 for SBUS (handled by library).
  sbus.begin();

  // Wait a moment to ensure CAN initialization is stable.
  delay(500);

  // ------------------------------------------------------
  // Example: put the ODrive axis in closed-loop control.
  // You can run calibrations or set other states as needed.
  // Make sure your ODrive is already configured for CAN,
  // and that your axis is calibrated if required.
  // ------------------------------------------------------
  // For instance, request state AXIS_STATE_CLOSED_LOOP_CONTROL = 8
  odrive.RunState(AXIS_ID, ODriveTeensyCAN::AXIS_STATE_CLOSED_LOOP_CONTROL);

  // Optional: set a velocity limit (units of turns/s).
  odrive.SetVelocityLimit(AXIS_ID, 10.0f);

  // Small delay to ensure ODrive processes state change
  delay(500);
}

// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
void loop() {
  // Continuously read SBUS frames
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    // Retrieve the raw steering value from the assigned channel
    uint16_t steerRaw = channels[STEERING_CHANNEL];

    // Map SBUS value to motor position (in turns)
    float targetPosition = mapSBUSToMotorPosition((float)steerRaw);

    // Send the position command to ODrive via CAN
    odrive.SetPosition(AXIS_ID, targetPosition);

  }

  // Example refresh rate for steering
  delay(10);
}
