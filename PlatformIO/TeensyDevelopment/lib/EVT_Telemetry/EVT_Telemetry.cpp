#include "EVT_Telemetry.h"
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

// Global variables for UDP data processing.
float steering_angle = 0.0;
float throttle = 0.0;
bool emergency = false;

void setDataFromUDP(const std::string &udpData) {
  std::istringstream ss(udpData);
  std::string token;
  std::vector<std::string> tokens;
  
  while (std::getline(ss, token, ',')) {
    tokens.push_back(token);
  }
  
  if (tokens.size() >= 3) {
    steering_angle = std::atof(tokens[0].c_str());
    throttle = std::atof(tokens[1].c_str());
    emergency = (std::atoi(tokens[2].c_str()) != 0);
  } else {
    Serial.print("Insufficient data received: ");
    Serial.println(udpData.c_str());
  }
}

// Telemetry destination details (local to this file).
static IPAddress telemetryDestIP(192, 168, 0, 132);
static const uint16_t TELEMETRY_DEST_PORT = 8888;

void setupTelemetryUDP() {
  Serial.println("Initializing Telemetry UDP...");
  Udp.begin(8888);
  Ethernet.begin(mac, ip);

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

void sendTelemetry(float rpm, float vescVoltage, float odrvVoltage, float avgMotorCurrent, float odrvCurrent, float steeringAngle) {
  snprintf(telemetryPacketBuffer, sizeof(telemetryPacketBuffer),
           "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
           rpm, vescVoltage, odrvVoltage, avgMotorCurrent, odrvCurrent, steeringAngle);
  Udp.beginPacket(telemetryDestIP, TELEMETRY_DEST_PORT);
  Udp.write(telemetryPacketBuffer);
  Udp.endPacket();
}

void receiveUdp() {
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int len = Udp.read(autoBuffer, sizeof(autoBuffer) - 1);
    if (len > 0) {
      autoBuffer[len] = '\0';
    }
    Serial.print("Received packet: ");
    Serial.println(autoBuffer);
    setDataFromUDP(std::string(autoBuffer));
    Serial.print("Steering angle: ");
    Serial.println(steering_angle);
    Serial.print("Throttle: ");
    Serial.println(throttle);
    Serial.print("Emergency flag: ");
    Serial.println(emergency ? "true" : "false");
  }
}
