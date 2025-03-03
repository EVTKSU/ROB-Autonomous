#include <ODriveUART.h>
#include <SoftwareSerial.h>
#include <VescUart.h>

// -----------------------
// SBUS configuration using an interrupt-driven ring buffer
// -----------------------
#define SBUS_BUFFER_SIZE 64
volatile uint8_t sbusBuffer[SBUS_BUFFER_SIZE];
volatile uint8_t sbusBufferHead = 0;
volatile uint8_t sbusBufferTail = 0;

uint16_t channels[16];  // Array to hold up to 16 SBUS channels
bool sbusFailSafe = false;
bool sbusLostFrame = false;

// -----------------------
// ODrive configuration
// -----------------------
HardwareSerial& odrive_serial = Serial3;
int baudrate = 115200; // Must match what you configure on the ODrive
ODriveUART odrive(odrive_serial);

// -----------------------
// Helper functions for the ring buffer
// -----------------------
uint8_t sbusBufferAvailable() {
  if (sbusBufferHead >= sbusBufferTail)
    return sbusBufferHead - sbusBufferTail;
  else
    return SBUS_BUFFER_SIZE - sbusBufferTail + sbusBufferHead;
}

uint8_t sbusBufferRead() {
  uint8_t val = sbusBuffer[sbusBufferTail];
  sbusBufferTail = (sbusBufferTail + 1) % SBUS_BUFFER_SIZE;
  return val;
}

void sbusBufferWrite(uint8_t byte) {
  uint8_t nextHead = (sbusBufferHead + 1) % SBUS_BUFFER_SIZE;
  if (nextHead != sbusBufferTail) { // Ensure buffer isn't full
    sbusBuffer[sbusBufferHead] = byte;
    sbusBufferHead = nextHead;
  }
  // Else: Buffer is full; byte is discarded.
}

// -----------------------
// serialEvent2 is automatically called when data is available on Serial2 (SBUS)
// -----------------------
void serialEvent2() {
  while (Serial2.available()) {
    uint8_t byte = Serial2.read();
    sbusBufferWrite(byte);
  }
}

// -----------------------
// Process SBUS frame from the ring buffer. Returns true if a valid frame is decoded.
// SBUS frames are 25 bytes long, start with 0x0F, and typically end with 0x00.
// -----------------------
bool processSBUSFrame() {
  const int FRAME_SIZE = 25;
  // Proceed only if enough bytes are available
  while (sbusBufferAvailable() >= FRAME_SIZE) {
    // Peek at the next FRAME_SIZE bytes without removing them
    uint8_t frame[FRAME_SIZE];
    int index = sbusBufferTail;
    for (int i = 0; i < FRAME_SIZE; i++) {
      frame[i] = sbusBuffer[index];
      index = (index + 1) % SBUS_BUFFER_SIZE;
    }
    // Validate the frame by checking start and end bytes
    if (frame[0] == 0x0F && frame[24] == 0x00) {
      // Remove the frame from the buffer
      for (int i = 0; i < FRAME_SIZE; i++) {
        sbusBufferRead();
      }
      
      // Decode channels (11-bit values) from the SBUS frame
      channels[0]  = ((frame[1]    | frame[2] << 8) & 0x07FF);
      channels[1]  = ((frame[2] >> 3 | frame[3] << 5) & 0x07FF);
      channels[2]  = ((frame[3] >> 6 | frame[4] << 2 | frame[5] << 10) & 0x07FF);
      channels[3]  = ((frame[5] >> 1 | frame[6] << 7) & 0x07FF);
      channels[4]  = ((frame[6] >> 4 | frame[7] << 4) & 0x07FF);
      channels[5]  = ((frame[7] >> 7 | frame[8] << 1 | frame[9] << 9) & 0x07FF);
      channels[6]  = ((frame[9] >> 2 | frame[10] << 6) & 0x07FF);
      channels[7]  = ((frame[10] >> 5 | frame[11] << 3) & 0x07FF);
      channels[8]  = ((frame[12]    | frame[13] << 8) & 0x07FF);
      channels[9]  = ((frame[13] >> 3 | frame[14] << 5) & 0x07FF);
      channels[10] = ((frame[14] >> 6 | frame[15] << 2 | frame[16] << 10) & 0x07FF);
      channels[11] = ((frame[16] >> 1 | frame[17] << 7) & 0x07FF);
      channels[12] = ((frame[17] >> 4 | frame[18] << 4) & 0x07FF);
      channels[13] = ((frame[18] >> 7 | frame[19] << 1 | frame[20] << 9) & 0x07FF);
      channels[14] = ((frame[20] >> 2 | frame[21] << 6) & 0x07FF);
      channels[15] = ((frame[21] >> 5 | frame[22] << 3) & 0x07FF);
      
      // Parse flags from frame[23]
      sbusFailSafe = (frame[23] & 0x08) != 0;
      sbusLostFrame = (frame[23] & 0x04) != 0;
      
      return true;
    } else {
      // If the frame is invalid, discard one byte and try again.
      sbusBufferRead();
    }
  }
  return false;
}

// -----------------------
// Main setup and loop
// -----------------------
void setup() {
  // Initialize ODrive UART
  odrive_serial.begin(baudrate);
  Serial.begin(115200); // USB Serial
  while (!Serial) { ; }
  delay(10);
  Serial.println("Established USB Serial :)");
  
  // Initialize SBUS on Serial2 (100000 baud, 8E2 format)
  Serial2.begin(100000, SERIAL_8E2);
  
  delay(500);
  
  // Wait for ODrive to become available
  Serial.println("Waiting for ODrive...");
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.println("Found ODrive! Yippeee!");
  Serial.print("DC voltage: ");
}

void loop() {
  // Process any complete SBUS frame from the ring buffer.
  processSBUSFrame();
  
  // Build one output string containing SBUS data and ODrive parameters.
  String output = "";
  output += "SBUS -> CH2: " + String(channels[1]) + ", CH4: " + String(channels[3]);
  output += " | DC voltage: " + String(odrive.getParameterAsFloat("vbus_voltage"), 2) + " V";
  output += " | ibus: " + String(odrive.getParameterAsFloat("ibus"), 2) + " A";
  
  // Print output with a carriage return to update the same line.
  Serial.print("\r" + output);
  Serial.flush();
  
  delay(10); // Small delay to yield processing time
}
