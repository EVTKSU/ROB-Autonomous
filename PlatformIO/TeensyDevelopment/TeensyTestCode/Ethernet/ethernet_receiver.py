#include <SPI.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

// MAC address for your Teensy (change as needed)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set a static IP address (change as needed)
IPAddress ip(192, 168, 0, 177);


// Local port to listen on
unsigned int localPort = 8888;

// Buffer for incoming UDP packets
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

EthernetUDP Udp;

void setup() {
  Serial.begin(9600);

  Serial.println("Initializing Ethernet...");
  Ethernet.begin(mac, ip);
  delay(2000); // Allow time for initialization

  // Print network configuration
  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  
  Serial.print("Subnet Mask: ");
  Serial.println(Ethernet.subnetMask());
  
  Serial.print("Gateway IP: ");
  Serial.println(Ethernet.gatewayIP());

  // Print hardware status
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No Ethernet hardware found.");
  } else {
    Serial.println("Ethernet hardware is present.");
  }

  // Print link status
  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Ethernet cable is connected (Link ON).");
  } else {
    Serial.println("Ethernet cable is not connected (Link OFF).");
  }

  // Start UDP
  Udp.begin(localPort);
  Serial.print("Listening on UDP port ");
  Serial.println(localPort);
}

void loop() {
  // Check for incoming UDP packets
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';  // Null-terminate the buffer
    }
    Serial.print("Received packet: ");
    Serial.println(packetBuffer);
  }
}
