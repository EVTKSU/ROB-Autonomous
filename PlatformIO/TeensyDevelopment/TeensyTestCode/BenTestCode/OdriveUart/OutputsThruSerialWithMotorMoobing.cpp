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
int baudrate = 115200; // Must match what you configure on the ODrive
ODriveUART odrive(odrive_serial);

// Global variable to store the initial multi-turn position
float initPos = 0.0;

void setup() {
  // Initialize ODrive UART and USB Serial for debugging
  odrive_serial.begin(baudrate);
  Serial.begin(115200);
  while (!Serial) { ; }
  delay(10);
  Serial.println("Established USB Serial :)");
  
  // Initialize SBUS on Serial2 (100000 baud, 8E2 format)
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);
  
  // Wait for ODrive to be available
  Serial.println("Waiting for ODrive...");
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.println("Found ODrive! Yippeee!");
  
  Serial.print("DC voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"));
  
  // Enable closed loop control (repeating until successful)
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("trying again to enable closed loop control");
    delay(10);
  }
  
  // Capture the initial multi-turn position for relative commands.
  // This requires that your encoder is configured for multi-turn tracking.
  ODriveFeedback fb = odrive.getFeedback();
  initPos = fb.pos;
  Serial.print("Initial multi-turn position: ");
  Serial.println(initPos);
  
  Serial.println("ODrive running!");
}

void loop() {
  // Create a 2-second cycle
  unsigned long currentTime = millis();
  unsigned long cycleTime = currentTime % 2000;
  
  float targetPos;
  float targetVel;
  
  // For the first 1000 ms, command -7 rotations from the initial position with -12 rps feedforward.
  // For the next 1000 ms, command +7 rotations from the initial position with +12 rps feedforward.
  if (cycleTime < 1000) {
    targetPos = initPos - 28.0f;
    targetVel = -12.0f;
  } else {
    targetPos = initPos + 28.0f;
    targetVel = 12.0f;
  }
  
  // Command the new target position and velocity feedforward to the ODrive.
  odrive.setPosition(targetPos, targetVel);
  
  // Use the SBUS library's simple read() to update channel data.
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    // Build one output string containing SBUS channel data and ODrive parameters.
    String output = "";
    output += "SBUS -> ";
    for (int i = 0; i < 10; i++) {
      output += "CH" + String(i) + ": " + String(channels[i]) + "  ";
    }
    float voltage = odrive.getParameterAsFloat("vbus_voltage");
    float current = odrive.getParameterAsFloat("ibus");
    output += " | DC voltage: " + String(voltage, 2) + " V";
    output += " | ibus: " + String(current, 2) + " A";
    
    // Print output with carriage return to update the same line.
    Serial.print("\r" + output);
    Serial.flush();
  }
  
  delay(10);  // Small delay for processing
}
