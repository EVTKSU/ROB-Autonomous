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

void setup() {
  // Initialize ODrive UART and Serial for debugging
  odrive_serial.begin(baudrate);
  Serial.begin(115200);
  while (!Serial) { ; }
  delay(10);
  Serial.println("Established USB Serial.");

  // Initialize SBUS on Serial2
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);

  // ------------------------------------------------------------
  // Check DC bus voltage
  // ------------------------------------------------------------
  float vbus = odrive.getParameterAsFloat("vbus_voltage");
  Serial.print("DC voltage: ");
  Serial.println(vbus, 2);

  // ------------------------------------------------------------
  // Clear pre-existing errors
  // ------------------------------------------------------------
  odrive.clearErrors();
  Serial.println("Cleared errors.");

  // ------------------------------------------------------------
  // Run motor calibration
  // ------------------------------------------------------------
  Serial.println("Starting motor calibration...");
  odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
  delay(4000); // Adjust delay as needed for your motor/encoder

  // Check errors again
  odrive.clearErrors();

  // ------------------------------------------------------------
  // Run encoder offset calibration
  // ------------------------------------------------------------
  Serial.println("Starting encoder offset calibration...");
  odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
  delay(4000);

  // Check errors again
  odrive.clearErrors();

  // ------------------------------------------------------------
  // Transition to closed-loop control
  // ------------------------------------------------------------
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("Trying again to enable closed-loop control...");
    delay(500);
  }
  
  Serial.println("ODrive is now in CLOSED_LOOP_CONTROL!");
  Serial.println("Setup complete.");
  Serial.println();
}

void loop() {
  // If new SBUS data is available, read it
  if (sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame)) {
    // Map channel 2 (channels[2]) to a target position in -10..+10 rotations
    const float sbusMin = 390.0f;   // Adjust if your SBUS channel is scaled differently
    const float sbusMax = 1811.0f;
    const float posRange = 20.0f;   // total span from -10 to +10
    float rawValue     = (float) channels[2];
    float normalized   = (rawValue - sbusMin) / (sbusMax - sbusMin);
    float targetPosition = normalized * posRange - 10.0f;

    // Send position command; feedforward velocity of 10.0
    odrive.setPosition(targetPosition, 10.0f);

    // Read back ODrive feedback
    ODriveFeedback fb = odrive.getFeedback(); // no axis param in your library

    // Get DC bus voltage, ibus, etc.
    float vbus = odrive.getParameterAsFloat("vbus_voltage");
    float ibus = odrive.getParameterAsFloat("ibus");

    // Print some info
    String output;
    output += "Raw CH2: "     + String(channels[2]);
    output += " | Target Pos: " + String(targetPosition, 2);
    output += " | Feedback Pos: " + String(fb.pos, 2);
    output += " | DC Voltage: " + String(vbus, 2) + " V";
    output += " | ibus: "       + String(ibus, 2) + " A";

    Serial.print("\r");
    Serial.print(output);
    Serial.flush();
  }

  delay(10);
}