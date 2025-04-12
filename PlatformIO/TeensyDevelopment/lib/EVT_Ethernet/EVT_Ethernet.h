#ifndef EVT_TELEMETRY_H
#define EVT_TELEMETRY_H

#include <Arduino.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <string>

// Global Telemetry objects and variables.
extern EthernetUDP Udp;
extern IPAddress ip;
extern byte mac[];

// Telemetry function prototypes.
void setupTelemetryUDP();
void sendTelemetry();
std::string receiveUdp();


#endif // EVT_TELEMETRY_H
