#include <ODriveUART.h>
#include <SoftwareSerial.h>
#include <SBUS.h>
#include <VescUart.h>

// -----------------------
// SBUS Configuration (10 channels)
// -----------------------
SBUS sbus(Serial2);
uint16_t channels[10];  // Array to hold 10 SBUS channels
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// -----------------------
// ODrive Configuration
// -----------------------
HardwareSerial& odrive_serial = Serial3;
int baudrate = 115200;  // Must match ODrive config
ODriveUART odrive(odrive_serial);

// Global variable to hold the last computed target position.
float lastTargetPosition = 0.0f;

void setup() {
  // Initialize ODrive UART and Serial for debugging.
  odrive_serial.begin(baudrate);
  Serial.begin(115200);
  while (!Serial) { ; }
  delay(10);
  Serial.println("Established USB Serial.");

  // Initialize SBUS on Serial2.
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);

  // Display initial DC bus voltage.
  float vbus = odrive.getParameterAsFloat("vbus_voltage");
  Serial.print("DC voltage: ");
  Serial.println(vbus, 2);

  // Clear any pre-existing errors.
  odrive.clearErrors();
  Serial.println("Cleared errors.");

  // Run motor calibration.
  Serial.println("Starting motor calibration...");
  odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
  delay(4000);
  odrive.clearErrors();

  // Run encoder offset calibration.
  Serial.println("Starting encoder offset calibration...");
  odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
  delay(4000);
  odrive.clearErrors();

  // Transition to closed-loop control.
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("Trying again to enable closed-loop control...");
    delay(500);
  }
  
  Serial.println("ODrive is now in CLOSED_LOOP_CONTROL!");
  Serial.println("Setup complete.");
}

void loop() {
  // Check for new SBUS data and update target if available.
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    const float sbusMin = 390.0f;   // Adjust as needed for your transmitter
    const float sbusMax = 1811.0f;
    const float posRange = 500.0f;   // Maps to a target range of -10 to +10 rotations
    float rawValue = (float)channels[2];
    float normalized = (rawValue - sbusMin) / (sbusMax - sbusMin);
    lastTargetPosition = normalized * posRange - 10.0f;
  }

  // Continuously send the latest target position with a feedforward velocity of 30.0.
  odrive.setPosition(lastTargetPosition, 50.0f);

  // Print debug information every 100 ms (reducing print overhead).
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 100) {
    ODriveFeedback fb = odrive.getFeedback();
    String output;
    output += "Target: " + String(lastTargetPosition, 2);
    output += " | Feedback: " + String(fb.pos, 2);
    Serial.print("\r" + output);
    Serial.flush();
    lastPrintTime = millis();
  }

  delay(1); // Minimal delay to let loop run nearly continuously.
}
