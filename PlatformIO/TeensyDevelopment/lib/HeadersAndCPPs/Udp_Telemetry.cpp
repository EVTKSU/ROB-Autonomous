#include <SPI.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <cstdio>

// MAC address for your Teensy (change as needed)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set a static IP address for the Teensy (change as needed)
IPAddress ip(192, 168, 0, 177);

// Local port for UDP (can be the same as used elsewhere, if only sending)
unsigned int localPort = 8888;

// Destination IP and port for telemetry packets (adjust these as needed)
IPAddress telemetryDestIP(192, 168, 0, 100);  // Receiver's IP address
#define TELEMETRY_DEST_PORT 8890

// Buffer for outgoing UDP packets
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

EthernetUDP TelemetryUdp;

void setupTelemetryUDP() {
  Serial.begin(9600);
  Serial.println("Initializing Ethernet for Telemetry...");

  Ethernet.begin(mac, ip);
  delay(2000); // Allow time for Ethernet initialization

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(Ethernet.gatewayIP());

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

  // Start UDP on the local port
  TelemetryUdp.begin(localPort);
  Serial.print("Telemetry UDP started on local port ");
  Serial.println(localPort);
}

void sendTelemetry(float rpm, float vescVoltage, float odrvVoltage, float avgMotorCurrent, float odrvCurrent, float steeringAngle)
 {
  snprintf(packetBuffer, sizeof(packetBuffer), "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", rpm, vescVoltage, odrvVoltage,  avgMotorCurrent, odrvCurrent, steeringAngle);
  Serial.print("Sending telemetry: ");
  Serial.println(packetBuffer);
  TelemetryUdp.beginPacket(telemetryDestIP, TELEMETRY_DEST_PORT);
  TelemetryUdp.write(packetBuffer);
  TelemetryUdp.endPacket();
}
