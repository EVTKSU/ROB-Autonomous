#include <Arduino.h>
#include <ODriveTeensyCAN.h>

ODriveTeensyCAN odriveCAN;
static const int STEERING_AXIS_ID = 3;

void setup() {
    Serial.begin(115200);

    // Clear any existing errors on the axis
    odriveCAN.ClearErrors(STEERING_AXIS_ID);

    // Start full calibration
    odriveCAN.RunState(STEERING_AXIS_ID, ODriveTeensyCAN::AXIS_STATE_FULL_CALIBRATION_SEQUENCE);
    
    // Wait until calibration is finished (axis state returns to IDLE)
    while (odriveCAN.GetCurrentState(STEERING_AXIS_ID) != ODriveTeensyCAN::AXIS_STATE_IDLE) {
        delay(50);
    }

    // Optionally, if you need a homing step:
    odriveCAN.RunState(STEERING_AXIS_ID, ODriveTeensyCAN::AXIS_STATE_HOMING);
    while (odriveCAN.GetCurrentState(STEERING_AXIS_ID) != ODriveTeensyCAN::AXIS_STATE_IDLE) {
        delay(50);
    }

    // Finally, switch to closed-loop control
    odriveCAN.RunState(STEERING_AXIS_ID, ODriveTeensyCAN::AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("Steering axis is now in closed-loop control.");
}