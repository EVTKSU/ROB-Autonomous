/*
   VESC-Teensy CAN Example (Single-Frame Status Requests)
   ------------------------------------------------------
   This program demonstrates how to request and receive single-frame status
   packets (e.g., CAN_PACKET_STATUS, command = 9) from a VESC over CAN
   using a Teensy + MCP2515 (coryjfowler's MCP_CAN_lib).

   Hardware/Library Setup:
   1) Teensy SPI pins -> MCP2515 SPI pins (MOSI, MISO, SCK, CS).
   2) MCP2515 INT pin -> Teensy digital pin (CAN0_INT).
   3) CANH, CANL from MCP2515 -> VESC CAN bus (with proper termination).
   4) VESC must be set at 500k baud and have the correct CAN mode (usually "VESC").
   5) Library: https://github.com/coryjfowler/MCP_CAN_lib

   Usage:
   - This code requests CAN_PACKET_STATUS (ID=9) from VESC ID=0 once per second.
   - Modify VESC_ID or command IDs to request other status frames.
   - See decodeStatusFrame9() for an example of parsing the returned data.
*/

#include <SPI.h>
#include <mcp_can.h>

// ----------------------------------------------------------
// USER-MODIFIABLE DEFINITIONS
// ----------------------------------------------------------

// Pin used for MCP2515 CS/SS (modify if wired differently).
#define MCP2515_CS_PIN 10

// Pin connected to MCP2515 INT pin (modify if wired differently).
#define CAN0_INT 2

// Desired VESC device ID for requests (modify if your VESC has a non-zero ID).
#define VESC_ID 0

// The status command you want to request (e.g., 9 = CAN_PACKET_STATUS).
#define STATUS_COMMAND_ID 9

// ----------------------------------------------------------
// MCP_CAN Object Creation
// ----------------------------------------------------------
MCP_CAN CAN0(MCP2515_CS_PIN);

// ----------------------------------------------------------
// Helper to Build Extended CAN ID
// ----------------------------------------------------------
// In the VESC protocol (for these simple commands):
// Bits [28..16]: Unused (0)
// Bits [15..8 ]: Command ID
// Bits [7..0  ]: VESC ID
uint32_t makeExtendedCANId(uint8_t commandId, uint8_t vescId) {
  return ((uint32_t)commandId << 8) | (vescId & 0xFF);
}

// ----------------------------------------------------------
// Function to Request a Status Frame
// ----------------------------------------------------------
// Sends a 4-byte single-frame request (scaled argument = 0).
// Many VESC firmwares broadcast status automatically, but this
// explicit request can be used if the firmware supports it.
void requestStatusFrame(uint8_t vescId, uint8_t commandId) {
  // Build 29-bit Extended ID for VESC commands
  uint32_t extId = makeExtendedCANId(commandId, vescId);

  // For a single-frame command, we send 4 data bytes (all zero).
  uint8_t data[4] = {0, 0, 0, 0};

  // Send the CAN frame
  // Param #1: ID (extId)
  // Param #2: 0 = standard frame, 1 = extended frame
  // Param #3: data length in bytes
  // Param #4: pointer to data array
  byte sendStatus = CAN0.sendMsgBuf(extId, 1, 4, data);

  if (sendStatus == CAN_OK) {
    Serial.println("Request Sent Successfully");
  } else {
    Serial.println("Error Sending Request");
  }
}

// ----------------------------------------------------------
// Example Decoder: CAN_PACKET_STATUS (command=9)
// ----------------------------------------------------------
// According to the docs, the 8-byte response is:
//   Byte 0-3: eRPM (RPM with scaling=1)
//   Byte 4-5: Current (A with scaling=1/10)
//   Byte 6-7: Duty (percent with scaling=1/100)
//
// The data is big-endian in each field.
void decodeStatusFrame9(const uint8_t* rxBuf) {
  // eRPM: 32-bit big-endian
  int32_t eRPM = (int32_t)((uint32_t)rxBuf[0] << 24 |
                           (uint32_t)rxBuf[1] << 16 |
                           (uint32_t)rxBuf[2] <<  8 |
                           (uint32_t)rxBuf[3]);

  // Current: 16-bit big-endian, scale=1/10
  int16_t currentRaw = (int16_t)((rxBuf[4] << 8) | rxBuf[5]);
  float currentA = currentRaw / 10.0f;

  // Duty: 16-bit big-endian, scale=1/100
  int16_t dutyRaw = (int16_t)((rxBuf[6] << 8) | rxBuf[7]);
  float dutyPct = dutyRaw / 100.0f;

  // Print the values
  Serial.print("Status 9 -> eRPM: ");
  Serial.print(eRPM);
  Serial.print(", Current (A): ");
  Serial.print(currentA, 1);
  Serial.print(", Duty (%): ");
  Serial.println(dutyPct, 2);
}

// ----------------------------------------------------------
// Setup
// ----------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Initialize MCP2515 @ 16MHz, with a 500kb/s CAN bus rate and no masks/filters
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("MCP2515 Initialization Failed!");
  }

  // Change to normal mode to allow sending/receiving
  CAN0.setMode(MCP_NORMAL);

  // Configure the INT pin
  pinMode(CAN0_INT, INPUT);

  Serial.println("Beginning VESC Single-Frame Status Request Example...");
  delay(1000);
}

// ----------------------------------------------------------
// Main Loop
// ----------------------------------------------------------
void loop() {
  // 1) Request a status frame from the VESC
  requestStatusFrame(VESC_ID, STATUS_COMMAND_ID);

  // 2) Give the VESC a moment to respond (if it is configured to do so)
  delay(100);

  // 3) Check for incoming CAN frames
  if (!digitalRead(CAN0_INT)) {
    long unsigned int rxId = 0;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    // Read the next available CAN frame
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    // Check if this is an extended frame
    bool isExtended = (rxId & 0x80000000) != 0 ? true : false;
    // Remove the extended flag bit so we can parse the actual 29-bit ID
    if (isExtended) {
      rxId &= 0x1FFFFFFF;
    }

    // Extract command (bits 15..8) and vescId (bits 7..0)
    uint8_t cmd = (rxId >> 8) & 0xFF;
    uint8_t vescIdRx = rxId & 0xFF;

    // If it's the command we're interested in, decode it
    if (cmd == 9) {  // CAN_PACKET_STATUS
      // Confirm we got 8 data bytes
      if (len == 8) {
        decodeStatusFrame9(rxBuf);
      }
    }
    // Add more "else if" or "switch" blocks here for other command IDs
  }

  // 4) Wait before the next request
  delay(900);
}
