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

   -- ODrive Section --
   ODrive:
     - Uses Serial3 for UART communication.
     - Baud rate: 115200.
     - Receives commands via setPosition() with a velocity feed-forward.
     
   SBUS (for ODrive):
     - Uses the same SBUS instance.
     - Uses channel 3 (channels[3]) for steering position mapping.
     - Midpoint is 1230 with a deadband of ±30:
         * Between 1200 and 1260, no steering change is sent.
         * Below 1200 maps linearly to left steering.
         * Above 1260 maps linearly to right steering.
         * The mapping is scaled to a maximum offset (here ±1.0).
     - The initial ODrive position is stored as the zero offset.
     - A lower feed-forward velocity (10.0) is used to ensure gentle movement.
     
   USB Serial:
     - Used for combined debug output.
     
   NOTE: Ensure all devices share a common ground.
*******************************************************/
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

// Global variables for steering calibration.
float steeringZeroOffset = 0.0f;  // Calibrated zero steering position
bool steeringCalibrated = false;

// Smoothing variable for VESC duty cycle command.
static float dutyFiltered = 0.0f;
const float smoothingAlpha = 0.1f;

// Maximum allowed steering offset.
const float maxSteeringOffset = 1.0f;

//////////////////////
// setup() Function
//////////////////////
void setup() {
  // ----- VESC & SBUS Setup -----
  Serial.begin(115200);
  while (!Serial) { ; }
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

  delay(500);
  
  Serial.println("Waiting for ODrive...");
  unsigned long startTime = millis();
  while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTime) < 15000) {
    delay(100);
  }
  if (odrive.getState() == AXIS_STATE_UNDEFINED) {
    Serial.println("ODrive not found! Proceeding without ODrive.");
  } else {
    Serial.println("Found ODrive! Yippeee!");
    
    Serial.print("DC voltage: ");
    Serial.println(odrive.getParameterAsFloat("vbus_voltage"), 2);
    
    Serial.println("Starting motor calibration...");
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    Serial.println("Starting encoder offset calibration...");
    odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    Serial.println("Enabling closed loop control...");
    startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime) < 5000) {
      odrive.clearErrors();
      odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
      Serial.println("Trying again to enable closed loop control");
      delay(10);
    }
    
    Serial.println("Setting input mode to TRAP_TRAJ...");
    odrive_serial.println("w axis0.controller.config.input_mode 1");
    delay(100);
    
    Serial.println("Setting velocity limit to 200.0...");
    odrive_serial.println("w axis0.controller.config.vel_limit 200.0");
    delay(100);
    
    Serial.println("Setting acceleration limit to 100.0...");
    odrive_serial.println("w axis0.controller.config.accel_limit 100.0");
    delay(100);
    
    Serial.println("Setting deceleration limit to 100.0...");
    odrive_serial.println("w axis0.controller.config.decel_limit 100.0");
    delay(100);
    
    Serial.println("ODrive running!");
    
    // ---- Steering Calibration ----
    // Record the current ODrive position as the zero offset.
    ODriveFeedback fb = odrive.getFeedback();
    steeringZeroOffset = fb.pos;
    steeringCalibrated = true;
    Serial.print("Steering zero offset set to: ");
    Serial.println(steeringZeroOffset, 2);
  }
  
  Serial.println("Setup complete.\n");
}

//////////////////////
// loop() Function
//////////////////////
void loop() {
  // Read SBUS data once per loop.
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);

  // ----- VESC Duty Cycle Mapping (using SBUS channel 1) -----
  float duty = 0.0;
  if (sbusDataValid) {
    int ch_vesc = channels[1];
    // Simple linear mapping with a deadband around 990 ±20.
    if (ch_vesc > 1010) {
      duty = (float)(ch_vesc - 1010) / 690.0; // Maps 1010->1700 to 0->1
    } else if (ch_vesc < 970) {
      duty = (float)(ch_vesc - 970) / 620.0;  // Maps 350->970 to -1->0
    } else {
      duty = 0.0;
    }
    // Apply smoothing to avoid sudden throttle bursts.
    dutyFiltered = (1 - smoothingAlpha) * dutyFiltered + smoothingAlpha * duty;
    if (fabs(dutyFiltered) < 0.07) {
      dutyFiltered = 0.0;
    }
    // Send the filtered duty cycle to both VESCs.
    vesc1.setDuty(dutyFiltered);
    vesc2.setDuty(dutyFiltered);
  }
  
  // ----- ODrive Steering Position Mapping (using SBUS channel 3) -----
  // For steering, the midpoint is 1230 with a ±30 deadband.
  if (sbusDataValid) {
    int ch_steer = channels[3];
    if (ch_steer >= 1200 && ch_steer <= 1260) {
      // Within deadband, maintain the calibrated zero.
      lastTargetPosition = steeringZeroOffset;
    } else if (ch_steer < 1200) {
      // Map [410, 1200] to a negative offset (left steering).
      float normalized = (float)(ch_steer - 1200) / (1200 - 410);  // Range: -1 to 0
      lastTargetPosition = steeringZeroOffset + normalized * maxSteeringOffset;
    } else { // ch_steer > 1260
      // Map [1260, 1811] to a positive offset (right steering).
      float normalized = (float)(ch_steer - 1260) / (1811 - 1260);  // Range: 0 to 1
      lastTargetPosition = steeringZeroOffset + normalized * maxSteeringOffset;
    }
  }
  // Use a reduced feed-forward velocity (10.0) for gentle initial movement.
  odrive.setPosition(lastTargetPosition, 10.0f);

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
