#include "EVT_ODriver.h"
#include "EVT_RC.h" // For channels array
#include "EVT_StateMachine.h" // For SetErrorState function
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
    // Ensure no previous errors are interfering.
    odrive.clearErrors();
    Serial.println("Init Calibration Triggered via SBUS Channel 5!");

    // Capture the current encoder reading BEFORE doing any calibration.
    ODriveFeedback fb = odrive.getFeedback();
    float originalZero = fb.pos;
    Serial.print("Captured pre-calibration zero (desired center): ");
    Serial.println(String(originalZero, 2));
    delay(1000);  // Small delay for stability.

    // Perform motor calibration. Note that this step might move the motor.
    Serial.println("Running motor calibration...");
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();

    // Now run the encoder offset calibration.
    Serial.println("Running encoder offset calibration...");
    odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    // For diagnostic purposes, we read the post-calibration encoder value.
    fb = odrive.getFeedback();
    Serial.print("Post-calibration encoder reading (for diagnostics): ");
    Serial.println(String(fb.pos, 2));

    // Use the originally captured value as the steering zero offset.
    steeringZeroOffset = originalZero;
    Serial.print("Steering zero offset set to pre-calibration value: ");
    Serial.println(String(steeringZeroOffset, 2));
    lastTargetPosition = steeringZeroOffset;

    // Enable closed loop control with a retry loop.
    unsigned long startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime < 5000)) {
        odrive.clearErrors();
        odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
        Serial.println("Attempting to enable closed loop control...");
        delay(10);
    }

    // Set the controller's input mode to TRAP_TRAJ.
    Serial.println("Setting input mode to TRAP_TRAJ...");
    odrive_serial.println("w axis0.controller.config.input_mode 1");
    delay(100);

    Serial.println("ODrive calibration complete. Running in closed loop control at the pre-calibration zero.");
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
String odriveErrorToString(uint32_t err) {
    if (err == ODRIVE_ERROR_NONE) return "None";
    String s = "";
    if (err & ODRIVE_ERROR_INITIALIZING)                s += "Initializing, ";
    if (err & ODRIVE_ERROR_SYSTEM_LEVEL)                s += "System Level, ";
    if (err & ODRIVE_ERROR_TIMING_ERROR)                s += "Timing Error, ";
    if (err & ODRIVE_ERROR_MISSING_ESTIMATE)            s += "Missing Estimate, ";
    if (err & ODRIVE_ERROR_BAD_CONFIG)                  s += "Bad Config, ";
    if (err & ODRIVE_ERROR_DRV_FAULT)                   s += "DRV Fault, ";
    if (err & ODRIVE_ERROR_MISSING_INPUT)               s += "Missing Input, ";
    if (err & ODRIVE_ERROR_DC_BUS_OVER_VOLTAGE)         s += "DC Bus Over Voltage, ";
    if (err & ODRIVE_ERROR_DC_BUS_UNDER_VOLTAGE)        s += "DC Bus Under Voltage, ";
    if (err & ODRIVE_ERROR_DC_BUS_OVER_CURRENT)         s += "DC Bus Over Current, ";
    if (err & ODRIVE_ERROR_DC_BUS_OVER_REGEN_CURRENT)   s += "DC Bus Over Regen Current, ";
    if (err & ODRIVE_ERROR_CURRENT_LIMIT_VIOLATION)     s += "Current Limit Violation, ";
    if (err & ODRIVE_ERROR_MOTOR_OVER_TEMP)             s += "Motor Over Temp, ";
    if (err & ODRIVE_ERROR_INVERTER_OVER_TEMP)          s += "Inverter Over Temp, ";
    if (err & ODRIVE_ERROR_VELOCITY_LIMIT_VIOLATION)    s += "Velocity Limit Violation, ";
    if (err & ODRIVE_ERROR_POSITION_LIMIT_VIOLATION)    s += "Position Limit Violation, ";
    if (err & ODRIVE_ERROR_WATCHDOG_TIMER_EXPIRED)      s += "Watchdog Timer Expired, ";
    if (err & ODRIVE_ERROR_ESTOP_REQUESTED)             s += "Estop Requested, ";
    if (err & ODRIVE_ERROR_SPINOUT_DETECTED)            s += "Spinout Detected, ";
    if (err & ODRIVE_ERROR_BRAKE_RESISTOR_DISARMED)     s += "Brake Resistor Disarmed, ";
    if (err & ODRIVE_ERROR_THERMISTOR_DISCONNECTED)     s += "Thermistor Disconnected, ";
    if (err & ODRIVE_ERROR_CALIBRATION_ERROR)           s += "Calibration Error, ";
    if (s.endsWith(", ")) s = s.substring(0, s.length()-2);
    return s;
}

void printOdriveError() {
    // Retrieve the error code as integer from ODrive.
    uint32_t errorCode = odrive.getParameterAsInt("axis0.error");
    Serial.print("ODrive Error: ");
    Serial.println(odriveErrorToString(errorCode));
}

void odrvErrorCheck() {
    uint32_t errorCode = odrive.getParameterAsInt("axis0.error");
if (errorCode != ODRIVE_ERROR_NONE ) {
    String errorDescription = odriveErrorToString(errorCode);
    Serial.print("ODrive Error: ");
    Serial.println(errorDescription);
    SetErrorState(ERR_ODRIVE, errorDescription.c_str());
}


}
void updateOdrvControl() {
// Replace the errorStr code block with this:
//printOdriveError();

    // static unsigned long lastLedToggle = 0;
    // if (!systemInitialized) {
    //     if (millis() - lastLedToggle > 500) {
    //         lastLedToggle = millis();
    //         digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    //     }
    // } else {
    //     digitalWrite(STATUS_LED_PIN, HIGH);
    // }
    // commented the above out as it might be blocking the loop and delaying odrive update
    int ch_clear = channels[5];
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
