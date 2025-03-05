#include <Arduino.h>
#include <VescUart.h>
#include <ODriveUART.h>
#include <SBUS.h>
#include <SoftwareSerial.h>

// -----------------------
// VESC Configuration (Serial1)
// -----------------------
VescUart UART;  // VESC communication

// -----------------------
// ODrive Configuration (Serial3)
// -----------------------
HardwareSerial& odrive_serial = Serial3;
int odriveBaud = 115200;  // Must match ODrive configuration
ODriveUART odrive(odrive_serial);
float lastTargetPosition = 0.0f;  // Global target position for ODrive

// -----------------------
// SBUS Configuration (Serial2)
// -----------------------
SBUS sbus(Serial2);
uint16_t channels[10];  // Array for SBUS channel values (assumes 10 channels)
bool sbusFailSafe = false;
bool sbusLostFrame = false;

void setup() {
  // Initialize USB Serial for debugging output.
  Serial.begin(115200);
  while (!Serial) { ; }
  
  // Print happy startup messages with carriage return.
  Serial.print("\r");
  Serial.println("Teensy 4.1 Integrated VESC, ODrive, and SBUS System Starting...");
  
  // Initialize VESC on Serial1.
  Serial.print("\r");
  Serial.println("Initializing VESC on Serial1...");
  Serial1.begin(115200);
  UART.setSerialPort(&Serial1);
  
  // Initialize SBUS on Serial2 (100000 baud, SERIAL_8E2).
  Serial.print("\r");
  Serial.println("Initializing SBUS on Serial2...");
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  
  // Initialize ODrive on Serial3.
  Serial.print("\r");
  Serial.println("Initializing ODrive on Serial3...");
  odrive_serial.begin(odriveBaud);
  
  delay(500);
  
  // --- ODrive Setup ---
  Serial.print("\r");
  Serial.println("Waiting for ODrive...");
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.print("\r");
  Serial.println("ODrive Found!");
  
  Serial.print("\r");
  Serial.print("ODrive DC Voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"), 2);
  
  // Run motor calibration.
  Serial.print("\r");
  Serial.println("Starting ODrive Motor Calibration...");
  odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // Run encoder offset calibration.
  Serial.print("\r");
  Serial.println("Starting ODrive Encoder Offset Calibration...");
  odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // Enable closed loop control.
  Serial.print("\r");
  Serial.println("Enabling ODrive Closed Loop Control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.print("\r");
    Serial.println("Retrying to Enable Closed Loop Control...");
    delay(10);
  }
  
  // Configure ODrive input mode and limits.
  Serial.print("\r");
  Serial.println("Setting ODrive Input Mode to TRAP_TRAJ...");
  odrive_serial.println("w axis0.controller.config.input_mode 1");
  delay(100);
  
  Serial.print("\r");
  Serial.println("Configuring ODrive Velocity Limit to 40.0...");
  odrive_serial.println("w axis0.controller.config.vel_limit 40.0");
  delay(100);
  
  Serial.print("\r");
  Serial.println("Configuring ODrive Acceleration Limit to 20.0...");
  odrive_serial.println("w axis0.controller.config.accel_limit 20.0");
  delay(100);
  
  Serial.print("\r");
  Serial.println("Configuring ODrive Deceleration Limit to 200.0...");
  odrive_serial.println("w axis0.controller.config.decel_limit 200.0");
  delay(100);
  
  Serial.print("\r");
  Serial.println("ODrive Setup Complete.");
  Serial.print("\r");
  Serial.println("System Setup Complete.\n");
}

void loop() {
  // --- SBUS Reading ---
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);
  
  // --- VESC Duty Cycle Mapping ---
  // Map SBUS channel 2 (range 350 to 1700) to duty cycle (0.0 to 1.0).
  float vescDuty = 0.0;
  if (sbusDataValid) {
    vescDuty = (float)(channels[2] - 350) / 1350.0;  // 1700 - 350 = 1350.
    vescDuty = constrain(vescDuty, 0.0, 1.0);
    if (vescDuty < 0.01) {
      vescDuty = 0.0;
    } else {
      // Send duty cycle command to VESC.
      UART.setDuty(vescDuty);
    }
  }
  
  // --- ODrive Position Mapping ---
  // Map SBUS channel 2 (range 410 to 1811) to target position (-10 to +10 rotations).
  const float sbusMin_OD = 410.0;
  const float sbusMax_OD = 1811.0;
  const float posRange = 20.0;  // Total range: 20 rotations.
  if (sbusDataValid) {
    float rawValue = (float) channels[2];
    float normalized = (rawValue - sbusMin_OD) / (sbusMax_OD - sbusMin_OD);
    lastTargetPosition = normalized * posRange - (posRange / 2.0);  // Maps to -10 to +10.
  }
  
  // --- Command Outputs ---
  // VESC: duty cycle command already sent if valid.
  // ODrive: command the target position with a velocity feed-forward value.
  odrive.setPosition(lastTargetPosition, 40.0f);
  
  // --- Telemetry and Debug Output ---
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 100) {
    // VESC Telemetry.
    bool vescDataValid = UART.getVescValues();
    String vescOutput = "";
    if (vescDataValid) {
      float realRpm = (float)(UART.data.rpm) / 30.0;
      vescOutput = "VESC -> RPM: " + String(realRpm, 2) +
                    ", Voltage: " + String(UART.data.inpVoltage, 2) +
                    ", AmpHours: " + String(UART.data.ampHours, 2) +
                    ", TachAbs: " + String(UART.data.tachometerAbs, 2);
    } else {
      vescOutput = "VESC -> No Data";
    }
    
    // ODrive Telemetry.
    ODriveFeedback fb = odrive.getFeedback();
    float odCurrent = odrive.getParameterAsFloat("ibus");
    float odVoltage = odrive.getParameterAsFloat("vbus_voltage");
    float odRpm = fb.vel * 60.0;  // Assuming fb.vel is in rev/s.
    
    // Print telemetry as CSV with a preceding carriage return.
    Serial.print("\r");
    // CSV columns: VESC_RPM, VESC_Voltage, VESC_AmpHours, VESC_Tach, SBUS_CH2, VESC_Duty,
    // ODrive_TargetPos, ODrive_FeedbackPos, ODrive_Current, ODrive_Voltage, ODrive_RPM.
    Serial.print((vescDataValid ? String((float)(UART.data.rpm), 2) : "NaN"));
    Serial.print(",");
    Serial.print((vescDataValid ? String(UART.data.inpVoltage, 2) : "NaN"));
    Serial.print(",");
    Serial.print((vescDataValid ? String(UART.data.ampHours, 2) : "NaN"));
    Serial.print(",");
    Serial.print((vescDataValid ? String(UART.data.tachometerAbs, 2) : "NaN"));
    Serial.print(",");
    Serial.print(sbusDataValid ? String(channels[2]) : "NaN");
    Serial.print(",");
    Serial.print(String(vescDuty, 3));
    Serial.print(",");
    Serial.print(String(lastTargetPosition, 2));
    Serial.print(",");
    Serial.print(String(fb.pos, 2));
    Serial.print(",");
    if (isnan(odCurrent)) {
      Serial.print("NaN");
    } else {
      Serial.print(String(odCurrent, 2));
    }
    Serial.print(",");
    Serial.print(String(odVoltage, 2));
    Serial.print(",");
    Serial.println(String(odRpm, 2));
    
    // Also print the happy VESC debug message.
    Serial.print("\r");
    Serial.println(vescOutput);
    
    lastPrintTime = millis();
  }
  
  delay(10);
}
