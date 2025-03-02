#include <Arduino.h>
#include <FlexCAN_T4.h>

// Define CAN command IDs (using indices from the VESC CAN command enumeration)
#define CAN_PACKET_STATUS      9
#define CAN_PACKET_PING       17
#define CAN_PACKET_STATUS_5   28

#define CONTROLLER_ID         1  // Change as needed for your VESC node ID

// Global CAN bus instance on CAN2 with buffers
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanBus;

// Structure to hold VESC status values
struct node_status_t {
  float rpm;
  float current;
  float v_in;
};

node_status_t status = {0, 0, 0};

// --- Buffer helper functions ---
// Read a 32-bit integer from the buffer (big-endian)
int32_t buffer_get_int32(const uint8_t *buffer, int32_t *index) {
  int32_t res = ((uint32_t)buffer[*index] << 24) |
                ((uint32_t)buffer[*index + 1] << 16) |
                ((uint32_t)buffer[*index + 2] << 8) |
                ((uint32_t)buffer[*index + 3]);
  *index += 4;
  return res;
}

// Read a 16-bit integer from the buffer (big-endian)
int16_t buffer_get_int16(const uint8_t *buffer, int32_t *index) {
  int16_t res = ((uint16_t)buffer[*index] << 8) |
                ((uint16_t)buffer[*index + 1]);
  *index += 2;
  return res;
}

// --- CAN send function for the ping command ---
bool sendPing(uint8_t controller_id) {
  CAN_message_t msg;
  msg.flags.extended = true;
  // Build the message ID: lower 8 bits are the node ID, upper 8 bits the command
  msg.id = controller_id | ((uint32_t)CAN_PACKET_PING << 8);
  msg.len = 0;  // No payload for a ping
  return CanBus.write(msg);
}

// --- Process incoming CAN messages ---
// Decodes messages with extended IDs that carry status data.
void processCANMessages() {
  CAN_message_t msg;
  // Continuously read messages until none remain
  while (CanBus.read(msg)) {
    uint8_t node = msg.id & 0xFF;
    uint8_t cmd  = msg.id >> 8;
    if (node == CONTROLLER_ID) {
      int32_t index = 0;
      if (cmd == CAN_PACKET_STATUS) {
        // First status packet: contains RPM and current.
        int32_t raw_rpm = buffer_get_int32(msg.buf, &index);
        int16_t raw_current = buffer_get_int16(msg.buf, &index);
        status.rpm = (float)raw_rpm;
        status.current = (float)raw_current / 10.0;  // Scale as in VESC example
      } else if (cmd == CAN_PACKET_STATUS_5) {
        // Second status packet: contains voltage.
        int32_t dummy = buffer_get_int32(msg.buf, &index);  // Unused 32-bit value
        int16_t raw_voltage = buffer_get_int16(msg.buf, &index);
        status.v_in = (float)raw_voltage / 10.0;  // Voltage scaling (example)
      }
    }
  }
}

unsigned long lastRequestTime = 0;
const unsigned long requestInterval = 5000; // 5 seconds

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("VESC Status Polling Program");

  CanBus.begin();
  CanBus.setBaudRate(250000);
  
  lastRequestTime = millis();
}

void loop() {
  processCANMessages();

  if (millis() - lastRequestTime >= requestInterval) {
    lastRequestTime = millis();
    if (sendPing(CONTROLLER_ID)) {
      Serial.println("Ping request sent");
    } else {
      Serial.println("Failed to send ping");
    }
    
    // Display the latest status
    Serial.print("Voltage: ");
    Serial.print(status.v_in);
    Serial.print(" V, Current: ");
    Serial.print(status.current);
    Serial.print(" A, RPM: ");
    Serial.println(status.rpm);
  }
}
