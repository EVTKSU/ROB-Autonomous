/*******************************************************
   Integrated VESC and SBUS with Duty Cycle Mapping and Filtering for Teensy 4.1

   VESC:
     - Uses Serial1 for UART communication with the VESC.
     - Baud rate: 115200 (ensure this matches your VESC settings).
     - Outputs: rpm, inpVoltage, ampHours, tachometerAbs.
     - Receives duty cycle commands via setDuty().

   SBUS:
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000 with configuration SERIAL_8E2.
     - Uses channel 2 (channels[2] in this code) for controlling duty cycle.
     - SBUS channel 2 input range: 350 to 1700.
     - Mapped to duty cycle range: 0.0 to 1.0.
     - Duty values below 0.01 are filtered (set to 0 and no command is sent).

   USB Serial:
     - Used for combined debug output.
     
   NOTE: Ensure all devices share a common ground.
*******************************************************/
#include <Arduino.h>
#include <VescUart.h>

// Instantiate VescUart for VESC communication on Serial1
VescUart UART;

void setup() {
  // Initialize USB Serial for debugging output
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for USB Serial port to connect
  }
  Serial.println("Teensy 4.1 Integrated VESC and SBUS with Duty Cycle Mapping and Filtering");

  // Initialize Serial1 for VESC communication
  Serial1.begin(115200);
  UART.setSerialPort(&Serial1); // Set the serial port for VESC communication

  // Allow time for the interface to initialize
  delay(500);
}

void loop() {
    // New duty cycle loop: cycle duty from 0 to 1 over 5 seconds (5000 ms)
    static unsigned long startTime = millis(); // Start time for the cycle
    const unsigned long cycleDuration = 5000; // 5 seconds
    unsigned long elapsed = millis() - startTime; // Elapsed time since start
    float duty = (elapsed % cycleDuration) / (float)cycleDuration;
    
    // Send the computed duty value to the VESC.
    UART.setDuty(duty);
    
    // Debug output.
    Serial.print("Cycled Duty: ");
    Serial.println(duty, 3);
    
    delay(250);
}