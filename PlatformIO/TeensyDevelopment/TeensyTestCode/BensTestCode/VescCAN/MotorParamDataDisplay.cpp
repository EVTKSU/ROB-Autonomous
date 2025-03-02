#include <Arduino.h>
#include <FlexCAN_T4.h>

// --- Define CAN command IDs (based on VESC CAN protocol) ---
#define CAN_PACKET_STATUS      9   // Contains RPM and current
#define CAN_PACKET_STATUS_2   10   // (Hypothetical) Contains duty cycle data
#define CAN_PACKET_STATUS_5   28   // Contains voltage info
#define CAN_PACKET_PING       17   // Ping to trigger status update

#define CONTROLLER_ID         1   // VESC node ID (adjust if needed)

// --- Instantiate CAN bus on CAN2 with defined RX/TX queue sizes ---
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanBus;

// --- Global structure to store status values ---
struct node_status_t {
  float rpm;      // Mechanical RPM
  float current;  // Current in A
  float v_in;     // Input voltage in V
  float duty;     // Duty cycle in percent
};

node_status_t status = {0, 0, 0, 0};

// --- Buffer helper functions ---
// Reads a 32-bit big-endian integer from a buffer.
int32_t buffer_get_int32(const uint8_t *buffer, int32_t *index) {
  int32_t res = ((uint32_t)buffer[*index] << 24) |
                ((uint32_t)buffer[*index + 1] << 16) |
                ((uint32_t)buffer[*index + 2] << 8) |
                ((uint32_t)buffer[*index + 3]);
  *index += 4;
  return res;
}

// Reads a 16-bit big-endian integer from a buffer.
int16_t buffer_get_int16(const uint8_t *buffer, int32_t *index) {
  int16_t res = ((uint16_t)buffer[*index] << 8) |
                ((uint16_t)buffer[*index + 1]);
  *index += 2;
  return res;
}

// --- Send a ping command to request status from the VESC ---
bool sendPing(uint8_t controller_id) {
  CAN_message_t msg;
  msg.flags.extended = true;
  // Message ID: lower 8 bits = node ID, upper 8 bits = command ID.
  msg.id = controller_id | ((uint32_t)CAN_PACKET_PING << 8);
  msg.len = 0; // No payload for ping.
  return CanBus.write(msg);
}

// --- Process incoming CAN messages ---
// This function reads all available messages and updates our status structure.
void processCANMessages() {
  CAN_message_t msg;
  // Instead of using an "available()" method, repeatedly call read()
  while (CanBus.read(msg)) {
    uint8_t node = msg.id & 0xFF;
    uint8_t cmd  = msg.id >> 8;
    if (node == CONTROLLER_ID) {
      int32_t idx = 0;
      if (cmd == CAN_PACKET_STATUS) {
        // Expecting 4 bytes for RPM and 2 bytes for current.
        int32_t raw_rpm = buffer_get_int32(msg.buf, &idx);
        int16_t raw_current = buffer_get_int16(msg.buf, &idx);
        status.rpm = (float)raw_rpm;
        status.current = (float)raw_current / 10.0;  // Scale factor per VESC example.
      } else if (cmd == CAN_PACKET_STATUS_2) {
        // Hypothetical packet for duty cycle: assume 2 bytes, scaled by 100.
        int16_t raw_duty = buffer_get_int16(msg.buf, &idx);
        status.duty = (float)raw_duty / 100.0;
      } else if (cmd == CAN_PACKET_STATUS_5) {
        // Expecting 4 bytes dummy + 2 bytes voltage.
        int32_t dummy = buffer_get_int32(msg.buf, &idx);  // Unused.
        int16_t raw_voltage = buffer_get_int16(msg.buf, &idx);
        status.v_in = (float)raw_voltage / 10.0;  // Example scaling.
      }
    }
  }
}

// --- Update the serial dashboard display ---
// This function uses ANSI escape codes to position the cursor and update only the numerical values.
// It computes wattage (voltage * current) on the fly.
void updateDisplay() {
  float wattage = status.v_in * status.current;
  char buf[16];
  // Update Voltage (line 1, column 12)
  sprintf(buf, "%8.2f", status.v_in);
  Serial.print("\033[1;12H");
  Serial.print(buf);
  
  // Update Current (line 2, column 12)
  sprintf(buf, "%8.2f", status.current);
  Serial.print("\033[2;12H");
  Serial.print(buf);
  
  // Update RPM (line 3, column 12)
  sprintf(buf, "%8.0f", status.rpm);
  Serial.print("\033[3;12H");
  Serial.print(buf);
  
  // Update Wattage (line 4, column 12)
  sprintf(buf, "%8.2f", wattage);
  Serial.print("\033[4;12H");
  Serial.print(buf);
  
  // Update Duty Cycle (line 5, column 12)
  sprintf(buf, "%8.2f", status.duty);
  Serial.print("\033[5;12H");
  Serial.print(buf);
  
  // Update Additional info (line 6, column 12) – placeholder text.
  Serial.print("\033[6;12H");
  Serial.print("   N/A  ");
}

// --- Timing variables ---
unsigned long lastPingTime = 0;
unsigned long lastDisplayTime = 0;
const unsigned long pingInterval = 500;    // Ping every 500ms.
const unsigned long displayInterval = 500; // Update display every 500ms.

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  // Clear the screen and set up the dashboard header.
  Serial.print("\033[2J"); // Clear entire screen.
  Serial.print("\033[H");  // Move cursor to home position.
  // Print header with fixed fields (the numerical values will update in place).
  Serial.println("Voltage:    [        ] V");
  Serial.println("Current:    [        ] A");
  Serial.println("RPM:        [        ] rpm");
  Serial.println("Wattage:    [        ] W");
  Serial.println("Duty Cycle: [        ] %");
  Serial.println("Additional: [        ]");
  
  // Initialize CAN bus on CAN2 at 250 kbps.
  CanBus.begin();
  CanBus.setBaudRate(250000);
  
  lastPingTime = millis();
  lastDisplayTime = millis();
}

void loop() {
  processCANMessages();
  
  unsigned long now = millis();
  // Periodically send a ping to request updated status.
  if (now - lastPingTime >= pingInterval) {
    lastPingTime = now;
    sendPing(CONTROLLER_ID);
  }
  
  // Periodically update the display.
  if (now - lastDisplayTime >= displayInterval) {
    lastDisplayTime = now;
    updateDisplay();
  }
}
