#include <SPI.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include "TheH-File.h"  // Includes prototypes for setupTelemetryUDP, setupSbus, setupVesc, setupOdrv, updateSbusData, updateVescControl, updateOdrvControl, and updateAutonomousMode

// Global debug strings definitions (resolve undefined references):
String vescDebug = "";
String odrvDebug = "";

// MAC address for your Teensy (change as needed)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set a static IP address (change as needed)
IPAddress ip(192, 168, 0, 177);

// Local port for UDP command reception
unsigned int localPort = 8888;

// Buffer for incoming UDP command packets
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

EthernetUDP Udp;  // UDP object for receiving command packets

void setup() {
  Serial.begin(9600);

  Serial.println("Initializing Ethernet...");
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

  // Start UDP for command reception on the specified local port
  Udp.begin(localPort);
  Serial.print("Listening for commands on UDP port ");
  Serial.println(localPort);

  // Initialize Telemetry UDP (from Udp_Telemetry.cpp)
  setupTelemetryUDP();
  
  // Initialize other modules
  setupSbus();
  setupVesc();
  setupOdrv();

  // Additional initialization can be added here if needed
}
    void loop() {
        if (updateSbusData()) {
            if (channels[6] > 1000) {
                updateAutonomousMode();
                odrvDebug = "Autonomous mode active.";
                // Check for incoming UDP command packets and output them via Serial
                
            }
            } else {
                updateVescControl();
                updateOdrvControl();
            }
                     
    
        }
    