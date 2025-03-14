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
        Serial.println("Init Calibration Triggered via SBUS Channel 5!");
        Serial.println("Starting motor calibration...");
    }
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    // Read pre-calibration steering value.
    ODriveFeedback fb = odrive.getFeedback();
    float preCalZero = fb.pos;
    if (Serial) {
        Serial.print("Pre-calibration steering zero: ");
        Serial.println(preCalZero, 2);
    }
    delay(3000);
    
    // Encoder offset calibration.
    if (Serial) {
        Serial.println("Calibrating steering (encoder offset calibration)...");
    }
    odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    // Post-calibration reading.
    fb = odrive.getFeedback();
    if (Serial) {
        Serial.print("Post-calibration steering reading: ");
        Serial.println(fb.pos, 2);
    }
    
    // Force steering zero offset to pre-calibration value.
    steeringZeroOffset = preCalZero;
    if (Serial) {
        Serial.print("Steering zero offset forced to: ");
        Serial.println(steeringZeroOffset, 2);
    }
    
    lastTargetPosition = steeringZeroOffset;
    
    // Enable closed loop control.
    unsigned long startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime < 5000)) {
        odrive.clearErrors();
        odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
        if (Serial) {
            Serial.println("Trying again to enable closed loop control");
        }
        delay(10);
    }
    
    if (Serial) {
        Serial.println("Setting input mode to TRAP_TRAJ...");
    }
    odrive_serial.println("w axis0.controller.config.input_mode 1"); // trap traj mode
    delay(100);
    odrive_serial.println("w axis0.controller.config.vel_limit 30.0"); // velocity limit
    delay(100);
    odrive_serial.println("w axis0.controller.config.accel_limit 25.0"); // acceleration limit
    delay(100);
    odrive_serial.println("w axis0.controller.config.decel_limit 50.0"); // decelleration limit
    delay(100);
    odrive_serial.println("w axis0.motor.config.current_lim 80.0"); //  current limit at motor level
    delay(100);
    odrive_serial.println("w axis0.controller.config.current_lim 90.0"); // current limit of controller 
    delay(100);
    
    if (Serial) {
        Serial.println("ODrive calibration complete and running!");
    }
}

void setupOdrv() {
    odrive_serial.begin(115200);
    if (Serial) {
        Serial.println("Established ODrive communication");
    }
    delay(500);
    if (Serial) {
        Serial.println("Waiting for ODrive...");
    }
    unsigned long startTimeOD = millis();
    while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTimeOD < 15000)) {
        delay(100);
    }
    if (odrive.getState() == AXIS_STATE_UNDEFINED) {
        if (Serial) {
            Serial.println("ODrive not found! Proceeding without ODrive.");
        }
    } else {
        if (Serial) {
            Serial.println("Found ODrive! Waiting for calibration trigger via SBUS channel 5...");
        }
    }
    if (Serial) {
        Serial.println("ODrive setup complete. System idle until calibration.");
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
                Serial.println("ODrive fault detected via SBUS channel 4. Recalibrating...");
            }
            odrive.clearErrors();
            systemInitialized = false;
            initCalibration();
            systemInitialized = true;
        } else {
            if (Serial) {
                Serial.println("SBUS channel 4 activated: Clearing ODrive errors.");
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
            if (Serial) {
                Serial.print("Waiting for calibration trigger, SBUS channel 5: ");
                Serial.println(channels[5]);
            }
            return;
        }
    }
    
    // ODrive steering control using SBUS channel 3.
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
    odrive.setPosition(lastTargetPosition, 20.0f);
    
    // Debug output every ~100 ms.
    if (millis() - lastPrintTime > 100) {
        ODriveFeedback fb = odrive.getFeedback();
        if (Serial) {
            Serial.print("Steering Target: ");
            Serial.print(lastTargetPosition, 2);
            Serial.print(" | ODrive Pos: ");
            Serial.print(fb.pos, 2);
            Serial.print(" | CH3: ");
            Serial.print(channels[3]);
            Serial.println();
        }
        lastPrintTime = millis();
    }
}
