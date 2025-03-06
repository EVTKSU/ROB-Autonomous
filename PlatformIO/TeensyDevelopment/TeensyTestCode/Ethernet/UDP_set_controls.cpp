#include <SPI.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <iostream> // For debugging, if needed

// MAC address for your Teensy (change as needed)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set a static IP address (change as needed)
IPAddress ip(192, 168, 0, 177);

// Local port to listen on
unsigned int localPort = 8888;

// Buffer for incoming UDP packets
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// Global variables that store our data
float steering_angle = 0.0;
float throttle = 0.0;
bool emergency = false;

void setDataFromUDP(const std::string &udpData) {
    // Create a string stream from the input data
    std::istringstream ss(udpData);
    std::string token;
    std::vector<std::string> tokens;

    // Split the string by commas
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    // Expecting at least 3 tokens: steering_angle, throttle, and emergency flag
    if (tokens.size() >= 3) {
        try {
            steering_angle = std::stof(tokens[0]);
            throttle = std::stof(tokens[1]);
            emergency = (std::stoi(tokens[2]) != 0);
        } catch (const std::exception &e) {
            std::cerr << "Error parsing UDP data: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Insufficient data received: " << udpData << std::endl;
    }
}


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
    setDataFromUDP(packetBuffer);
    Serial.print("Steering angle: ");
    Serial.print(steering_angle);
    Serial.print("throttle %: ");
    Serial.print(throttle);
    Serial.print("Emergency flag: ");
    Serial.print(emergency);
    
  }
}
