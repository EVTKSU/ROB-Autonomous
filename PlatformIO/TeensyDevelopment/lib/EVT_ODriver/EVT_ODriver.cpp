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
float manualTrim = 0.0f;  // CHANGE ME TO TRIM ZERO; use negative for right trim, positive for left trim.
static bool errorClearFlag = false;
static unsigned long lastSteerUpdateTime = 0;
static float currentSteeringPos = 0.0f;
static unsigned long lastPrintTime = 0;
static float maxLeft = 0.0f;
static float maxRight = 0.0f;
float OdrvCurrent = odrive.getParameterAsFloat("ibus");
bool actionDone = false;

void CenterTheOdrive() {
    Serial.println("Init Calibration Triggered via SBUS Channel 5!");
    ODriveFeedback fb = odrive.getFeedback();
    float startZero = fb.pos;
    Serial.println("Starting da slow calibration...");
    delay(700);
    const float oopsCurrent = 10.0f;    // changable current threshold.
    const float stepIncrement = 0.25f;  // Increment value for each setPosition command.
    const unsigned long delayBetweenSteps = 200; // Delay (in ms) between commands.
    const unsigned long timeout = 8000; // Timeout period in ms.
    int stepValue = 0.0f;
    odrive.clearErrors();
    Serial.print("startup steering zero: ");
    Serial.println(String(startZero, 2));
    delay(500);

    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        stepValue += stepIncrement;  // Increase position by 0.25f.
        odrive.setPosition(stepValue, 27.0f);
        delay(delayBetweenSteps);  // Wait for the command to take effect.

        float currentVal = odrive.getParameterAsFloat("ibus");
        
        Serial.print("Right calibration: pos = ");
        Serial.print(stepValue, 3);
        Serial.print(" | Current = ");
        Serial.println(currentVal, 2);
        
        if (currentVal > oopsCurrent) {
            maxRight = stepValue - 0.075f;
            Serial.print("Max right detected at (with offset): ");
            Serial.println(maxRight, 3);
            break;
        }
    }
    if (millis() - startTime >= timeout) {
        Serial.println("Error: Right calibration timeout.");
        return;
    }
    
    // ----- Calibrate Maximum Left Travel -----
    Serial.println("Calibrating maximum left travel...");
    startTime = millis();  // Reset the timer.
        
    while (millis() - startTime < timeout) {
        stepValue -= stepIncrement;  // Decrease position by 0.25f.
        odrive.setPosition(stepValue, 27.0f);
        delay(delayBetweenSteps);
        
        float currentVal = odrive.getParameterAsFloat("ibus");
       
        Serial.print("Left calibration: pos = ");
        Serial.print(stepValue, 3);
        Serial.print(" | Current = ");
        Serial.println(currentVal, 2);
        
        if (currentVal > oopsCurrent) {
            // Extend further left by subtracting a 0.05f safety offset.
            maxLeft = stepValue - 0.05f;
            Serial.print("Max left detected at (with offset): ");
            Serial.println(maxLeft, 3);
            break;
        }
    }
    if (millis() - startTime >= timeout) {
        Serial.println("Error: Left calibration timeout.");
        return;
    }
    
    // ----- Finalize Calibration -----
    Serial.println("Calibration complete.");
    Serial.print("Zero: ");
    Serial.println(startZero, 3);
    Serial.print("Max Left: ");
    Serial.println(maxLeft, 3);
    Serial.print("Max Right: ");
    Serial.println(maxRight, 3);
    
    // then apply the manual trim offset so that the new zero becomes:
    // (midpoint + manualTrim)
    float mechanicalCenter = (maxLeft + maxRight) / 2.0f;
    steeringZeroOffset = mechanicalCenter + manualTrim;
    Serial.print("Mechanical Center: ");
    Serial.println(mechanicalCenter, 3);
    Serial.print("Steering Zero Offset (with manual trim): ");
    Serial.println(steeringZeroOffset, 3);
    
    // Set the ODrive into closed loop control mode.
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    delay(1000);
    
    odrive.clearErrors(); // just for safety? 
    
    fb = odrive.getFeedback();
    Serial.print("Post-calibration steering reading: ");
    Serial.println(String(fb.pos, 2));
    lastTargetPosition = steeringZeroOffset;
    delay(1000);

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
            CenterTheOdrive();
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
            CenterTheOdrive();
            systemInitialized = true;
        } else {
            Serial.print("Waiting for calibration trigger, SBUS channel 5: ");
            Serial.println(String(channels[5]));
            return;
        }
    }
    
    const unsigned long steerHoldTime = 300; // unsure what does i forgot...
    const float steeringDecayRate = 0.005f; // how fast the steering offset decays when not being updated.
    int ch_steer = channels[3]; // SBUS channel 3 is steering
    if (ch_steer < 1200 || ch_steer > 1260) {   
        float steerPos = 0.0f;     // this is the desired steering "angle" based on the sbus channel value
        if (ch_steer < 1200) {                                                  // if the steering is less than 1200, we are turning left
            float AdjustedSteerVal = (1200.0f - ch_steer) / float(1200 - 410);   // 
            steerPos = -AdjustedSteerVal * 2.2f; // 2.2f is the max steering movement, either direction, in radians? i think 
        } else {                                                                // if the steering is greater than 1260, we are turning right
            float AdjustedSteerVal = (ch_steer - 1260.0f) / float(1811 - 1260);
            steerPos = AdjustedSteerVal * 2.2f;
        }
        currentSteeringPos= steerPos; // numerically sets the steering position to the commanded position
        lastSteerUpdateTime = millis(); 
    } else {
        if (millis() - lastSteerUpdateTime > steerHoldTime) {
            if (currentSteeringPos > 0.0f) {  
                currentSteeringPos -= steeringDecayRate;
                if (currentSteeringPos < 0.0f)
                currentSteeringPos = 0.0f; 
            } else if (currentSteeringPos < 0.0f) {
                currentSteeringPos += steeringDecayRate;
                if (currentSteeringPos > 0.0f)
                currentSteeringPos = 0.0f;
            }
        }
    }
      
    

    lastTargetPosition = steeringZeroOffset + currentSteeringPos;
    
// Clamp the target position using Arduino's constrain() function.
lastTargetPosition = constrain(lastTargetPosition, maxLeft, maxRight);

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
