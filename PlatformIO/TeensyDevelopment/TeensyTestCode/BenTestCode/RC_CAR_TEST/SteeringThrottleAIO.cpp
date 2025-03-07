#include <Arduino.h>
#include <VescUart.h>
#include <ODriveUART.h>
#include <SBUS.h>
#include <SoftwareSerial.h>

// ---------- Global Objects ----------

// VESC on Serial1
VescUart UART;

// ODrive on Serial3
HardwareSerial& odrive_serial = Serial3;
int baudrate = 115200;  // Must match ODrive config
ODriveUART odrive(odrive_serial);

// SBUS on Serial2
SBUS sbus(Serial2);
uint16_t channels[10];  // Assumes 10 channels
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// Global variable for ODrive target position (Program 2)
float lastTargetPosition = 0.0f;

//////////////////////
// setup() Function
//////////////////////
void setup() {
  // ----- Common Setup -----
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("Teensy 4.1 Integrated VESC, ODrive, and SBUS");

  // ----- VESC & SBUS Setup (Program 1) -----
  Serial.println("Initializing VESC on Serial1...");
  Serial1.begin(115200);
  UART.setSerialPort(&Serial1);

  Serial.println("Initializing SBUS on Serial2...");
  Serial2.begin(100000, SERIAL_8E2);
  sbus.begin();
  delay(500);

  // ----- ODrive Setup (Program 2) -----
  Serial.println("Initializing ODrive on Serial3...");
  odrive_serial.begin(baudrate);

  // Add a timeout to avoid waiting forever for ODrive
  Serial.println("Waiting for ODrive...");
  unsigned long startTime = millis();
  while (odrive.getState() == AXIS_STATE_UNDEFINED && (millis() - startTime) < 5000) {
    delay(100);
  }
  if (odrive.getState() == AXIS_STATE_UNDEFINED) {
    Serial.println("ODrive not found! Proceeding without ODrive.");
  } else {
    Serial.println("Found ODrive! Yippeee!");
    Serial.print("DC voltage: ");
    Serial.println(odrive.getParameterAsFloat("vbus_voltage"), 2);

    Serial.println("Starting motor calibration...");
    odrive.setState(AXIS_STATE_MOTOR_CALIBRATION);
    delay(4000);
    odrive.clearErrors();

    Serial.println("Starting encoder offset calibration...");
    odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
    delay(4000);
    odrive.clearErrors();

    Serial.println("Enabling closed loop control...");
    startTime = millis();
    while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL && (millis() - startTime) < 5000) {
      odrive.clearErrors();
      odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
      Serial.println("trying again to enable closed loop control");
      delay(10);
    }

    Serial.println("Setting input mode to TRAP_TRAJ...");
    odrive_serial.println("w axis0.controller.config.input_mode 1");
    delay(100);

    Serial.println("Setting velocity limit to 200.0...");
    odrive_serial.println("w axis0.controller.config.vel_limit 200.0");
    delay(100);

    Serial.println("Setting acceleration limit to 100.0...");
    odrive_serial.println("w axis0.controller.config.accel_limit 100.0");
    delay(100);

    Serial.println("Setting deceleration limit to 100.0...");
    odrive_serial.println("w axis0.controller.config.decel_limit 100.0");
    delay(100);

    Serial.println("ODrive running!");
  }
  Serial.println("Setup complete.\n");
}

//////////////////////
// loop() Function
//////////////////////
void loop() {
  // ----- Program 1: VESC Duty Cycle Mapping -----
  bool vescDataValid = UART.getVescValues();
  bool sbusDataValid1 = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);
  float realRpm = 0;
  float duty = 0.0;
  if (sbusDataValid1) {
    // Using channel index 1 for VESC mapping (per original logic)
    int ch = channels[1];
    if (ch > 1010) {
      duty = (float)(ch - 1040) / (1700 - 1040);  // maps from 0 at 1010 to +1 at 1700
    } else if (ch < 970) {
      duty = (float)(ch - 970) / (970 - 350);     // maps from 0 at 970 to -1 at 350
    } else {
      duty = 0.0;
    }
    if (fabs(duty) < 0.07) {
      duty = 0.0;
    } else {
      UART.setDuty(duty);
    }
    realRpm = (float)(UART.data.rpm / 30);
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
  if (sbusDataValid1) {
    output1 += "SBUS -> CH0: " + String(channels[1]) +
               ", Mapped Duty: " + String(duty, 3);
  } else {
    output1 += "SBUS -> No Data";
  }
  Serial.println(output1);

  // ----- Program 2: ODrive Position Mapping -----
  bool sbusDataValid2 = sbus.read(&channels[0], &sbusFailSafe, &sbusLostFrame);
  const float sbusMin = 410.0f;
  const float sbusMax = 1811.0f;
  const float posRange = 4.0f;
  if (sbusDataValid2) {
    float rawValue = (float) channels[3];  // using channel index 3 for ODrive mapping
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
