// THIS IS AN ATTEMPT TO RECREATE THE MOTOR.PY PROGRAM IN C++. THIS IS A WORK IN PROGRESS. - ben

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

// Data structure for VESC values
struct VescData {
  float temp_mosfet;
  float temp_motor;
  float avg_motor_current;
  float avg_input_current;
  float duty_cycle_now;
  float rpm;
  float input_voltage;
  // (Other fields can be added as needed)
};

class VescUart {
public:
  VescUart(HardwareSerial* serial, unsigned long baudrate, unsigned long timeout_ms = 100) 
    : _serial(serial), _timeout(timeout_ms) 
  {
    _serial->begin(baudrate);
  }

  // Send a duty cycle command. (Note: In the original Python code,
  // duty of 0.50 is sent but the printed message says "20%".)
  void setDuty(float duty, uint8_t can_id = 0) {
    uint8_t payload[5];
    uint8_t index = 0;
    // If using CAN forwarding, additional bytes would be added.
    payload[index++] = COMM_SET_DUTY;
    int32_t duty_int = (int32_t)(duty * 100000);
    payload[index++] = (duty_int >> 24) & 0xFF;
    payload[index++] = (duty_int >> 16) & 0xFF;
    payload[index++] = (duty_int >> 8) & 0xFF;
    payload[index++] = duty_int & 0xFF;
    
    sendMessage(payload, index);
  }

  // Request and decode VESC values into the provided data structure.
  bool getValues(VescData &data, uint8_t can_id = 0) {
    uint8_t payload[3];
    uint8_t length = 0;
    if (can_id != 0) {
      // For CAN forwarding: payload would be [COMM_FORWARD_CAN, can_id, COMM_GET_VALUES]
      payload[length++] = COMM_FORWARD_CAN;
      payload[length++] = can_id;
    }
    payload[length++] = COMM_GET_VALUES;
    sendMessage(payload, length);

    // Buffer to hold the incoming full message
    uint8_t messageBuffer[300];
    uint8_t messageLength = 0;
    if (receiveMessage(messageBuffer, messageLength)) {
      // The first byte in the received payload is the command id.
      if (messageLength > 0 && messageBuffer[0] == COMM_GET_VALUES) {
        processReadPacket(messageBuffer, messageLength, data);
        return true;
      }
    }
    return false;
  }

private:
  HardwareSerial* _serial;
  unsigned long _timeout; // in milliseconds

  // Compute CRC16 (polynomial 0x1021) over given data.
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

  // Pack the payload into a full message (start byte, length, payload, CRC16, end byte)
  void sendMessage(const uint8_t* payload, uint8_t payloadLength) {
    uint8_t message[300];
    uint8_t index = 0;
    // For short messages (payload length <= 256)
    message[index++] = 2;                 // Start byte
    message[index++] = payloadLength;       // Length byte

    // Copy payload into message
    for (uint8_t i = 0; i < payloadLength; i++) {
      message[index++] = payload[i];
    }
    
    // Compute CRC16 on the payload
    uint16_t crc = crc16(payload, payloadLength);
    message[index++] = (crc >> 8) & 0xFF;
    message[index++] = crc & 0xFF;
    message[index++] = 3;                 // End byte

    _serial->write(message, index);
  }

  // Receive a full UART message with timeout.
  // On success, copies the payload (excluding framing bytes) into payloadBuffer
  // and sets payloadLength to the payload size.
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
        
        // Once we have the first two bytes, we can determine the expected message length.
        if (messageIndex == 2 && message[0] == 2) {
          expectedLength = message[1] + 5;  // start, length, payload, CRC (2 bytes), end
          lengthDetermined = true;
        }
        if (lengthDetermined && messageIndex == expectedLength) {
          // Validate end byte
          if (message[expectedLength - 1] != 3) {
            return false;
          }
          uint8_t plLength = message[1];
          uint16_t receivedCrc = ((uint16_t)message[expectedLength - 3] << 8) | message[expectedLength - 2];
          uint16_t computedCrc = crc16(&message[2], plLength);
          if (receivedCrc == computedCrc) {
            // Copy payload (which starts at index 2) to payloadBuffer.
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

  // Process the received payload from a COMM_GET_VALUES packet.
  // The payload format (after the command ID) is:
  //   int16 temp_mosfet (/10.0), int16 temp_motor (/10.0),
  //   float avg_motor_current (/100.0), float avg_input_current (/100.0),
  //   [skip 8 bytes: avg_id and avg_iq],
  //   int16 duty_cycle_now (/1000.0), float rpm, int16 input_voltage (/10.0)
  void processReadPacket(const uint8_t* payload, uint8_t payloadLength, VescData &data) {
    // The first byte in the payload is the command id (COMM_GET_VALUES)
    uint8_t idx = 1; 
    if (payloadLength < 28) {
      return; // Not enough data received
    }
    // temp_mosfet (int16, big-endian)
    int16_t temp_mosfet = (payload[idx] << 8) | payload[idx+1];
    data.temp_mosfet = temp_mosfet / 10.0f;
    idx += 2;

    // temp_motor (int16)
    int16_t temp_motor = (payload[idx] << 8) | payload[idx+1];
    data.temp_motor = temp_motor / 10.0f;
    idx += 2;

    // avg_motor_current (float, 4 bytes), divide by 100.0
    uint32_t avg_motor_current_int = ((uint32_t)payload[idx] << 24) | ((uint32_t)payload[idx+1] << 16) |
                                     ((uint32_t)payload[idx+2] << 8) | payload[idx+3];
    float avg_motor_current;
    memcpy(&avg_motor_current, &avg_motor_current_int, sizeof(float));
    data.avg_motor_current = avg_motor_current / 100.0f;
    idx += 4;

    // avg_input_current (float, 4 bytes), divide by 100.0
    uint32_t avg_input_current_int = ((uint32_t)payload[idx] << 24) | ((uint32_t)payload[idx+1] << 16) |
                                     ((uint32_t)payload[idx+2] << 8) | payload[idx+3];
    float avg_input_current;
    memcpy(&avg_input_current, &avg_input_current_int, sizeof(float));
    data.avg_input_current = avg_input_current / 100.0f;
    idx += 4;

    // Skip 8 bytes (avg_id and avg_iq)
    idx += 8;

    // duty_cycle_now (int16), divide by 1000.0
    int16_t duty_cycle_now = (payload[idx] << 8) | payload[idx+1];
    data.duty_cycle_now = duty_cycle_now / 1000.0f;
    idx += 2;

    // rpm (float, 4 bytes)
    uint32_t rpm_int = ((uint32_t)payload[idx] << 24) | ((uint32_t)payload[idx+1] << 16) |
                       ((uint32_t)payload[idx+2] << 8) | payload[idx+3];
    float rpm;
    memcpy(&rpm, &rpm_int, sizeof(float));
    data.rpm = rpm;
    idx += 4;

    // input_voltage (int16), divide by 10.0
    int16_t input_voltage = (payload[idx] << 8) | payload[idx+1];
    data.input_voltage = input_voltage / 10.0f;
    idx += 2;
  }
};

VescUart* vesc;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial) { /* wait for serial port to connect */ }

  // Initialize Serial1 for VESC communication (adjust as needed)
  Serial1.begin(115200);
  delay(100);  // Allow time for serial port to initialize

  // Create the VescUart instance using Serial1
  vesc = new VescUart(&Serial1, 115200, 100);

  // Set duty cycle (sending 0.50, though the printed message says 20%)
  vesc->setDuty(0.50);
  Serial.println("Set duty cycle to 20%");

  delay(100);  // Short delay before requesting values

  // Request VESC values and print them if received
  VescData data;
  if (vesc->getValues(data)) {
    Serial.println("Current VESC Values:");
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
    Serial.println("Failed to get VESC values");
  }
}

void loop() {
  // Nothing further to do in loop
}
