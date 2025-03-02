#include <Arduino.h>
#include <ODriveTeensyCAN.h>
#include <SBUS.h>

// ----------------------------------------------
// Constants
// ----------------------------------------------
static const uint8_t  SBUS_STEERING_CH   = 3;     // SBUS Channel 4 => index 3
static const uint16_t SBUS_IN_MIN        = 172;   // Typical SBUS min
static const uint16_t SBUS_IN_MAX        = 1811;  // Typical SBUS max

// Desired ±35° at the steering output, with a 27:1 gear ratio
static const float     STEERING_RANGE_DEG = 35.0f;  // degrees from center
static const float     GEAR_RATIO         = 27.0f;  // motor turns : 1 output turn

// Calculate how many motor turns correspond to 35° at output
// 35° = 35/360 of one output revolution => times GEAR_RATIO => ~2.625 turns
static const float     STEERING_TURNS_MAX = (STEERING_RANGE_DEG / 360.0f) * GEAR_RATIO;

// ODrive axis ID
static const int STEERING_AXIS_ID = 3;

// ----------------------------------------------
// ODrive & SBUS objects
// ----------------------------------------------
ODriveTeensyCAN odriveCAN;           // ODriveTeensyCAN
SBUS sbus(Serial1);                  // SBUS on Serial1 (RX1)
uint16_t sbusChannels[16] = {0};
bool sbusFailsafe = false;
bool sbusLostFrame = false;

// ----------------------------------------------
// Maps raw SBUS (172..1811) to -1..+1
// ----------------------------------------------
float mapSbusToNormalized(int sbusValue)
{
    float norm = (static_cast<float>(sbusValue) - SBUS_IN_MIN) 
               / (SBUS_IN_MAX - SBUS_IN_MIN);
    // This yields 0..1, so shift to -1..+1:
    return (norm * 2.0f) - 1.0f;
}

// ----------------------------------------------
// Setup
// ----------------------------------------------
void setup()
{
    Serial.begin(115200);
    sbus.begin();

    // Assuming the ODrive axis is already calibrated,
    // move it to closed-loop control.
    odriveCAN.RunState(STEERING_AXIS_ID, ODriveTeensyCAN::AXIS_STATE_CLOSED_LOOP_CONTROL);

    Serial.println("ODrive in closed-loop control for steering.");
}

// ----------------------------------------------
// Loop
// ----------------------------------------------
void loop()
{
    // Check for new SBUS data
    if (sbus.read(&sbusChannels[0], &sbusFailsafe, &sbusLostFrame))
    {
        // Channel 4 value is sbusChannels[3]
        float steerRatio = mapSbusToNormalized(sbusChannels[SBUS_STEERING_CH]);

        // Map -1..+1 to the motor turns needed for ±35° steering
        float desiredMotorTurns = steerRatio * STEERING_TURNS_MAX;

        // Command position on the steering axis
        odriveCAN.SetPosition(STEERING_AXIS_ID, desiredMotorTurns);
    }

    delay(10);  // Slight delay to avoid sending messages too quickly
}
