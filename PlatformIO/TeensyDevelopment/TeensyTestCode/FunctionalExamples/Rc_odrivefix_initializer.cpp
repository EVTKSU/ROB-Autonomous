/*******************************************************
   Combined Integrated VESC and ODrive with SBUS
   for Teensy 4.1

   -- VESC Section --
   VESC:
     - VESC1 uses Serial1 for UART communication.
     - VESC2 uses Serial5 for UART communication.
     - Baud rate: 115200.
     - Outputs: rpm, inpVoltage, ampHours, tachometerAbs.
     - Receives current commands via setCurrent().

   SBUS (for VESC):
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000, SERIAL_8E2.
     - Uses channel 1 (channels[1]) for throttle in this script.
     - Input range: 350 to 1700 (approx).
     - Neutral is 990 with a deadband of ±10 counts.
     - Throttle above neutral maps to forward current.
     - Throttle below neutral maps to negative current (reverse).
     - Throttle around neutral is within a small deadband that yields 0 A (coast).
     - No smoothing is applied; direct mapping to current.

   -- ODrive Section --
   ODrive:
     - Uses Serial3 for UART communication.
     - Baud rate: 115200.
     - Receives commands via setPosition() with a velocity feed-forward.
     
   SBUS (for ODrive):
     - Uses the same SBUS instance.
     - Uses channel 3 (channels[3]) for steering control.
     - Midpoint is 1230 with a deadband of ±30.
         * When driven outside the deadband, a steering offset is computed.
         * When within the deadband, the offset decays toward zero.
     - The init calibration routine (triggered via SBUS channel 5 > 900) does:
         1. Reads and outputs the pre-calibration steering position.
         2. Waits 3 seconds so we can see it in the console.
         3. Performs motor calibration and encoder offset calibration.
         4. Reads the new steering reading.
         5. Forces the steering zero offset to remain at the pre-calibration value.
         6. Sets the target position to that value.
     - A lower feed-forward velocity (10.0) is used.
     
   USB Serial:
     - Used for combined debug output. Debug messages are sent only if USB is connected.
     
   Status LED:
     - Uses built-in LED on pin 13.
     - Blinks while waiting for calibration trigger; solid after calibration.
     
   NOTE: Ensure all devices share a common ground.
*******************************************************/

#define STATUS_LED_PIN 13

#include <Arduino.h>
#include <VescUart.h>
#include <ODriveUART.h>
#include <SBUS.h>
#include <SoftwareSerial.h>

// Configure how many amps correspond to full-throttle
static const float MAX_CURRENT = 10.0f;

// Global constant for steering offset
const float maxSteeringOffset = 1.5f;

// ---------- VESC Setup (Dual VESC: VESC1 on Serial1, VESC2 on Serial5) ----------
VescUart vesc1;
VescUart vesc2;

// ---------- ODrive Setup (Serial3) ----------
HardwareSerial& odrive_serial = Serial6;  // Teensy 4.1 hardware serial
int baudrate = 115200;                   // Must match ODrive config
ODriveUART odrive(odrive_serial);

// ---------- SBUS Setup (Serial2) ----------
SBUS sbus(Serial2);
uint16_t channels[10];  // Array for 10 SBUS channels
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// Global variable for ODrive steering target position.
float lastTargetPosition = 0.0f;
// Global variable for forced steering zero (post-calibration).
float steeringZeroOffset = 0.0f;

// Global flag: set to true once the init calibration has been completed.
bool systemInitialized = false;

//////////////////////
// initCalibration() Function
//////////////////////
void initCalibration() {
  if (Serial) {
    Serial.println("Init Calibration Triggered via SBUS Channel 5!");
  }
  
  // 1. Motor Calibration on ODrive.
  if (Serial) {
    Serial.println("Starting motor calibration...");
  }
  odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // 2. Pre-calibration steering reading.
  ODriveFeedback fb = odrive.getFeedback();
  float preCalZero = fb.pos;
  if (Serial) {
    Serial.print("Pre-calibration steering zero: ");
    Serial.println(preCalZero, 2);
  }
  
  delay(3000);
  
  // 3. Encoder Offset Calibration.
  if (Serial) {
    Serial.println("Calibrating steering (encoder offset calibration)...");
  }
  odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // 4. Post-calibration reading.
  fb = odrive.getFeedback();
  if (Serial) {
    Serial.print("Post-calibration steering reading: ");
    Serial.println(fb.pos, 2);
  }
  
  // Force the steering zero offset to remain at the pre-calibration value.
  steeringZeroOffset = preCalZero;
  if (Serial) {
    Serial.print("Steering zero offset forced to pre-calibration value: ");
    Serial.println(steeringZeroOffset, 2);
  }
  
  lastTargetPosition = steeringZeroOffset;
  
  // 5. Enable closed loop control.
  unsigned long startTime = millis();
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime < 5000)) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    if (Serial) {
      Serial.println("Trying again to enable closed loop control");
    }
    delay(10);
  }
  
  // 6. (Optional) Additional ODrive config commands
  if (Serial) {
    Serial.println("Setting input mode to TRAP_TRAJ...");
  }
  odrive_serial.println("w axis0.controller.config.input_mode 1");
  delay(100);
  odrive_serial.println("w axis0.controller.config.vel_limit 30.0");
  delay(100);
  odrive_serial.println("w axis0.controller.config.accel_limit 25.0");
  delay(100);
  odrive_serial.println("w axis0.controller.config.decel_limit 50.0");
  delay(100);
  odrive_serial.println("w axis0.motor.config.current_lim 80.0");
  delay(100);
  odrive_serial.println("w axis0.controller.config.current_lim 90.0");
  delay(100);
  
  if (Serial) {
    Serial.println("ODrive calibration complete and running!");
  }
}

//////////////////////
// setup() Function
//////////////////////
void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  Serial.begin(115200);
  if (Serial) {
    Serial.println("Teensy 4.1 Integrated VESC (Current Control), ODrive, and SBUS");
  }
  
  // Initialize VESC1 on Serial1
  Serial1.begin(115200);
  vesc1.setSerialPort(&Serial1);
  
  // Initialize VESC2 on Serial5
  Serial5.begin(115200);
  vesc2.setSerialPort(&Serial5);
  
  // Initialize SBUS on Serial2
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);
  
  // ODrive Setup
  odrive_serial.begin(baudrate);
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
      Serial.println("Found ODrive! Waiting for init calibration trigger via SBUS channel 5...");
    }
  }
  
  if (Serial) {
    Serial.println("Setup complete. System is idle until calibration is triggered.");
  }
}

//////////////////////
// loop() Function
//////////////////////
void loop() {
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);

  // ODrive Error Clearing and Re-Calibration (using SBUS channel 4)
  static bool errorClearFlag = false;
  if (sbusDataValid) {
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
  }

  // LED Status Indicator
  static unsigned long lastLedToggle = 0;
  if (!systemInitialized) {
    if (millis() - lastLedToggle > 500) {
      lastLedToggle = millis();
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  } else {
    digitalWrite(STATUS_LED_PIN, HIGH);
  }
  
  // Init Calibration Trigger (using SBUS channel 5)
  if (!systemInitialized) {
    if (sbusDataValid && channels[5] > 900) {
      initCalibration();
      systemInitialized = true;
    } else {
      if (Serial) {
        Serial.print("Waiting for init calibration trigger, SBUS channel 5: ");
        Serial.println(sbusDataValid ? channels[5] : 0);
      }
      delay(100);
      return;
    }
  }
  
  // VESC Current Control (using SBUS channel 1) WITH REVERSE
  // SBUS range: ~350..1700, neutral ~990, deadband ±20.
  // If above (neutral+deadband), forward current up to MAX_CURRENT.
  // If below (neutral-deadband), reverse current down to -MAX_CURRENT.
  // Otherwise, coast at 0 A.
  float currentCommand = 0.0f;
  if (sbusDataValid) {
    int ch_vesc = channels[1];
    const int neutral = 990;
    const int deadband = 20;
    
    if (ch_vesc > (neutral + deadband)) {
      // Forward
      float forwardRange = (1700.0f - (neutral + deadband));
      currentCommand = (float)(ch_vesc - (neutral + deadband)) / forwardRange;
      if (currentCommand < 0.0f) currentCommand = 0.0f;
      if (currentCommand > 1.0f) currentCommand = 1.0f;
      currentCommand *= MAX_CURRENT;
    } 
    else if (ch_vesc < (neutral - deadband)) {
      // Reverse
      float reverseRange = (float)((neutral - deadband) - 350);
      float proportion = (float)((neutral - deadband) - ch_vesc) / reverseRange;
      if (proportion < 0.0f) proportion = 0.0f;
      if (proportion > 1.0f) proportion = 1.0f;
      currentCommand = -proportion * MAX_CURRENT;
    } 
    else {
      // Deadband => coast
      currentCommand = 0.0f;
    }
    
    // Issue current command to both VESCs
    vesc1.setCurrent(currentCommand);
    vesc2.setCurrent(currentCommand);
  }
  
  // ODrive Steering Position Mapping (using SBUS channel 3)
  static float currentSteeringOffset = 0.0f;
  static unsigned long lastSteerUpdateTime = millis();
  const unsigned long steerHoldTime = 300;
  const float steeringDecayRate = 0.005f;
  
  if (sbusDataValid) {
    int ch_steer = channels[3];
    if (ch_steer < 1200 || ch_steer > 1260) {
      float desiredOffset = 0.0f;
      if (ch_steer < 1200) {
        float normalized = (1200.0f - ch_steer) / float(1200 - 410);
        desiredOffset = -normalized * maxSteeringOffset;
      } else {
        float normalized = (ch_steer - 1260.0f) / float(1811 - 1260);
        desiredOffset = normalized * maxSteeringOffset;
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
  }
  
  // Command the ODrive with the new steering target.
  odrive.setPosition(lastTargetPosition, 20.0f);
  
  // Debug Output every 100 ms.
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 100) {
    ODriveFeedback fb = odrive.getFeedback();
    if (Serial) {
      Serial.print("Steering Target: ");
      Serial.print(lastTargetPosition, 2);
      Serial.print(" | ODrive Pos: ");
      Serial.print(fb.pos, 2);
      Serial.print(" | VESC Current: ");
      Serial.print(currentCommand, 2);
      Serial.print(" A  CH1: ");
      Serial.print(channels[1]);
      Serial.print(" | VESC RPM: ");
      Serial.print((float)(vesc1.data.rpm) / 30.0, 1);
      Serial.println();
    }
    lastPrintTime = millis();
  }
  
  delay(10);
}
