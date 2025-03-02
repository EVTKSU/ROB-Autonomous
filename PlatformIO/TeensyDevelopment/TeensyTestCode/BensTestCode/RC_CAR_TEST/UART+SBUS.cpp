/*******************************************************
   Integrated VESC and SBUS Data Example for Teensy 4.1

   VESC:
     - Uses Serial1 for UART communication with the VESC.
     - Baud rate: 115200 (ensure this matches your VESC settings).
     - Outputs: rpm, inpVoltage, ampHours, tachometerAbs.

   SBUS:
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000 with configuration SERIAL_8E2.
     - Outputs: Channels 2 and 4 (channel 2 = channels[1], channel 4 = channels[3]).

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
  Serial.println("Teensy 4.1 Integrated VESC and SBUS Data Example");

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
  // Retrieve VESC data from Serial1
  bool vescDataValid = UART.getVescValues();

  // Retrieve SBUS data from Serial2
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);

  // Build a combined output string for USB Serial
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
    // Only display channels 2 and 4 (channels[1] and channels[3])
    output += "SBUS -> CH2: " + String(channels[1]) +
              ", CH4: " + String(channels[3]);
  } else {
    output += "SBUS -> No Data";
  }

  // Output the combined data to the USB Serial monitor
  Serial.println(output);

  // Short delay to allow for new data
  delay(10);
}
