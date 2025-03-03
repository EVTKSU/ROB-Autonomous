 
#include <ODriveUART.h>
#include <SoftwareSerial.h>

// Documentation for this example can be found here:
// https://docs.odriverobotics.com/v/latest/guides/arduino-uart-guide.html


////////////////////////////////
// Set up serial pins to the ODrive
////////////////////////////////

// Below are some sample configurations.
// You can comment out the default one and uncomment the one you wish to use.
// You can of course use something different if you like
// Don't forget to also connect ODrive ISOVDD and ISOGND to Arduino 3.3V/5V and GND.

// Teensy 3 and 4 (all versions) - Serial1
// pin 0: RX - connect to ODrive TX
// pin 1: TX - connect to ODrive RX
// See https://www.pjrc.com/teensy/td_uart.html for other options on Teensy
 HardwareSerial& odrive_serial = Serial3;
 int baudrate = 115200; // Must match what you configure on the ODrive (see docs for details)


ODriveUART odrive(odrive_serial);

void setup() {
  odrive_serial.begin(baudrate);

  Serial.begin(115200); // Serial to PC
  
  delay(10);

  // comment this out for debugging sometimes
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
  // float SINE_PERIOD = 1.0f; // Period of the position command sine wave in seconds

  // float t = 0.001 * millis();
  
  // float phase = t * (TWO_PI / SINE_PERIOD);
  
  // odrive.setPosition(
  //   sin(phase), // position
  //   cos(phase) * (TWO_PI / SINE_PERIOD) // velocity feedforward (optional)
  // );

  for (int i = 0; i < 100; i++) {

    odrive.setPosition(i, 5);
    
    ODriveFeedback feedback = odrive.getFeedback();
    Serial.print("pos:");
    Serial.print(feedback.pos);
    Serial.print(", ");
    Serial.print("vel:");
    Serial.print(feedback.vel);
    Serial.println();
    delay(1000);
  }
}