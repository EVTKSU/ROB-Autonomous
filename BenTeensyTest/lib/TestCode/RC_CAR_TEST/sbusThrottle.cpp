/*******************************************************
   Integrated VESC and SBUS with Duty Cycle Mapping for Teensy 4.1

   VESC:
     - Uses Serial1 for UART communication with the VESC.
     - Baud rate: 115200 (ensure this matches your VESC settings).
     - Outputs: rpm, inpVoltage, ampHours, tachometerAbs.
     - Receives duty cycle commands via setDuty().

   SBUS:
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000 with configuration SERIAL_8E2.
     - Uses channel 2 (channels[1]) for controlling duty cycle.
     - SBUS channel 2 input range: 350 to 1700.
     - Mapped to duty cycle range: 0.0 to 1.0.
     
   USB Serial:
     - Used for combined debug output.
     
   NOTE: Ensure all devices share a common ground.
*******************************************************/
#include <Arduino.h>
#include <VescUart.h>
#include <SBUS.h>

// Instantiate VescUart for VESC communication on Serial1
VescUart UART;

// Instantiate SBUS for RC receiver data on Serial2
SBUS sbus(Serial2);
uint16_t channels[10];  // Array to hold SBUS channel values (assumes 10 channels)
bool sbusFailSafe = false;
bool sbusLostFrame = false;

void setup() {
  // Initialize USB Serial for debugging output
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for USB Serial port to connect
  }
  Serial.println("Teensy 4.1 Integrated VESC and SBUS with Duty Cycle Mapping");

  // Initialize Serial1 for VESC communication
  Serial1.begin(115200);
  UART.setSerialPort(&Serial1);

  // Initialize Serial2 for SBUS input with typical SBUS parameters:
  // 100000 baud, 8 data bits, even parity, 2 stop bits.
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();

  // Allow time for both interfaces to initialize
  delay(500);
}

void loop() {
  // Retrieve VESC telemetry from Serial1
  bool vescDataValid = UART.getVescValues();

  // Retrieve SBUS data from Serial2
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);

  // Map SBUS channel 2 (channels[1]) input (350 to 1700) to duty cycle (0.0 to 1.0)
  float duty = 0.0;
  if (sbusDataValid) {
    duty = (float)(channels[2] - 350) / (1700 - 350);
    duty = constrain(duty, 0.0, 1.0);
    // Send the mapped duty cycle to the VESC
    UART.setDuty(duty);
  }

  // Build a combined output string for USB Serial debug
  String output = "";

  if (vescDataValid) {
    output += "VESC -> RPM: " + String(UART.data.rpm) +
              ", Voltage: " + String(UART.data.inpVoltage) +
              ", AmpHours: " + String(UART.data.ampHours) +
              ", TachAbs: " + String(UART.data.tachometerAbs) + " | ";
  } else {
    output += "VESC -> No Data | ";
  }

  if (sbusDataValid) {
    output += "SBUS -> CH2: " + String(channels[2]) +
              ", Mapped Duty: " + String(duty, 3);
  } else {
    output += "SBUS -> No Data";
  }

  // Output the combined data to the USB Serial monitor
  Serial.println(output);

  // Short delay before the next iteration
  delay(250);
}
