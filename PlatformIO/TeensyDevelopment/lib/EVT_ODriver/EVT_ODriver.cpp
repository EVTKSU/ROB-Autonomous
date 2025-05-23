
#include "EVT_ODriver.h"
#include "EVT_RC.h"           // For channels array
#include "EVT_StateMachine.h" // For SetErrorState function
// Create a reference to Serial6 for ODrive.
HardwareSerial &odrive_serial = Serial6;

// Define the ODriveUART object.
ODriveUART odrive(odrive_serial);

// Define global variables.
bool systemInitialized = false;
String odrvDebug = "";  // Added definition for odrvDebug.
float lastTargetPosition = 0.0f;
float steeringZeroOffset  = 0.0f;  // Will be set to the midpoint (‑0.665) after calibration.

static bool   errorClearFlag          = false;
static float  currentSteeringOffset   = 0.0f;

// -------------------------------------------------------------------------------------------------
//                                   CALIBRATION ROUTINES
// -------------------------------------------------------------------------------------------------
void initCalibration() {
    // Ensure no previous errors are interfering.
    odrive.clearErrors();
    Serial.println("Init Calibration Triggered via SBUS Channel 5!");

    // Capture the current encoder reading BEFORE doing any calibration.
    ODriveFeedback fb = odrive.getFeedback();
    float originalZero = fb.pos;
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

    // For diagnostic purposes, we read the post‑calibration encoder value.
    fb = odrive.getFeedback();
    Serial.print("Post‑calibration encoder reading (for diagnostics): ");
    Serial.println(String(fb.pos, 2));

    // Use the captured value for diagnostics only; the functional zero will be set later.
    steeringZeroOffset = originalZero;
    Serial.print("Steering zero offset set to pre‑calibration value: ");
    Serial.println(String(steeringZeroOffset, 2));
    lastTargetPosition = steeringZeroOffset;

    // Enable closed‑loop control with a retry loop.
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

    Serial.println("ODrive calibration complete. Running in closed loop control at the pre‑calibration zero.");
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

// -------------------------------------------------------------------------------------------------
//                                   ERROR‑HANDLING HELPERS
// -------------------------------------------------------------------------------------------------
String odriveErrorToString(uint32_t err) {
    if (err == ODRIVE_ERROR_NONE) return "None";
    String s = "";
    for (const auto& [errorCode, errorString] : odrvErrorMap) {
        if (err & errorCode) {
            if (s != "") s += ", ";
            s += errorString;
        }
    }
    return s;
}

void printOdriveError() {
    uint32_t errorCode = odrive.getParameterAsInt("axis0.error");
    Serial.print("ODrive Error: ");
    Serial.println(odriveErrorToString(errorCode));
}

void odrvErrorCheck() {
    uint32_t errorCode = odrive.getParameterAsInt("axis0.error");
    if (errorCode != ODRIVE_ERROR_NONE) {
        String errorDescription = odriveErrorToString(errorCode);
        Serial.print("ODrive Error: ");
        Serial.println(errorDescription);
        SetErrorState(ERR_ODRIVE, errorDescription.c_str());
    }
}

// -------------------------------------------------------------------------------------------------
//                                         CONTROL LOOP
// -------------------------------------------------------------------------------------------------
void updateOdrvControl() {
    // Uncomment the line below for verbose error tracking
    // printOdriveError();

    // Handle SBUS channel 5 for error clear / re‑cal.
    int ch_clear = channels[5];
    if (ch_clear > 1500 && !errorClearFlag) {
        errorClearFlag = true;
        if (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
            Serial.println("ODrive fault reset trigger hit. Recalibrating...");
            odrive.clearErrors();
            systemInitialized = false;
            initCalibration();
            systemInitialized = true;
        } else {
            Serial.println("SBUS channel 4 activated: Clearing ODrive errors.");
            odrive.clearErrors();
            initCalibration();
        }
    }
    if (ch_clear < 1500 && errorClearFlag) {
        errorClearFlag = false;
    }

    // Wait for first‑time calibration.
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

    // ----------------------------------------------------------
    // NEW STEERING MAPPING (no decay)
    // ----------------------------------------------------------

    // Encoder extrema and derived midpoint
    const float MAX_LEFT_POS  =  -2.33f;   // Hard left encoder reading
    const float MAX_RIGHT_POS = 1.0f;  // Hard right encoder reading
    const float MID_POS       = (MAX_LEFT_POS + MAX_RIGHT_POS) / 2.0f;   // -0.665
    const float MAX_OFFSET    = MAX_LEFT_POS - MID_POS;                  // 1.665 (magnitude)

    // One‑time midpoint assignment after calibration
    static bool midpointSet = false;
    if (!midpointSet) {
        steeringZeroOffset  = MID_POS;
        lastTargetPosition  = steeringZeroOffset;
        midpointSet = true;
    }

    // Read steering channel (CH3)
    int ch_steer = channels[3];

    if (ch_steer < 1200) {
        // Map 410‑>MAX_LEFT_POS, 1200‑>MID_POS
        float normalized = (1200.0f - ch_steer) / float(1200 - 410); // 0‑>1
        currentSteeringOffset =  normalized * MAX_OFFSET;

    } else if (ch_steer > 1260) {
        // Map 1260‑>MID_POS, 1811‑>MAX_RIGHT_POS
        float normalized = (ch_steer - 1260.0f) / float(1811 - 1260);
        currentSteeringOffset = -normalized * MAX_OFFSET;

    } else {
        // Dead‑band – snap to centre
        currentSteeringOffset = 0.0f;
    }

    // Compute and send target
    lastTargetPosition = steeringZeroOffset + currentSteeringOffset;
    odrive.setPosition(lastTargetPosition, 0.0f);   // zero velocity‑FF to avoid overshoot

    // Non‑blocking debug print every 1000 ms with carriage return.
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 1000) {
        ODriveFeedback fb = odrive.getFeedback();
        odrvDebug = String("Steering Target: ") + String(lastTargetPosition, 2) +
                    " | ODrive Pos: " + String(fb.pos, 2) +
                    " | CH3: " + String(ch_steer);
                   
                    
        Serial.print(odrvDebug + "\r");
        lastDebugPrint = millis();
    }
}
