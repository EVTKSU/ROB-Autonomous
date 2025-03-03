#include <ODriveUART.h>
#include <SoftwareSerial.h>


 HardwareSerial& odrive_serial = Serial3;
 int baudrate = 115200; // Must match what you configure on the ODrive (see docs for details)


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
  float SINE_PERIOD = 2.0f; // Period of the position command sine wave in seconds

  float t = 0.001 * millis();
  
  float phase = t * (TWO_PI / SINE_PERIOD);
  
  odrive.setPosition(
    sin(phase), // position
    cos(phase) * (TWO_PI / SINE_PERIOD) // velocity feedforward (optional)
  );

  Serial.print("\n");
  Serial.print("goal pos: ");
  Serial.print(sin(phase));
  Serial.print(" goal vel: ");
  Serial.print(cos(phase) * (TWO_PI / SINE_PERIOD));
  Serial.print("\n");



  ODriveFeedback feedback = odrive.getFeedback();
  Serial.print("pos:");
  Serial.print(feedback.pos);
  Serial.print(", ");
  Serial.print("vel:");
  Serial.print(feedback.vel);
  Serial.println();
  delay(1000);
}