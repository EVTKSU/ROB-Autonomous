#include <ODriveUART.h>
#include <SoftwareSerial.h>
#include <SBUS.h>

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
  // Initialize ODrive UART and USB Serial for debugging/plotting.
  odrive_serial.begin(baudrate);
  Serial.begin(115200);
  while (!Serial) { ; }
  delay(10);
  Serial.print("\r");
  Serial.println("Established USB Serial :)");
  
  // Initialize SBUS on Serial2 (100000 baud, 8E2 format)
  Serial.print("\r");
  Serial.println("Initializing SBUS on Serial2...");
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);
  
  // Wait for ODrive to become available.
  Serial.print("\r");
  Serial.println("Waiting for ODrive...");
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.print("\r");
  Serial.println("Found ODrive! Yippeee!");
  
  Serial.print("\r");
  Serial.print("DC voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"), 2);
  
  // Run motor calibration.
  Serial.print("\r");
  Serial.println("Starting motor calibration...");
  odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  // Run encoder offset calibration.
  Serial.print("\r");
  Serial.println("Starting encoder offset calibration...");
  odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
  delay(4000);
  odrive.clearErrors();
  
  Serial.print("\r");
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.print("\r");
    Serial.println("trying again to enable closed loop control");
    delay(10);
  }
  
  // Set input mode to TRAP_TRAJ for aggressive trapezoidal trajectory motion.
  Serial.print("\r");
  Serial.println("Setting input mode to TRAP_TRAJ...");
  odrive_serial.println("w axis0.controller.config.input_mode 1");
  delay(100);
  
  // Set velocity limit to allow faster movement.
  Serial.print("\r");
  Serial.println("Setting velocity limit to 200.0...");
  odrive_serial.println("w axis0.controller.config.vel_limit 200.0");
  delay(100);
  
  // Increase acceleration limit to 200.0 for faster acceleration.
  Serial.print("\r");
  Serial.println("Setting acceleration limit to 200.0...");
  odrive_serial.println("w axis0.controller.config.accel_limit 200.0");
  delay(100);
  
  // Set deceleration limit to 200.0 for symmetric braking.
  Serial.print("\r");
  Serial.println("Setting deceleration limit to 200.0...");
  odrive_serial.println("w axis0.controller.config.decel_limit 200.0");
  delay(100);
  
  Serial.print("\r");
  Serial.println("ODrive running!");
  Serial.print("\r");
  Serial.println("Setup complete.\n");
}

void loop() {
  // Update SBUS data if available.
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    const float sbusMin = 410.0f;   // Adjust these if necessary.
    const float sbusMax = 1811.0f;
    const float posRange = 4.0f;   // Maps sbus values to 20 rotations each direction.
    float rawValue = (float) channels[3];
    float normalized = (rawValue - sbusMin) / (sbusMax - sbusMin);
    lastTargetPosition = normalized * posRange - 2.0f;
  }
  
  // Command the new target position with a velocity feed-forward value.
  odrive.setPosition(lastTargetPosition, 40.0f);
  
  // Debug printing for plotting every 100 ms.
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 100) {
    // Get feedback from ODrive.
    ODriveFeedback fb = odrive.getFeedback();
    
    // Retrieve current bus amperage and voltage.
    float current = odrive.getParameterAsFloat("ibus");
    float voltage = odrive.getParameterAsFloat("vbus_voltage");
    
    // Calculate RPM (assuming fb.vel is in rev/s).
    float rpm = fb.vel * 60.0;
    
    // Start each line with a carriage return.
    Serial.print("\r");
    // Print CSV: target position, feedback position.
    Serial.print(lastTargetPosition, 2);
    Serial.print(",");
    Serial.print(fb.pos, 2);
    Serial.print(",");
    
    // Check current reading; if it's NaN, print "NaN", otherwise print value.
    if (isnan(current)) {
      Serial.print("NaN");
    } else {
      Serial.print(current, 2);
    }
    
    Serial.print(",");
    Serial.print(voltage, 2);
    Serial.print(",");
    Serial.println(rpm, 2);
    
    lastPrintTime = millis();
  }
}
