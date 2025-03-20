#include <SPI.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <iostream> // For debugging, if needed
#include "TheH-File.h"

// Destination IP and port for telemetry packets (adjust as needed)
IPAddress telemetryDestIP(192, 168, 0, 132);  // Receiver's IP address
#define TELEMETRY_DEST_PORT 8888

// Buffer for incoming UDP packets and outgoing telemetry packets
char autoBuffer[UDP_TX_PACKET_MAX_SIZE];
static char telemetryPacketBuffer[UDP_TX_PACKET_MAX_SIZE];

// Global variables to store data from UDP
float steering_angle = 0.0;
float throttle = 0.0;
bool emergency = false;

void setDataFromUDP(const std::string &udpData) {
  std::istringstream ss(udpData);
  std::string token;
  std::vector<std::string> tokens;
  
  // Split the string by commas
  while (std::getline(ss, token, ',')) {
    tokens.push_back(token);
  }
  
  // Expecting at least 3 tokens: steering_angle, throttle, and emergency flag
  if (tokens.size() >= 3) {
    steering_angle = std::atof(tokens[0].c_str());
    throttle = std::atof(tokens[1].c_str());
    emergency = (std::atoi(tokens[2].c_str()) != 0);
  } else {
    std::cerr << "Insufficient data received: " << udpData << std::endl;
  }
}

void setupTelemetryUDP() {
  Serial.println("Initializing Telemetry UDP...");
  // The global UDP object Udp is already initialized in main().
  Serial.println("Telemetry UDP is ready.");
}

void sendTelemetry(float rpm, float vescVoltage, float odrvVoltage, float avgMotorCurrent, float odrvCurrent, float steeringAngle) {
  // Format the telemetry packet with 6 float values.
  snprintf(telemetryPacketBuffer, sizeof(telemetryPacketBuffer),
           "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
           rpm, vescVoltage, odrvVoltage, avgMotorCurrent, odrvCurrent, steeringAngle);
  // Serial.print("Sending telemetry: ");
  // Serial.println(telemetryPacketBuffer);
  
  // Use the global Udp object to send the telemetry packet.
  Udp.beginPacket(telemetryDestIP, TELEMETRY_DEST_PORT);
  Udp.write(telemetryPacketBuffer);
  Udp.endPacket();
}

void receiveUdp() {
  // Use the global Udp object to check for incoming UDP packets.
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int len = Udp.read(autoBuffer, sizeof(autoBuffer) - 1);
    if (len > 0) {
      autoBuffer[len] = '\0';  // Null-terminate the buffer.
    }
    Serial.print("Received packet: ");
    Serial.println(autoBuffer);
    setDataFromUDP(std::string(autoBuffer));
    Serial.print("Steering angle: ");
    Serial.print(steering_angle);
    Serial.print(" throttle %: ");
    Serial.print(throttle);
    Serial.print(" Emergency flag: ");
    Serial.println(emergency);
  }
}
