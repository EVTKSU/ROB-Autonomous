/*******************************************************
   getVescValuesTeensy.ino
   Adapted for Teensy 4.1 using Serial1 for VESC UART.

   Teensy 4.1 Pins for Serial1:
   TX1 = Pin 1
   RX1 = Pin 0
   GND must be shared with the VESC.

   Make sure your VESC is configured for the matching
   UART baud rate (19200 in this example).
*******************************************************/

#include <Arduino.h>
#include <VescUart.h>

/** Instantiate the VescUart class */
VescUart UART;

void setup() {
  // Begin the USB Serial port for debugging
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  // Display a startup message
  Serial.println("Teensy 4.1 VESC UART Example - getVescValuesTeensy.ino");

  // Initialize Serial1 at 19200 baud; adjust if your VESC is configured differently
  Serial1.begin(115200);

  // Set the hardware serial port for the VescUart library
  UART.setSerialPort(&Serial1);
}

void loop() {
  // Request data from VESC
  if (UART.getVescValues()) {
    Serial.print("RPM: ");
    Serial.println(UART.data.rpm);

    Serial.print("Input Voltage: ");
    Serial.println(UART.data.inpVoltage);

    Serial.print("Amp Hours: ");
    Serial.println(UART.data.ampHours);

    Serial.print("Tachometer (Absolute): ");
    Serial.println(UART.data.tachometerAbs);
  } else {
    Serial.println("Failed to get data!");
  }

  // Delay before the next request
  delay(500);
}
