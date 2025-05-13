/*******************************************************
   Integrated 2 VESCs (Serial1 & Serial5) and SBUS with 
   Duty Cycle Mapping, Filtering, and Minimum RPM Filter 
   for Teensy 4.1

   VESCs:
     - VESC1 on Serial1
     - VESC2 on Serial5
     - Baud rate: 115200 (ensure this matches your VESC settings).
     - Both output telemetry: rpm, inpVoltage, ampHours, tachometerAbs.
     - Both receive duty cycle commands via setDuty().

   SBUS:
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000 with configuration SERIAL_8E2.
     - Uses channel 2 (channels[2]) for controlling duty cycle.
     - SBUS channel 2 input range: 350 to 1700.
     - Mapped to duty cycle range: 0.0 to 1.0.
     - A low-duty filter is applied:
         * Duty values below 0.01 are filtered (set to 0).
         * Additionally, if the duty is below MIN_DUTY_FOR_5_RPM (set to 0.05),
           it is filtered to prevent running the motor under 5 rpm.
           (Adjust MIN_DUTY_FOR_5_RPM as needed for your system.)
           
   USB Serial:
     - Used for combined debug output.
     
   NOTE: Ensure all devices share a common ground.
*******************************************************/
#include <Arduino.h>
#include <VescUart.h>
#include <SBUS.h>

// Instantiate two VescUart objects for VESC communication on Serial1 and Serial5
VescUart UART1;  // For VESC1 (Serial1)
VescUart UART2;  // For VESC2 (Serial5)

// Instantiate SBUS for RC receiver data on Serial2
SBUS sbus(Serial2);

// Array to hold SBUS channel values (assumes 10 channels)
uint16_t channels[10];
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// Define a minimum duty threshold that corresponds to ~5 rpm.
// Adjust this value based on your motor and calibration.
const float MIN_DUTY_FOR_5_RPM = 0.05;

void setup() {
  // Initialize USB Serial for debugging output
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for USB Serial port to connect
  }
  Serial.println("Teensy 4.1: 2 VESCs, SBUS, and Minimum RPM Filter Integrated");

  // Initialize Serial1 for VESC1 communication
  Serial1.begin(115200);
  UART1.setSerialPort(&Serial1);

  // Initialize Serial5 for VESC2 communication
  Serial5.begin(115200);
  UART2.setSerialPort(&Serial5);

  // Initialize Serial2 for SBUS input with typical SBUS parameters:
  // 100000 baud, 8 data bits, even parity, 2 stop bits.
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();

  // Allow time for all interfaces to initialize
  delay(500);
}

void loop() {
  // Retrieve telemetry from both VESCs
  bool vesc1DataValid = UART1.getVescValues();
  bool vesc2DataValid = UART2.getVescValues();

  // Retrieve SBUS data from Serial2
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);
  
  float duty = 0.0;
  if (sbusDataValid) {
    // Map SBUS channel 2 (channels[2]) input (350 to 1700) to duty cycle (0.0 to 1.0)
    duty = (float)(channels[2] - 350) / 1350.0;
    duty = constrain(duty, 0.0, 1.0);
    
    // Filter: if the duty cycle is below 0.01, set it to 0.0
    if (duty < 0.01) {
      duty = 0.0;
    }
    // Additional filter: prevent the motor from running under 5 rpm.
    // If the duty is below the minimum threshold, set it to 0.
    if (duty < MIN_DUTY_FOR_5_RPM) {
      duty = 0.0;
    }
    
    // Send the mapped duty cycle to both VESCs only if duty is non-zero.
    if (duty > 0.0) {
      UART1.setDuty(duty);
      UART2.setDuty(duty);
    }
  }

  // Calculate a readable RPM value from VESC1 telemetry.
  // (Assuming UART1.data.rpm divided by 30 gives the actual RPM.)
  float realRpm = (float)(UART1.data.rpm) / 30.0;

  // Build a combined output string for USB Serial debug
  String output = "";

  if (vesc1DataValid) {
    output += "VESC1 -> RPM: " + String(realRpm, 1) +
              ", Voltage: " + String(UART1.data.inpVoltage) +
              ", AmpHours: " + String(UART1.data.ampHours) +
              ", TachAbs: " + String(UART1.data.tachometerAbs) + " | ";
  } else {
    output += "VESC1 -> No Data | ";
  }

  if (vesc2DataValid) {
    output += "VESC2 -> RPM: " + String((float)(UART2.data.rpm) / 30.0, 1) +
              ", Voltage: " + String(UART2.data.inpVoltage) +
              ", AmpHours: " + String(UART2.data.ampHours) +
              ", TachAbs: " + String(UART2.data.tachometerAbs) + " | ";
  } else {
    output += "VESC2 -> No Data | ";
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
  delay(10);
}
