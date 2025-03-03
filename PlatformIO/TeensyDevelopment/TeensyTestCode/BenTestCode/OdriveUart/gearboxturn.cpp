#include <ODriveUART.h>
#include <SoftwareSerial.h>

HardwareSerial& odrive_serial = Serial3;
int baudrate = 115200; // Must match what you configure on the ODrive

ODriveUART odrive(odrive_serial);

void setup() {
  odrive_serial.begin(baudrate);
  Serial.begin(115200); // Serial to PC
  
  delay(10);
  Serial.println("Waiting for ODrive...");
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.println("found ODrive");
  
  Serial.print("DC voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"));
  
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
  unsigned long currentTime = millis();
  // Repeat every 2000 ms (2 seconds)
  unsigned long cycleTime = currentTime % 2000;

  float targetPos;
  float targetVel;
  
  // For the first 1000ms, command -7 rotations with -12 rotations per second.
  // For the next 1000ms, command +7 rotations with +12 rotations per second.
  if(cycleTime < 1000) {
    targetPos = -7.0f;
    targetVel = -12.0f;
  } else {
    targetPos = 7.0f;
    targetVel = 12.0f;
  }
  
  // Command the new target position and velocity feedforward.
  odrive.setPosition(targetPos, targetVel);
  
  Serial.print("\n");
  Serial.print("goal pos: ");
  Serial.print(targetPos);
  Serial.print(" goal vel: ");
  Serial.print(targetVel);
  Serial.print("\n");
  
  ODriveFeedback feedback = odrive.getFeedback();
  Serial.print("pos:");
  Serial.print(feedback.pos);
  Serial.print(", ");
  Serial.print("vel:");
  Serial.print(feedback.vel);
  Serial.println();
  
  delay(100);
}
