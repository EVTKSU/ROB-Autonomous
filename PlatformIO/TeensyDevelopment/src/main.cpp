#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// Teensy's MAC and static IP
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 177);  // Match your network config

// Port to listen on
const unsigned int localPort = 8888;

// Create EthernetUDP instance
EthernetUDP Udp;

// Buffer to hold incoming packet
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // Standard max size is 24 bytes

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for Serial to be ready (for some Teensy boards)
  }

  Serial.println("Starting UDP Receiver");

  // Start Ethernet and bind UDP
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  // Network checks
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield not found");
  } else {
    Serial.println("Ethernet hardware is present");
  }

  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Ethernet cable is connected");
  } else {
    Serial.println("Ethernet cable is NOT connected pi5 <-> teensy");
  }
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int len = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE - 1);
    if (len > 0) {
      packetBuffer[len] = '\0'; // Null-terminate the string
    }

    Serial.print("Received packet: ");
    Serial.println(packetBuffer);
  }
}
