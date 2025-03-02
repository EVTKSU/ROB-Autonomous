#include <Arduino.h>
#include <FlexCAN_T4.h>

// Define CAN command IDs used for control
#define CAN_PACKET_SET_RPM     3   // Command to set RPM (erpm command is sent)
#define CAN_PACKET_SET_CURRENT 1   // Command to set current

#define CONTROLLER_ID          1  // VESC node ID

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanBus;

// --- Buffer helper function ---
// Append a 32-bit integer to the buffer (big-endian)
void buffer_append_int32(uint8_t *buffer, int32_t number, int32_t *index) {
  buffer[(*index)++] = number >> 24;
  buffer[(*index)++] = number >> 16;
  buffer[(*index)++] = number >> 8;
  buffer[(*index)++] = number;
}

// --- Functions to send control commands ---
bool sendRPMCommand(uint8_t controller_id, float rpm) {
  int32_t index = 0;
  uint8_t buffer[4];
  // Convert rpm to electrical rpm (erpm) using motor pole count; assuming 14 poles.
  int32_t erpm = (int32_t)(rpm * 14);
  buffer_append_int32(buffer, erpm, &index);
  
  CAN_message_t msg;
  msg.flags.extended = true;
  msg.id = controller_id | ((uint32_t)CAN_PACKET_SET_RPM << 8);
  msg.len = index;
  return CanBus.write(msg);
}

bool sendCurrentCommand(uint8_t controller_id, float current) {
  int32_t index = 0;
  uint8_t buffer[4];
  // Scale current (A) to the expected value; here multiplied by 1000 as an example.
  int32_t scaled = (int32_t)(current * 1000.0);
  buffer_append_int32(buffer, scaled, &index);
  
  CAN_message_t msg;
  msg.flags.extended = true;
  msg.id = controller_id | ((uint32_t)CAN_PACKET_SET_CURRENT << 8);
  msg.len = index;
  return CanBus.write(msg);
}

// --- Simple interactive menu state ---
enum MenuState {
  WAITING_FOR_SELECTION,
  WAITING_FOR_VALUE
};

MenuState menuState = WAITING_FOR_SELECTION;
char selection = '\0';

void printMenu() {
  Serial.println("Select command:");
  Serial.println("R: Set RPM");
  Serial.println("C: Set Current");
  Serial.print("Enter selection: ");
}

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Interactive Control Menu Program");

  CanBus.begin();
  CanBus.setBaudRate(250000);
  
  printMenu();
}

void loop() {
  // Read incoming Serial characters
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputString.length() > 0) {
        stringComplete = true;
      }
    } else {
      inputString += inChar;
    }
  }
  
  if (stringComplete) {
    if (menuState == WAITING_FOR_SELECTION) {
      selection = toupper(inputString.charAt(0));
      if (selection == 'R' || selection == 'C') {
        Serial.print("Enter value for ");
        Serial.println(selection == 'R' ? "RPM:" : "Current (A):");
        menuState = WAITING_FOR_VALUE;
      } else {
        Serial.println("Invalid selection. Try again.");
        printMenu();
      }
      inputString = "";
      stringComplete = false;
    } else if (menuState == WAITING_FOR_VALUE) {
      float value = inputString.toFloat();
      bool success = false;
      if (selection == 'R') {
        success = sendRPMCommand(CONTROLLER_ID, value);
        Serial.print("Sent RPM command: ");
        Serial.println(value);
      } else if (selection == 'C') {
        success = sendCurrentCommand(CONTROLLER_ID, value);
        Serial.print("Sent Current command: ");
        Serial.println(value);
      }
      if (!success) {
        Serial.println("Failed to send command.");
      }
      // Reset for next command
      menuState = WAITING_FOR_SELECTION;
      printMenu();
      inputString = "";
      stringComplete = false;
    }
  }
}
