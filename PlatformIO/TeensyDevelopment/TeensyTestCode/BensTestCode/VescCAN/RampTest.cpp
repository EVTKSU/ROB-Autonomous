#include <Arduino.h>
#include <FlexCAN_T4.h>

// Define CAN command IDs and status IDs
#define CAN_PACKET_SET_RPM     3
#define CAN_PACKET_STATUS      9
#define CAN_PACKET_STATUS_5   28

#define CONTROLLER_ID          1  // VESC node ID

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanBus;

// --- Buffer helper functions ---
void buffer_append_int32(uint8_t *buffer, int32_t number, int32_t *index) {
  buffer[(*index)++] = number >> 24;
  buffer[(*index)++] = number >> 16;
  buffer[(*index)++] = number >> 8;
  buffer[(*index)++] = number;
}

int32_t buffer_get_int32(const uint8_t *buffer, int32_t *index) {
  int32_t res = ((uint32_t)buffer[*index] << 24) |
                ((uint32_t)buffer[*index + 1] << 16) |
                ((uint32_t)buffer[*index + 2] << 8) |
                ((uint32_t)buffer[*index + 3]);
  *index += 4;
  return res;
}

int16_t buffer_get_int16(const uint8_t *buffer, int32_t *index) {
  int16_t res = ((uint16_t)buffer[*index] << 8) |
                ((uint16_t)buffer[*index + 1]);
  *index += 2;
  return res;
}

// --- Structure to hold VESC status ---
struct node_status_t {
  float rpm;
  float current;
  float v_in;
};

node_status_t status = {0, 0, 0};

// --- Function to send an RPM command ---
// Converts rpm to electrical rpm (erpm) using motor pole count (assumed 14).
bool sendRPMCommand(uint8_t controller_id, float rpm) {
  int32_t index = 0;
  uint8_t buffer[4];
  int32_t erpm = (int32_t)(rpm * 14);
  buffer_append_int32(buffer, erpm, &index);
  
  CAN_message_t msg;
  msg.flags.extended = true;
  msg.id = controller_id | ((uint32_t)CAN_PACKET_SET_RPM << 8);
  msg.len = index;
  return CanBus.write(msg);
}

// --- Process incoming CAN messages to update status ---
// Instead of using a non-existent available() method, we use read() in a loop.
void processCANMessages() {
  CAN_message_t msg;
  while (CanBus.read(msg)) {
    uint8_t node = msg.id & 0xFF;
    uint8_t cmd  = msg.id >> 8;
    if (node == CONTROLLER_ID) {
      int32_t index = 0;
      if (cmd == CAN_PACKET_STATUS) {
        int32_t raw_rpm = buffer_get_int32(msg.buf, &index);
        int16_t raw_current = buffer_get_int16(msg.buf, &index);
        status.rpm = (float)raw_rpm;
        status.current = (float)raw_current / 10.0;
      } else if (cmd == CAN_PACKET_STATUS_5) {
        int32_t dummy = buffer_get_int32(msg.buf, &index);
        int16_t raw_voltage = buffer_get_int16(msg.buf, &index);
        status.v_in = (float)raw_voltage / 10.0;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("VESC Ramp Test Program");

  CanBus.begin();
  CanBus.setBaudRate(250000);
}

void loop() {
  // Continuously process incoming messages
  processCANMessages();
  
  // --- Ramp parameters ---
  const float maxRPM = 3000.0;     // Maximum RPM to ramp to
  const float step = 100.0;        // RPM increment per step
  const unsigned long stepDelay = 200;   // Delay (ms) between steps
  const unsigned long holdDelay = 2000;    // Hold time at maximum RPM
  
  // --- Ramp Up ---
  for (float rpm = 0; rpm <= maxRPM; rpm += step) {
    sendRPMCommand(CONTROLLER_ID, rpm);
    Serial.print("Ramping up, set RPM: ");
    Serial.println(rpm);
    delay(stepDelay);
    processCANMessages();
  }
  
  // --- Hold at maximum RPM ---
  Serial.print("Holding at max RPM: ");
  Serial.println(maxRPM);
  unsigned long holdStart = millis();
  while (millis() - holdStart < holdDelay) {
    processCANMessages();
    delay(100);
  }
  
  // --- Ramp Down ---
  for (float rpm = maxRPM; rpm >= 0; rpm -= step) {
    sendRPMCommand(CONTROLLER_ID, rpm);
    Serial.print("Ramping down, set RPM: ");
    Serial.println(rpm);
    delay(stepDelay);
    processCANMessages();
  }
  
  // Print final status for monitoring
  Serial.print("Final Status - Voltage: ");
  Serial.print(status.v_in);
  Serial.print(" V, Current: ");
  Serial.print(status.current);
  Serial.print(" A, RPM: ");
  Serial.println(status.rpm);
  
  // Wait before starting the next ramp cycle
  delay(2000);
}
