#include "EVT_Ethernet.h"
#include <SPI.h>
#include <cstdio>
#include <sstream>
#include <vector>
#include <cstdlib>

// Global object definitions.
EthernetUDP Udp;
IPAddress ip(192, 168, 0, 177);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Internal buffers for UDP packets.
char autoBuffer[UDP_TX_PACKET_MAX_SIZE];
static char telemetryPacketBuffer[UDP_TX_PACKET_MAX_SIZE];


// Telemetry destination details.
static IPAddress telemetryDestIP(192, 168, 0, 132);
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
void sendTelemetry(float rpm, float vescVoltage, float odrvVoltage, float avgMotorCurrent, float odrvCurrent, float steeringAngle) {
  snprintf(telemetryPacketBuffer, sizeof(telemetryPacketBuffer),
           "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
           rpm, vescVoltage, odrvVoltage, avgMotorCurrent, odrvCurrent, steeringAngle);
  
  // Display telemetry data on Serial.
 // Serial.print("Sending telemetry: ");
  //Serial.println(telemetryPacketBuffer);
  
  Udp.beginPacket(telemetryDestIP, TELEMETRY_DEST_PORT);
  Udp.write(telemetryPacketBuffer);
  Udp.endPacket();
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


