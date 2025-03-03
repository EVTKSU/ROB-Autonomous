#include <ODriveUART.h>
#include <SoftwareSerial.h>
#include <SBUS.h>
#include <VescUart.h>

// SBUS Configuration (10 channels)
SBUS sbus(Serial2);
uint16_t channels[10];  // initializing array of 10 channels
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// -----------------------
// ODrive Configuration
// -----------------------
HardwareSerial& odrive_serial = Serial3;
int baudrate = 115200;  // Must match ODrive config
ODriveUART odrive(odrive_serial);

// Global variable to store the latest target position.
float lastTargetPosition = 0.0f;

void setup() {
  // Initialize ODrive UART and USB Serial for debugging.
  odrive_serial.begin(baudrate);
  Serial.begin(115200);
  while (!Serial) { ; }
  delay(10);
  Serial.println("Established USB Serial :)");
  
  // Initialize SBUS on Serial2 (100000 baud, 8E2 format)
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);
  
  // Wait for ODrive to become available.
  Serial.println("Waiting for ODrive...");
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.println("Found ODrive! Yippeee!");
  
  Serial.print("DC voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"), 2);
  
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
  
  // Enable closed loop control (retry until successful)
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("trying again to enable closed loop control");
    delay(10);
  }
  
  // Set ODrive to use POS_FILTER mode.
  Serial.println("Setting input mode to POS_FILTER...");
  odrive_serial.println("w axis0.controller.config.input_mode 5");
  delay(100);
  
  // Increase pos_filter bandwidth to 100 Hz for faster response.
  Serial.println("Setting pos_filter bandwidth to 100 Hz...");
  odrive_serial.println("w axis0.controller.config.pos_filter_bandwidth 100");
  delay(100);
  
  Serial.println("ODrive running!");
  Serial.println("Setup complete.\n");
}

void loop() {
  // Update SBUS data if available.
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    const float sbusMin = 390.0f;   // Adjust these if necessary.
    const float sbusMax = 1811.0f;
    const float posRange = 20.0f;   // Maps to -10 to +10 rotations.
    float rawValue = (float) channels[2];
    float normalized = (rawValue - sbusMin) / (sbusMax - sbusMin);
    lastTargetPosition = normalized * posRange - 10.0f;
  }
  
  // Continuously send the latest target position.
  // Feedforward velocity is set high (200.0f) for rapid movement.
  odrive.setPosition(lastTargetPosition, 200.0f);
  
  // Debug printing every 100 ms to minimize overhead.
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
  
  // No extra delay to keep the loop as fast as possible.
}
