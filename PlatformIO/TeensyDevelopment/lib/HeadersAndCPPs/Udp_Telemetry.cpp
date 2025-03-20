#include <SPI.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <cstdio>

// Destination IP and port for telemetry packets (adjust as needed)
IPAddress telemetryDestIP(192, 168, 0, 132);  // Receiver's IP address
#define TELEMETRY_DEST_PORT 8888

// Buffer for outgoing UDP telemetry packets (private to this file)
static char telemetryPacketBuffer[UDP_TX_PACKET_MAX_SIZE];

EthernetUDP TelemetryUdp;  // UDP object for sending telemetry

void setupTelemetryUDP() {
  Serial.println("Initializing Telemetry UDP...");
  // Begin Telemetry UDP on the chosen local port (adjust if necessary)
  TelemetryUdp.begin(8888);
  Serial.println("Telemetry UDP initialized on local port 8888.");
}

void sendTelemetry(float rpm, float vescVoltage, float odrvVoltage, float avgMotorCurrent, float odrvCurrent, float steeringAngle)
{
  // Format the telemetry packet with 6 float values
  snprintf(telemetryPacketBuffer, sizeof(telemetryPacketBuffer), "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
           rpm, vescVoltage, odrvVoltage, avgMotorCurrent, odrvCurrent, steeringAngle);
  Serial.print("Sending telemetry: ");
  Serial.println(telemetryPacketBuffer);
  
  // Send the telemetry packet to the specified destination
  TelemetryUdp.beginPacket(telemetryDestIP, TELEMETRY_DEST_PORT);
  TelemetryUdp.write(telemetryPacketBuffer);
  TelemetryUdp.endPacket();
}
