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
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"));
  
  // Enable closed loop control (retry until successful)
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("trying again to enable closed loop control");
    delay(10);
  }
  
  Serial.println("ODrive running!");
}

void loop() {
  // Read SBUS frame; proceed if valid data is available.
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    // Use SBUS channel 2 (channels[2]) to determine the velocity.
    // Mapping:
    //   360   -> -20
    //   990   -> 0   (center)
    //   1800  -> +20
    float rawValue = (float)channels[2];
    float mappedVelocity = 0.0f;
    if (rawValue <= 990.0f) {
      // For values from 360 to 990:
      // Slope = (0 - (-20)) / (990 - 360) = 20 / 630.
      mappedVelocity = (rawValue - 990.0f) * (20.0f / 630.0f);
    } else {
      // For values from 990 to 1800:
      // Slope = (20 - 0) / (1800 - 990) = 20 / 810.
      mappedVelocity = (rawValue - 990.0f) * (20.0f / 810.0f);
    }
    
    // Command the ODrive in velocity mode.
    // The second parameter is a feedforward (set to 0.0f here).
    odrive.setVelocity(mappedVelocity, 0.0f);
    
    // Retrieve feedback to check current velocity.
    ODriveFeedback fb = odrive.getFeedback();
    
    // Build an output string with carriage return (updates same line).
    String output = "";
    output += "Raw CH2: " + String(channels[2]);
    output += " | Mapped Vel: " + String(mappedVelocity, 2);
    output += " | FB Vel: " + String(fb.vel, 2);
    output += " | DC Voltage: " + String(odrive.getParameterAsFloat("vbus_voltage"), 2) + " V";
    output += " | ibus: " + String(odrive.getParameterAsFloat("ibus"), 2) + " A";
    
    Serial.print("\r" + output);
    Serial.flush();
  }
  
  delay(10);  // Small delay for processing
}
