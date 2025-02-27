#include <Arduino.h>

#define COMM_GET_VALUES 4
#define START_BYTE 2
#define END_BYTE 3

// Compute CRC16 using polynomial 0x1021 (big-endian)
uint16_t crc16(const uint8_t* data, uint16_t length) {
  uint16_t crc = 0;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}

// Pack the payload into a full message with framing bytes, CRC, and send it over the given serial port.
// Message format: [START_BYTE, payloadLength, payload..., CRC high, CRC low, END_BYTE]
void sendMessage(HardwareSerial &serial, const uint8_t* payload, uint8_t payloadLength) {
  uint8_t message[300];
  uint8_t index = 0;
  message[index++] = START_BYTE;
  message[index++] = payloadLength;
  for (uint8_t i = 0; i < payloadLength; i++) {
    message[index++] = payload[i];
  }
  uint16_t crc = crc16(payload, payloadLength);
  message[index++] = (crc >> 8) & 0xFF;
  message[index++] = crc & 0xFF;
  message[index++] = END_BYTE;
  serial.write(message, index);
}

// Receive a full UART message and extract its payload.
// Returns true if a valid message is received (with correct CRC and framing) within the timeout.
bool receiveMessage(HardwareSerial &serial, uint8_t* payloadBuffer, uint8_t &payloadLength, unsigned long timeout) {
  unsigned long startTime = millis();
  uint8_t message[300];
  uint8_t messageIndex = 0;
  bool lengthDetermined = false;
  uint8_t expectedLength = 0;
  
  while (millis() - startTime < timeout) {
    if (serial.available()) {
      int byteRead = serial.read();
      if (byteRead < 0)
        continue;
      uint8_t byteVal = (uint8_t)byteRead;
      
      // If this is the first byte, ensure it is the START_BYTE.
      if (messageIndex == 0 && byteVal != START_BYTE)
        continue;
      
      message[messageIndex++] = byteVal;
      
      // After receiving the second byte, determine the expected total message length.
      if (messageIndex == 2) {
        payloadLength = message[1];
        expectedLength = payloadLength + 5;  // start + length + payload + CRC (2 bytes) + end
        lengthDetermined = true;
      }
      
      // Once the expected number of bytes is received...
      if (lengthDetermined && messageIndex == expectedLength) {
        // Check that the end byte is correct.
        if (message[expectedLength - 1] != END_BYTE)
          return false;
        
        // Extract payload from the message.
        for (uint8_t i = 0; i < payloadLength; i++) {
          payloadBuffer[i] = message[2 + i];
        }
        
        // Verify CRC.
        uint16_t receivedCrc = ((uint16_t)message[expectedLength - 3] << 8) | message[expectedLength - 2];
        uint16_t computedCrc = crc16(payloadBuffer, payloadLength);
        if (receivedCrc == computedCrc) {
          return true;
        } else {
          return false;
        }
      }
    }
  }
  return false;
}

// Parse the input voltage from the payload.
// The expected payload structure (big-endian) is:
// [command (1), temp_mosfet (2), temp_motor (2), avg_motor_current (4),
//  avg_input_current (4), skip 8 bytes, duty_cycle (2), rpm (4), input_voltage (2)]
// Input voltage is divided by 10.0.
bool parseInputVoltage(const uint8_t* payload, uint8_t payloadLength, float &inputVoltage) {
  // Total expected payload length is 1 + 2 + 2 + 4 + 4 + 8 + 2 + 4 + 2 = 29 bytes.
  if (payloadLength < 29)
    return false;
  
  uint8_t idx = 1; // Skip command id (first byte)
  idx += 2; // temp_mosfet
  idx += 2; // temp_motor
  idx += 4; // avg_motor_current
  idx += 4; // avg_input_current
  idx += 8; // skip avg_id and avg_iq
  idx += 2; // duty_cycle_now
  idx += 4; // rpm
  // Now, next 2 bytes are the input_voltage.
  int16_t rawVoltage = ((int16_t)payload[idx] << 8) | payload[idx + 1];
  inputVoltage = rawVoltage / 10.0;
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { } // Wait for serial port to be ready
  Serial.println("VESC Voltage Query via UART");

  // Initialize Serial1 for VESC communication
  Serial1.begin(115200);
  delay(100);

  // Prepare the COMM_GET_VALUES payload.
  uint8_t payload[1];
  payload[0] = COMM_GET_VALUES;

  // Send the get_values command to the VESC.
  sendMessage(Serial1, payload, 1);
  Serial.println("Sent COMM_GET_VALUES command");

  // Attempt to receive a response from the VESC.
  uint8_t receivedPayload[300];
  uint8_t receivedPayloadLength = 0;
  if (receiveMessage(Serial1, receivedPayload, receivedPayloadLength, 200)) {
    Serial.print("Received payload length: ");
    Serial.println(receivedPayloadLength);
    // Check that the first byte is the expected command id.
    if (receivedPayload[0] == COMM_GET_VALUES) {
      float voltage = 0.0;
      if (parseInputVoltage(receivedPayload, receivedPayloadLength, voltage)) {
        Serial.print("VESC Input Voltage: ");
        Serial.print(voltage);
        Serial.println(" V");
      } else {
        Serial.println("Failed to parse input voltage from payload");
      }
    } else {
      Serial.print("Unexpected command id: ");
      Serial.println(receivedPayload[0]);
    }
  } else {
    Serial.println("No valid response received from VESC");
  }
}

void loop() {
  // No continuous actions in loop.
}
