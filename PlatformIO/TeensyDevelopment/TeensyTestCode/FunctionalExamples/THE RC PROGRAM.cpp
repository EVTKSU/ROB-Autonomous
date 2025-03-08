/*******************************************************
   Combined Integrated VESC and ODrive with SBUS
   for Teensy 4.1

   -- VESC Section --
   VESC:
     - VESC1 uses Serial1 for UART communication.
     - VESC2 uses Serial5 for UART communication.
     - Baud rate: 115200.
     - Outputs: rpm, inpVoltage, ampHours, tachometerAbs.
     - Receives duty cycle commands via setDuty().

   SBUS (for VESC):
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000, SERIAL_8E2.
     - Uses channel 1 (channels[1]) for controlling duty cycle.
     - Input range: 350 to 1700.
     - Dead center is 990 with a deadband of ±20 counts.
     - Values above 1010 map to forward throttle; below 970 map to reverse.
     - Duty values below 0.07 (absolute) are filtered.
     - Smoothing is applied to reduce sudden throttle bursts.
     - An exponential mapping (quadratic) is used for a smoother ramp.

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
         2. Waits 3 seconds so the value can be seen.
         3. Performs motor calibration and encoder offset calibration.
         4. Reads the new steering reading.
         5. Forces the steering zero offset to remain at the pre-calibration value.
         6. Sets the target position to that value.
     - A lower feed-forward velocity (10.0) is used.
     
   USB Serial:
     - Used for combined debug output.
     
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

// ---------- VESC Setup (Dual VESC: VESC1 on Serial1, VESC2 on Serial5) ----------
VescUart vesc1;
VescUart vesc2;

// ---------- ODrive Setup (Serial3) ----------
// NOTE: In this code, ODrive is connected to Serial6.
HardwareSerial& odrive_serial = Serial6;
int baudrate = 115200;  // Must match ODrive config
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

// Smoothing variable for VESC duty cycle command.
static float dutyFiltered = 0.0f;
const float smoothingAlpha = 0.1f;

// Maximum allowed steering distance.
const float maxSteeringOffset = 1.5f;

// Global flag: set to true once the init calibration has been completed.
bool systemInitialized = false;

//////////////////////
// initCalibration() Function
//////////////////////
void initCalibration() {
  Serial.println("Init Calibration Triggered via SBUS Channel 5!");
  
  // Perform full ODrive calibration.
  // 1. Motor Calibration.
  Serial.println("Starting motor calibration...");
  odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // 2. Pre-calibration steering reading.
  ODriveFeedback fb = odrive.getFeedback();
  float preCalZero = fb.pos;
  Serial.print("Pre-calibration steering zero: ");
  Serial.println(preCalZero, 2);
  
  // Wait 3 seconds so we can view the pre-calibration value.
  delay(3000);
  
  // 3. Encoder Offset Calibration.
  Serial.println("Calibrating steering (encoder offset calibration)...");
  odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // 4. Post-calibration reading.
  fb = odrive.getFeedback();
  Serial.print("Post-calibration steering reading: ");
  Serial.println(fb.pos, 2);
  
  // Force the steering zero offset to remain at the pre-calibration value.
  steeringZeroOffset = preCalZero;
  Serial.print("Steering zero offset forced to pre-calibration value: ");
  Serial.println(steeringZeroOffset, 2);
  
  // Set target position to the forced zero.
  lastTargetPosition = steeringZeroOffset;
  
  // 5. Enable closed loop control.
  unsigned long startTime = millis();
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime) < 5000) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("Trying again to enable closed loop control");
    delay(10);
  }
  
  // 6. Set configuration parameters.
  Serial.println("Setting input mode to TRAP_TRAJ...");
  odrive_serial.println("w axis0.controller.config.input_mode 1");
  delay(100);
  Serial.println("Setting velocity limit to 200.0...");
  odrive_serial.println("w axis0.controller.config.vel_limit 30.0");
  delay(100);
  Serial.println("Setting acceleration limit to 100.0...");
  odrive_serial.println("w axis0.controller.config.accel_limit 25.0");
  delay(100);
  Serial.println("Setting deceleration limit to 100.0...");
  odrive_serial.println("w axis0.controller.config.decel_limit 50.0");
  delay(100);

  Serial.println("ODrive calibration complete and running!");
}

//////////////////////
// setup() Function
//////////////////////
void setup() {
  // Initialize status LED.
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // ----- USB Serial Setup with Timeout -----
  Serial.begin(115200);
  unsigned long serialStart = millis();
  // Wait up to 5 seconds for a USB Serial connection.
 
  Serial.println("Teensy 4.1 Integrated VESC, ODrive, and SBUS");
  
  // Initialize VESC1 on Serial1
  Serial1.begin(115200);
  vesc1.setSerialPort(&Serial1);
  
  // Initialize VESC2 on Serial5
  Serial5.begin(115200);
  vesc2.setSerialPort(&Serial5);
  
  // Initialize SBUS on Serial2 (100000 baud, 8E2)
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);
  
  // ----- ODrive Setup -----
  odrive_serial.begin(baudrate);
  Serial.println("Established ODrive communication");
  delay(500);!
  
  Serial.println("Waiting for ODrive...");
  unsigned long startTimeOD = millis();
  while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTimeOD < 15000)) {
    delay(100);
  }
  if (odrive.getState() == AXIS_STATE_UNDEFINED) {
    Serial.println("ODrive not found! Proceeding without ODrive.");
  } else {
    Serial.println("Found ODrive! Waiting for init calibration trigger via SBUS channel 5...");
  }
  
  Serial.println("Setup complete. System is idle until calibration is triggered.");
}

//////////////////////
// loop() Function
//////////////////////
void loop() {
  // Read SBUS data once per loop.
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);

  // Blink status LED if system is not yet initialized.
  static unsigned long lastLedToggle = 0;
  if (!systemInitialized) {
    if (millis() - lastLedToggle > 500) {
      lastLedToggle = millis();
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  } else {
    digitalWrite(STATUS_LED_PIN, HIGH);
  }
  
  // ----- Init Calibration Trigger -----
  // Wait until SBUS channel 5 (channels[5]) reads above 900.
  if (!systemInitialized) {
    if (sbusDataValid && channels[5] > 900) {
      initCalibration();
      systemInitialized = true;
    } else {
      // Print current SBUS channel 5 value for debugging.
      Serial.print("Waiting for init calibration trigger, SBUS channel 5: ");
      Serial.println(sbusDataValid ? channels[5] : 0);
      delay(100);
      return;  // Skip the rest of the loop until calibration is done.
    }
  }
  
  // ----- VESC Duty Cycle Mapping (using SBUS channel 1) -----
  float duty = 0.0;
  if (sbusDataValid) {
    int ch_vesc = channels[1];
    // Simple linear mapping with a deadband around 990 ±20.
    if (ch_vesc > 1010) {
      duty = (float)(ch_vesc - 1010) / 690.0; // Maps 1010 -> 1700 to 0 -> 1
    } else if (ch_vesc < 970) {
      duty = (float)(ch_vesc - 970) / 620.0;  // Maps 350 -> 970 to -1 -> 0
    } else {
      duty = 0.0;
    }
    // Apply exponential (quadratic) scaling for a smoother, slower ramp.
    float throttleExponent = 4.0;  // Adjust exponent as needed for more/less easing.
    float nonLinearDuty = (duty >= 0) ? pow(duty, throttleExponent) : -pow(-duty, throttleExponent);
    
    // Apply smoothing to avoid sudden throttle bursts.
    dutyFiltered = (1 - smoothingAlpha) * dutyFiltered + smoothingAlpha * nonLinearDuty;
    if (fabs(dutyFiltered) < 0.07) {
      dutyFiltered = 0.0;
    }
    // Send the filtered duty cycle to both VESCs.
    vesc1.setDuty(dutyFiltered);
    vesc2.setDuty(dutyFiltered);
  }
  
  // ----- ODrive Steering Position Mapping (using SBUS channel 3) -----
  // For steering, the midpoint is 1230 with a deadband of ±30.
  // When driven outside the deadband, a steering offset is computed.
  // When within the deadband, the offset decays toward zero.
  static float currentSteeringOffset = 0.0f;
  static unsigned long lastSteerUpdateTime = millis();
  const unsigned long steerHoldTime = 300;    // Hold time before decay (ms)
  const float steeringDecayRate = 0.005f;       // Decay rate per loop iteration
  
  if (sbusDataValid) {
    int ch_steer = channels[3];
    if (ch_steer < 1200 || ch_steer > 1260) {
      float desiredOffset = 0.0f;
      if (ch_steer < 1200) {
        // For left steering: lower values produce a negative offset.
        float normalized = (1200 - ch_steer) / float(1200 - 410);
        desiredOffset = - normalized * maxSteeringOffset;
      } else {  // ch_steer > 1260
        // For right steering: higher values produce a positive offset.
        float normalized = (ch_steer - 1260) / float(1811 - 1260);
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
  
  // Command the ODrive with the new steering target using a gentle feed-forward velocity.
  odrive.setPosition(lastTargetPosition, 20.0f);

  // Optional: Debug printing every 100 ms.
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 100) {
    ODriveFeedback fb = odrive.getFeedback();
    Serial.print("Steering Target: ");
    Serial.print(lastTargetPosition, 2);
    Serial.print(" | ODrive Pos: ");
    Serial.print(fb.pos, 2);
    Serial.print(" | VESC Duty: ");
    Serial.print(dutyFiltered, 3);
    Serial.print(" | VESC RPM: ");
    Serial.print((float)(vesc1.data.rpm) / 30.0, 1);
    Serial.println();
    lastPrintTime = millis();
  }
  
  delay(10);
}
