/*******************************************************
   Combined Integrated VESC and ODrive with SBUS
   for Teensy 4.1

   -- VESC Section --
   VESC:
     - Uses Serial1 for UART communication.
     - Baud rate: 115200.
     - Outputs: rpm, inpVoltage, ampHours, tachometerAbs.
     - Receives duty cycle commands via setDuty().

   SBUS (for VESC):
     - Uses Serial2 for receiving SBUS data.
     - Typical SBUS baud rate: 100000, SERIAL_8E2.
     - Uses channel 1 (channels[1]) for controlling duty cycle.
     - Input range: 350 to 1700.
     - Dead center is 990, with a deadband of ±20 counts.
     - Mapped so that values above 1010 produce forward throttle and below 970 produce reverse throttle.
     - Duty values below 0.07 (absolute) are filtered.

   -- ODrive Section --
   ODrive:
     - Uses Serial3 for UART communication.
     - Baud rate: 115200.
     - Receives commands via setPosition() with a velocity feed-forward value.
     
   SBUS (for ODrive):
     - Uses the same SBUS instance.
     - Uses channel 3 (channels[3]) for position mapping.
     - Input range: 410 to 1811.
     - Mapped so that: lastTargetPosition = normalized * 4.0 - 10.0.

   USB Serial:
     - Used for combined debug output.
     
   NOTE: Ensure all devices share a common ground.
*******************************************************/
#include <Arduino.h>
#include <VescUart.h>
#include <ODriveUART.h>
#include <SBUS.h>
#include <SoftwareSerial.h>

// ---------- VESC Setup (Serial1) ----------
VescUart UART;

// ---------- ODrive Setup (Serial3) ----------
HardwareSerial& odrive_serial = Serial3;
int baudrate = 115200;  // Must match ODrive config
ODriveUART odrive(odrive_serial);

// ---------- SBUS Setup (Serial2) ----------
SBUS sbus(Serial2);
uint16_t channels[10];  // Array to hold SBUS channel values (assumes 10 channels)
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// Global variable for ODrive target position.
float lastTargetPosition = 0.0f;

//////////////////////
// setup() Function
//////////////////////
void setup() {
  // ----- VESC & SBUS Setup (Program 1) -----
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("Teensy 4.1 Integrated VESC and SBUS with Duty Cycle Mapping and Filtering");

  Serial1.begin(115200);
  UART.setSerialPort(&Serial1);

  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);

  // ----- ODrive Setup (Program 2) -----
  odrive_serial.begin(baudrate);
  Serial.print("\r");
  Serial.println("Established USB Serial :)");

  Serial.print("\r");
  Serial.println("Initializing SBUS on Serial2...");
  // (SBUS already initialized above)
  delay(500);

  Serial.print("\r");
  Serial.println("Waiting for ODrive...");
  unsigned long startTime = millis();
  while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTime) < 5000) {
    delay(100);
  }
  if (odrive.getState() == AXIS_STATE_UNDEFINED) {
    Serial.println("ODrive not found! Proceeding without ODrive.");
  } else {
    Serial.print("\r");
    Serial.println("Found ODrive! Yippeee!");
    
    Serial.print("\r");
    Serial.print("DC voltage: ");
    Serial.println(odrive.getParameterAsFloat("vbus_voltage"), 2);
    
    Serial.print("\r");
    Serial.println("Starting motor calibration...");
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    Serial.print("\r");
    Serial.println("Starting encoder offset calibration...");
    odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    delay(4000);
    odrive.clearErrors();
    
    Serial.print("\r");
    Serial.println("Enabling closed loop control...");
    startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime) < 5000) {
      odrive.clearErrors();
      odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
      Serial.print("\r");
      Serial.println("trying again to enable closed loop control");
      delay(10);
    }
    
    Serial.print("\r");
    Serial.println("Setting input mode to TRAP_TRAJ...");
    odrive_serial.println("w axis0.controller.config.input_mode 1");
    delay(100);
    
    Serial.print("\r");
    Serial.println("Setting velocity limit to 200.0...");
    odrive_serial.println("w axis0.controller.config.vel_limit 200.0");
    delay(100);
    
    Serial.print("\r");
    Serial.println("Setting acceleration limit to 100.0...");
    odrive_serial.println("w axis0.controller.config.accel_limit 100.0");
    delay(100);
    
    Serial.print("\r");
    Serial.println("Setting deceleration limit to 100.0...");
    odrive_serial.println("w axis0.controller.config.decel_limit 100.0");
    delay(100);
    
    Serial.print("\r");
    Serial.println("ODrive running!");
  }
  Serial.print("\r");
  Serial.println("Setup complete.\n");
}

//////////////////////
// loop() Function
//////////////////////
void loop() {
  // Call SBUS.read() only once per loop to reduce lag.
  bool sbusDataValid = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);

  // ----- Program 1: VESC Duty Cycle Mapping -----
  bool vescDataValid = UART.getVescValues();
  float realRpm = 0;
  float duty = 0.0;
  if (sbusDataValid) {
    // Using channel index 1 for VESC mapping (per original Program 1)
    int ch_vesc = channels[1];
    if (ch_vesc > 1010) {
      duty = (float)(ch_vesc - 1040) / (1700 - 1040);
    } else if (ch_vesc < 970) {
      duty = (float)(ch_vesc - 970) / (970 - 350);
    } else {
      duty = 0.0;
    }
    if (fabs(duty) < 0.07) {
      duty = 0.0;
    } else {
      UART.setDuty(duty);
    }
    realRpm = (float)(UART.data.rpm) / 30.0;
  }
  String output1 = "";
  if (vescDataValid) {
    output1 += "VESC -> RPM: " + String(realRpm) +
               ", Voltage: " + String(UART.data.inpVoltage) +
               ", AmpHours: " + String(UART.data.ampHours) +
               ", TachAbs: " + String(UART.data.tachometerAbs) + " | ";
  } else {
    output1 += "VESC -> No Data | ";
  }
  if (sbusDataValid) {
    output1 += "SBUS -> CH0: " + String(channels[1]) +
               ", Mapped Duty: " + String(duty, 3);
  } else {
    output1 += "SBUS -> No Data";
  }
  Serial.println(output1);

  // ----- Program 2: ODrive Position Mapping -----
  // Using channel index 3 for ODrive mapping
  const float sbusMin = 410.0f;
  const float sbusMax = 1811.0f;
  const float posRange = 4.0f;
  if (sbusDataValid) {
    float rawValue = (float) channels[3];
    float normalized = (rawValue - sbusMin) / (sbusMax - sbusMin);
    lastTargetPosition = normalized * posRange - 10.0f;
  }
  odrive.setPosition(lastTargetPosition, 40.0f);

  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 100) {
    ODriveFeedback fb = odrive.getFeedback();
    float current = odrive.getParameterAsFloat("ibus");
    float voltage = odrive.getParameterAsFloat("vbus_voltage");
    float rpm = fb.vel * 60.0;
    Serial.print("\r");
    Serial.print(lastTargetPosition, 2);
    Serial.print(",");
    Serial.print(fb.pos, 2);
    Serial.print(",");
    if (isnan(current)) {
      Serial.print("NaN");
    } else {
      Serial.print(current, 2);
    }
    Serial.print(",");
    Serial.print(voltage, 2);
    Serial.print(",");
    Serial.println(rpm, 2);
    lastPrintTime = millis();
  }

  delay(10);
}
