#include "TheH-File.h"
extern bool systemInitialized;  // Already declared in TheH-File.h

// Initialize the current state to IDLE.
SystemState currentState = STATE_IDLE;

void updateStateMachine() {
    // Check emergency condition first: if SBUS channel 6 exceeds threshold, engage emergency.
    if (channels[4] > ESTOP_THRESHOLD) {
        // Set the emergency relay pin high.
        digitalWrite(ESTOP_PIN, HIGH);
        currentState = STATE_EMERGENCY;
    } else {
        // Ensure the emergency relay is disengaged when not in emergency.
        digitalWrite(ESTOP_PIN, LOW);
        // Determine normal state based on system initialization and RC mode.
        if (!systemInitialized) {
            currentState = STATE_IDLE;
        } else {
            // Use SBUS channel 7 to choose between RC and Autonomous.
            if (channels[7] > 1000) {
                currentState = STATE_RC;
            } else {
                currentState = STATE_AUTONOMOUS;
            }
        }
    }
    
    // Process actions based on the current state.
    switch (currentState) {
        case STATE_IDLE:
            Serial.println("State: IDLE");
            // Insert idle state logic (disable outputs, wait for calibration, etc.)
            break;
            
        case STATE_RC:
            Serial.println("State: RC Mode");
            // Insert RC mode logic here (update VESC/ODrive controls with RC parameters)
            break;
            
        case STATE_AUTONOMOUS:
            Serial.println("State: Autonomous Mode");
            // Insert autonomous control logic here.
            break;
            
        case STATE_EMERGENCY:
            Serial.println("State: EMERGENCY");
            // Emergency logic: disable all motor outputs, trigger alarms, etc.
            break;
            
        default:
            Serial.println("State: Unknown");
            break;
    }
}
