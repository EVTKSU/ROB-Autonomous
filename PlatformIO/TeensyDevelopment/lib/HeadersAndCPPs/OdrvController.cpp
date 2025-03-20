#include "TheH-File.h"

// ODrive uses Serial6.
HardwareSerial &odrive_serial = Serial6;
ODriveUART odrive(odrive_serial);

// ODrive control globals.
bool systemInitialized = false;
float lastTargetPosition = 0.0f;
float steeringZeroOffset = 0.0f;

static bool errorClearFlag = false;
static unsigned long lastSteerUpdateTime = 0;
static float currentSteeringOffset = 0.0f;
static unsigned long lastPrintTime = 0;

// Performs calibration and forces the steering zero offset.
void initCalibration() {
    if (Serial) {
        Serial.print("Init Calibration Triggered via SBUS Channel 5!\r\n");
        Serial.print("Starting motor calibration...\r\n");
    }
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    // Read pre-calibration steering value.
    ODriveFeedback fb = odrive.getFeedback();
    float preCalZero = fb.pos; // i undid the override it fucked it all up
    if (Serial) {
        Serial.print("Pre-calibration steering zero: ");
        Serial.print(String(preCalZero, 2));
        Serial.print("\r\n");
    }
    delay(3000);
    
    // Encoder offset calibration.
    if (Serial) {
        Serial.print("Calibrating steering (encoder offset calibration)...\r\n");
    }
    odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    // Post-calibration reading.
    fb = odrive.getFeedback();
    if (Serial) {
        Serial.print("Post-calibration steering reading: ");
        Serial.print(String(fb.pos, 2));
        Serial.print("\r\n");
    }
    
    // Force steering zero offset to pre-calibration value.
    steeringZeroOffset = preCalZero;
    if (Serial) {
        Serial.print("Steering zero offset forced to: ");
        Serial.print(String(steeringZeroOffset, 2));
        Serial.print("\r\n");
    }
    
    lastTargetPosition = steeringZeroOffset;
    
    // Enable closed loop control.
    unsigned long startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime < 5000)) {
        odrive.clearErrors();
        odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
        if (Serial) {
            Serial.print("Trying again to enable closed loop control\r\n");
        }
        delay(10);
    }
    
    if (Serial) {
        Serial.print("Setting input mode to TRAP_TRAJ...\r\n");
    }
    odrive_serial.println("w axis0.controller.config.input_mode 1"); // trap traj mode
    delay(100);
    odrive_serial.println("w axis0.controller.config.vel_limit 30.0"); // velocity limit
    delay(100);
    odrive_serial.println("w axis0.controller.config.accel_limit 25.0"); // acceleration limit
    delay(100);
    odrive_serial.println("w axis0.controller.config.decel_limit 50.0"); // deceleration limit
    delay(100);
    odrive_serial.println("w axis0.motor.config.current_lim 80.0"); // current limit at motor level
    delay(100);
    odrive_serial.println("w axis0.controller.config.current_lim 90.0"); // current limit of controller 
    delay(100);
    
    if (Serial) {
        Serial.print("ODrive calibration complete and running!\r\n");
    }
}

void setupOdrv() {
    odrive_serial.begin(115200);
    if (Serial) {
        Serial.print("Established ODrive communication\r\n");
    }
    delay(500);
    if (Serial) {
        Serial.print("Waiting for ODrive...\r\n");
    }
    unsigned long startTimeOD = millis();
    while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTimeOD < 15000)) {
        delay(100);
    }
    if (odrive.getState() == AXIS_STATE_UNDEFINED) {
        if (Serial) {
            Serial.print("ODrive not found! Proceeding without ODrive.\r\n");
        }
    } else {
        if (Serial) {
            Serial.print("Found ODrive! Waiting for calibration trigger via SBUS channel 5...\r\n");
        }
    }
    if (Serial) {
        Serial.print("ODrive setup complete. System idle until calibration.\r\n");
    }
}

void updateOdrvControl() {
    // Update LED status based on calibration.
    static unsigned long lastLedToggle = 0;
    if (!systemInitialized) {
        if (millis() - lastLedToggle > 500) {
            lastLedToggle = millis();
            digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        }
    } else {
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
    
    // Handle error clearing and recalibration (SBUS channel 4).
    int ch_clear = channels[4];
    if (ch_clear > 1500 && !errorClearFlag) {
        errorClearFlag = true;
        if (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
            if (Serial) {
                Serial.print("ODrive fault detected via SBUS channel 4. Recalibrating...\r\n");
            }
            odrive.clearErrors();
            systemInitialized = false;
            initCalibration();
            systemInitialized = true;
        } else {
            if (Serial) {
                Serial.print("SBUS channel 4 activated: Clearing ODrive errors.\r\n");
            }
            odrive.clearErrors();
        }
    }
    if (ch_clear < 1500 && errorClearFlag) {
        errorClearFlag = false;
    }
    
    // Trigger calibration if not yet initialized (SBUS channel 5).
    if (!systemInitialized) {
        if (channels[5] > 900) {
            initCalibration();
            systemInitialized = true;
        } else {
            // One-time calibration message; not part of the continuously updated block.
            Serial.print("Waiting for calibration trigger, SBUS channel 5: ");
            Serial.print(String(channels[5]));
            Serial.print("\r\n");
            return;
        }
    }
    
  
    
    // RC mode: ODrive steering control using SBUS channel 3.
    const unsigned long steerHoldTime = 300;
    const float steeringDecayRate = 0.005f;
    int ch_steer = channels[3];
    if (ch_steer < 1200 || ch_steer > 1260) {
        float desiredOffset = 0.0f;
        if (ch_steer < 1200) {
            float normalized = (1200.0f - ch_steer) / float(1200 - 410);
            desiredOffset = -normalized * MAX_STEERING_OFFSET;
        } else {
            float normalized = (ch_steer - 1260.0f) / float(1811 - 1260);
            desiredOffset = normalized * MAX_STEERING_OFFSET;
        }
        currentSteeringOffset = desiredOffset;
        lastSteerUpdateTime = millis();
    } else {
        if (millis() - lastSteerUpdateTime > steerHoldTime) {
            if (currentSteeringOffset > 0.0f) {
                currentSteeringOffset -= steeringDecayRate;
                if (currentSteeringOffset < 0.0f)
                    currentSteeringOffset = 0.0f;
            } else if (currentSteeringOffset < 0.0f) {
                currentSteeringOffset += steeringDecayRate;
                if (currentSteeringOffset > 0.0f)
                    currentSteeringOffset = 0.0f;
            }
        }
    }
    lastTargetPosition = steeringZeroOffset + currentSteeringOffset;
    odrive.setPosition(lastTargetPosition, 27.0f);
    
    // Instead of rolling prints, update the debug string once every ~100 ms.
    if (millis() - lastPrintTime > 100) {
        ODriveFeedback fb = odrive.getFeedback();
        odrvDebug = "Steering Target: " + String(lastTargetPosition, 2) +
                    " | ODrive Pos: " + String(fb.pos, 2) +
                    " | CH3: " + String(channels[3]);
        lastPrintTime = millis();
    }
}
