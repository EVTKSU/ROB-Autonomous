#include <Arduino.h>
#include <stdint.h>
#include <string.h>

// VESC Command IDs
#define COMM_FW_VERSION        0
#define COMM_GET_VALUES        4
#define COMM_SET_DUTY          5
#define COMM_SET_CURRENT       6
#define COMM_SET_CURRENT_BRAKE 7
#define COMM_SET_RPM           8
#define COMM_SET_CHUCK_DATA    23
#define COMM_ALIVE             29
#define COMM_FORWARD_CAN       33

// RC receiver analog input pin (adjust as needed)
#define RC_INPUT_PIN A0

// Data structure for VESC values
struct VescData {
  float temp_mosfet;
  float temp_motor;
  float avg_motor_current;
  float avg_input_current;
  float duty_cycle_now;
  float rpm;
  float input_voltage;
};

class VescUart {
public:
  VescUart(HardwareSerial* serial, unsigned long baudrate, unsigned long timeout_ms = 100) 
    : _serial(serial), _timeout(timeout_ms) 
  {
    _serial->begin(baudrate);
  }

  // Send duty cycle command
  void setDuty(float duty, uint8_t can_id = 0) {
    uint8_t payload[5];
    uint8_t index = 0;
    payload[index++] = COMM_SET_DUTY;
    int32_t duty_int = (int32_t)(duty * 100000);
    payload[index++] = (duty_int >> 24) & 0xFF;
    payload[index++] = (duty_int >> 16) & 0xFF;
    payload[index++] = (duty_int >> 8) & 0xFF;
    payload[index++] = duty_int & 0xFF;
    
    sendMessage(payload, index);
  }

  // Request VESC values and decode into data structure
  bool getValues(VescData &data, uint8_t can_id = 0) {
    uint8_t payload[3];
    uint8_t length = 0;
    if (can_id != 0) {
      payload[length++] = COMM_FORWARD_CAN;
      payload[length++] = can_id;
    }
    payload[length++] = COMM_GET_VALUES;
    sendMessage(payload, length);

    uint8_t messageBuffer[300];
    uint8_t messageLength = 0;
    if (receiveMessage(messageBuffer, messageLength)) {
      if (messageLength > 0 && messageBuffer[0] == COMM_GET_VALUES) {
        processReadPacket(messageBuffer, messageLength, data);
        return true;
      }
    }
    return false;
  }

private:
  HardwareSerial* _serial;
  unsigned long _timeout; // milliseconds

  // Compute CRC16 (polynomial 0x1021)
  uint16_t crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0;
    for (size_t i = 0; i < length; i++) {
      crc ^= (uint16_t)data[i] << 8;
      for (uint8_t j = 0; j < 8; j++) {
        if (crc & 0x8000) {
          crc = (crc << 1) ^ 0x1021;
        } else {
          crc <<= 1;
        }
      }
      crc &= 0xFFFF;
    }
    return crc;
  }

  // Pack payload with framing and send over UART
  void sendMessage(const uint8_t* payload, uint8_t payloadLength) {
    uint8_t message[300];
    uint8_t index = 0;
    message[index++] = 2;          // Start byte
    message[index++] = payloadLength; // Length byte

    for (uint8_t i = 0; i < payloadLength; i++) {
      message[index++] = payload[i];
    }
    
    uint16_t crc = crc16(payload, payloadLength);
    message[index++] = (crc >> 8) & 0xFF;
    message[index++] = crc & 0xFF;
    message[index++] = 3;          // End byte

    _serial->write(message, index);
  }

  // Receive a full UART message with timeout.
  bool receiveMessage(uint8_t* payloadBuffer, uint8_t &payloadLength) {
    unsigned long startTime = millis();
    uint8_t message[300];
    uint8_t messageIndex = 0;
    uint8_t expectedLength = 0;
    bool lengthDetermined = false;

    while (millis() - startTime < _timeout) {
      if (_serial->available()) {
        int byteRead = _serial->read();
        if (byteRead < 0) continue;
        uint8_t byteVal = (uint8_t)byteRead;
        message[messageIndex++] = byteVal;
        
        if (messageIndex == 2 && message[0] == 2) {
          expectedLength = message[1] + 5;  // start, length, payload, CRC (2 bytes), end
          lengthDetermined = true;
        }
        if (lengthDetermined && messageIndex == expectedLength) {
          if (message[expectedLength - 1] != 3) {
            return false;
          }
          uint8_t plLength = message[1];
          uint16_t receivedCrc = ((uint16_t)message[expectedLength - 3] << 8) | message[expectedLength - 2];
          uint16_t computedCrc = crc16(&message[2], plLength);
          if (receivedCrc == computedCrc) {
            for (uint8_t i = 0; i < plLength; i++) {
              payloadBuffer[i] = message[2 + i];
            }
            payloadLength = plLength;
            return true;
          } else {
            return false;
          }
        }
      }
    }
    return false;
  }

  // Process the COMM_GET_VALUES packet payload
  void processReadPacket(const uint8_t* payload, uint8_t payloadLength, VescData &data) {
    uint8_t idx = 1; 
    if (payloadLength < 28) {
      return;
    }
    int16_t temp_mosfet = (payload[idx] << 8) | payload[idx+1];
    data.temp_mosfet = temp_mosfet / 10.0f;
    idx += 2;

    int16_t temp_motor = (payload[idx] << 8) | payload[idx+1];
    data.temp_motor = temp_motor / 10.0f;
    idx += 2;

    uint32_t avg_motor_current_int = ((uint32_t)payload[idx] << 24) | ((uint32_t)payload[idx+1] << 16) |
                                     ((uint32_t)payload[idx+2] << 8) | payload[idx+3];
    float avg_motor_current;
    memcpy(&avg_motor_current, &avg_motor_current_int, sizeof(float));
    data.avg_motor_current = avg_motor_current / 100.0f;
    idx += 4;

    uint32_t avg_input_current_int = ((uint32_t)payload[idx] << 24) | ((uint32_t)payload[idx+1] << 16) |
                                     ((uint32_t)payload[idx+2] << 8) | payload[idx+3];
    float avg_input_current;
    memcpy(&avg_input_current, &avg_input_current_int, sizeof(float));
    data.avg_input_current = avg_input_current / 100.0f;
    idx += 4;

    idx += 8; // Skip avg_id and avg_iq

    int16_t duty_cycle_now = (payload[idx] << 8) | payload[idx+1];
    data.duty_cycle_now = duty_cycle_now / 1000.0f;
    idx += 2;

    uint32_t rpm_int = ((uint32_t)payload[idx] << 24) | ((uint32_t)payload[idx+1] << 16) |
                       ((uint32_t)payload[idx+2] << 8) | payload[idx+3];
    float rpm;
    memcpy(&rpm, &rpm_int, sizeof(float));
    data.rpm = rpm;
    idx += 4;

    int16_t input_voltage = (payload[idx] << 8) | payload[idx+1];
    data.input_voltage = input_voltage / 10.0f;
    idx += 2;
  }
};

VescUart* vesc;

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for serial port connection */ }

  // Set analog read resolution to 8-bit so values are in the range 0-255.
  analogReadResolution(8);
  pinMode(RC_INPUT_PIN, INPUT);

  // Initialize Serial1 for VESC communication (adjust pin if necessary)
  Serial1.begin(115200);
  delay(100);

  vesc = new VescUart(&Serial1, 115200, 100);
  Serial.println("Teensy RC Receiver to VESC Duty Cycle Controller Initialized.");
}

void loop() {
  // Read the analog value (0-255) from the RC receiver input.
  uint8_t pwmValue = analogRead(RC_INPUT_PIN);
  // Convert the 8-bit value to a float duty cycle in the range 0.0 to 1.0.
  float dutyCycle = pwmValue / 255.0f;

  Serial.print("Analog Value: ");
  Serial.print(pwmValue);
  Serial.print(" -> Duty Cycle: ");
  Serial.println(dutyCycle, 3);

  // Send the duty cycle command to the VESC.
  vesc->setDuty(dutyCycle);

  // Optionally, request and print VESC values.
  VescData data;
  if (vesc->getValues(data)) {
    Serial.println("VESC Values:");
    Serial.print("Duty Cycle: ");
    Serial.print(data.duty_cycle_now * 100.0f, 1);
    Serial.println("%");
    Serial.print("RPM: ");
    Serial.println(data.rpm, 0);
    Serial.print("Current: ");
    Serial.print(data.avg_motor_current, 1);
    Serial.println("A");
    Serial.print("Voltage: ");
    Serial.print(data.input_voltage, 1);
    Serial.println("V");
  } else {
    Serial.println("Failed to get VESC values.");
  }

  delay(50);
}
