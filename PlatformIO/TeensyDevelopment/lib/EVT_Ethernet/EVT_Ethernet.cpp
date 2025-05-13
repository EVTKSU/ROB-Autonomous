#include "EVT_Ethernet.h"
#include <SPI.h>
#include <cstdio>
#include <sstream>
#include <vector>
#include <cstdlib>
#include "EVT_VescDriver.h"
#include "EVT_StateMachine.h"
#include "EVT_ODriver.h"
#include "EVT_Ethernet.h"

// Global object definitions.
EthernetUDP Udp;
IPAddress ip(192, 168, 0, 177); // teensy ip defined here
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Internal buffers for UDP packets.
char autoBuffer[UDP_TX_PACKET_MAX_SIZE];
static char telemetryPacketBuffer[UDP_TX_PACKET_MAX_SIZE];


// Telemetry destination details.
static IPAddress telemetryDestIP(192, 168, 0, 132); // pi ip defined here
static const uint16_t TELEMETRY_DEST_PORT = 8888;

// Setup function for initializing Ethernet and UDP.
void setupTelemetryUDP() {
  Serial.println("Initializing Telemetry UDP...");
  Ethernet.begin(mac, ip);
  Udp.begin(8888);
  
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No Ethernet hardware found.");
  } else {
    Serial.println("Ethernet hardware is present.");
  }
  
  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Ethernet cable is connected (Link ON).");
  } else {
    Serial.println("Ethernet cable is not connected (Link OFF).");
  }
  delay(1000);
}

// Function to send telemetry data over UDP and display on Serial.
void sendTelemetry() {
  // Retrieve current ODrive telemetry values.
  // The velocity is obtained here but not used in the current telemetry packet.
  float velocity = odrive.getVelocity();
  
  // Get current ODrive feedback.
  ODriveFeedback fb = odrive.getFeedback();
  float steeringAngle = fb.pos;
  
  // Get ODrive parameters.
  float odrvCurrent = odrive.getParameterAsFloat("ibus");
  float odrvVoltage = odrive.getParameterAsFloat("vbus_voltage");
  
  // Update VESC telemetry.
  vesc1.getVescValues();
  float rpm = vesc1.data.rpm;  // VESC2 will be identical so it doesn't matter.
  float vescVoltage = vesc1.data.inpVoltage;  // VESCs are in parallel so voltage is the same.
  float avgMotorCurrent = vesc1.data.avgInputCurrent + vesc2.data.avgInputCurrent;
    
  // Format telemetry packet.
  snprintf(telemetryPacketBuffer, sizeof(telemetryPacketBuffer),
           "%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f, %.2f",
           StateToString(CurrentState), rpm, vescVoltage, odrvVoltage, avgMotorCurrent, odrvCurrent, steeringAngle, velocity);
  
  // Send telemetry packet over UDP.
  Udp.beginPacket(telemetryDestIP, TELEMETRY_DEST_PORT);
  Udp.write(telemetryPacketBuffer);
  Udp.endPacket();
}

void checkConnection() {
  // Check if the Ethernet cable is connected.
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No Ethernet hardware found.");
    SetErrorState(ERR_ETHERNET, "Ethernet connection severed");
  
  }
}
std::string receiveUdp() {
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int len = Udp.read(autoBuffer, sizeof(autoBuffer) - 1);
    if (len > 0) {
      autoBuffer[len] = '\0';
    }
    Serial.print("Received packet: ");
    Serial.println(autoBuffer);
    return std::string(autoBuffer);
  }
  // Return an empty string if no packet is received.
  return std::string();
}


