#include <ODriveUART.h>
#include <SoftwareSerial.h>

// For Teensy 3/4, we use the hardware serial port (Serial3 in this example)
// (If you’re using another board, adjust accordingly)
HardwareSerial &odrive_serial = Serial3;
int baudrate = 115200; // Must match your ODrive’s UART baudrate

ODriveUART odrive(odrive_serial);

void setup() {
  // Start UART communication with ODrive
  odrive_serial.begin(baudrate);
  // For debugging via USB serial monitor
  Serial.begin(115200);
  delay(10);

  Serial.println("Waiting for ODrive...");
  // Wait until ODrive is discovered (its state is not UNDEFINED)
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.println("Found ODrive!");
  
  Serial.print("DC voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"));
  
  // Enable closed-loop control
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("Trying again to enable closed loop control...");
    delay(10);
  }
  Serial.println("ODrive running!");
}

void loop() {
  // Command a constant velocity
  // (Velocity is in units of motor turns per second)
  float velocity = 14.0f; // 1 turn per second (adjust as needed)
  Serial.print("Setting velocity to: ");
  Serial.println(velocity);
  // Set velocity (second parameter is optional torque feedforward, here 0.0)
  odrive.setVelocity(velocity, 0.0f);
  
  delay(10000);  // Run for 2 seconds
  
  // Reverse direction
  velocity = -14.0f;
  Serial.print("Setting velocity to: ");
  Serial.println(velocity);
  odrive.setVelocity(velocity, 0.0f);
  
  delay(10000);  // Run for 2 seconds
}
