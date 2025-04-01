#include "EVT_ODriver.h"
#include "EVT_RC.h" // For channels array

// Create a reference to Serial6 for ODrive.
HardwareSerial &odrive_serial = Serial6;

// Define the ODriveUART object.
ODriveUART odrive(odrive_serial);

// Define global variables.
bool systemInitialized = false;
String odrvDebug = "";  // Added definition for odrvDebug.
float lastTargetPosition = 0.0f;
float steeringZeroOffset = 0.0f;

static bool errorClearFlag = false;
static unsigned long lastSteerUpdateTime = 0;
static float currentSteeringOffset = 0.0f;
static unsigned long lastPrintTime = 0;

void initCalibration() {
    Serial.println("Init Calibration Triggered via SBUS Channel 5!");
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    ODriveFeedback fb = odrive.getFeedback();
    float preCalZero = fb.pos;
    Serial.print("Pre-calibration steering zero: ");
    Serial.println(String(preCalZero, 2));
    delay(3000);
    
    // Serial.println("Calibrating steering (encoder offset calibration)...");
    // odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    // delay(4000);
    // as of now we don't need these above^^
    odrive.clearErrors(); // just for safety? 
    
    fb = odrive.getFeedback();
    Serial.print("Post-calibration steering reading: ");
    Serial.println(String(fb.pos, 2));
    
    steeringZeroOffset = preCalZero;
    Serial.print("Steering zero offset forced to: ");
    Serial.println(String(steeringZeroOffset, 2));
    
    lastTargetPosition = steeringZeroOffset;
    
    unsigned long startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime < 5000)) {
        odrive.clearErrors();
        odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
        Serial.println("Trying again to enable closed loop control");
        delay(10);
    }
    
    Serial.println("Setting input mode to TRAP_TRAJ...");
    odrive_serial.println("w axis0.controller.config.input_mode 1");
    delay(100);
    
    Serial.println("ODrive calibration complete and running!");
}

void setupOdrv() {
    odrive_serial.begin(115200);
    Serial.println("Established ODrive communication");
    delay(500);
    Serial.println("Waiting for ODrive...");
    unsigned long startTimeOD = millis();
    while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTimeOD < 15000)) {
        delay(100);
    }
    if (odrive.getState() == AXIS_STATE_UNDEFINED) {
        Serial.println("ODrive not found! Proceeding without ODrive.");
    } else {
        Serial.println("Found ODrive! Waiting for calibration trigger via SBUS channel 5...");
    }
    Serial.println("ODrive setup complete. System idle until calibration.");
}

void updateOdrvControl() {
    static unsigned long lastLedToggle = 0;
    if (!systemInitialized) {
        if (millis() - lastLedToggle > 500) {
            lastLedToggle = millis();
            digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        }
    } else {
        digitalWrite(STATUS_LED_PIN, HIGH);
    }
    
    int ch_clear = channels[4];
    if (ch_clear > 1500 && !errorClearFlag) {
        errorClearFlag = true;
        if (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
            Serial.println("ODrive fault reset trigger hit. Recalibrating..."); // serials out that it saw we hit da switch to recalibrate
            odrive.clearErrors();
            systemInitialized = false;
            initCalibration();
            systemInitialized = true;
        } else {
            Serial.println("SBUS channel 4 activated: Clearing ODrive errors.");
            odrive.clearErrors();
        }
    }
    if (ch_clear < 1500 && errorClearFlag) {
        errorClearFlag = false;
    }
    
    if (!systemInitialized) {
        if (channels[5] > 900) {
            initCalibration();
            systemInitialized = true;
        } else {
            Serial.print("Waiting for calibration trigger, SBUS channel 5: ");
            Serial.println(String(channels[5]));
            return;
        }
    }
    
    const unsigned long steerHoldTime = 300;
    const float steeringDecayRate = 0.005f;
    int ch_steer = channels[3];
    if (ch_steer < 1200 || ch_steer > 1260) {
        float desiredOffset = 0.0f;
        if (ch_steer < 1200) {
            float normalized = (1200.0f - ch_steer) / float(1200 - 410);
            desiredOffset = -normalized * 2.4f;
        } else {
            float normalized = (ch_steer - 1260.0f) / float(1811 - 1260);
            desiredOffset = normalized * 2.4f;
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
    
    static unsigned long localPrintTime = lastPrintTime;
    if (millis() - localPrintTime > 100) {
        ODriveFeedback fb = odrive.getFeedback();
        odrvDebug = "Steering Target: " + String(lastTargetPosition, 2) +
                    " | ODrive Pos: " + String(fb.pos, 2) +
                    " | CH3: " + String(channels[3]);
        localPrintTime = millis();
    }
}
