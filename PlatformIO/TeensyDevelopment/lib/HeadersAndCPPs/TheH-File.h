#ifndef INTEGRATED_CONTROLLER_H
#define INTEGRATED_CONTROLLER_H

#include <Arduino.h>
#include <VescUart.h>
#include <ODriveUART.h>
#include <SBUS.h>
#include <SoftwareSerial.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

enum EVTstate {Idle, Auto, RC, Oops};

// Common definitions.
#define STATUS_LED_PIN 13
static const float MAX_CURRENT = 40.0f;
static const float MAX_STEERING_OFFSET = 2.4f;

// Global SBUS channel array.
extern uint16_t channels[10];

// Global EthernetUDP object for receiving UDP packets.
extern EthernetUDP Udp;  // Used for raw UDP reception.

// Global ODrive system initialization flag.
extern bool systemInitialized;

// Global debug strings for aggregated output.
extern String vescDebug;
extern String odrvDebug;

extern EVTstate currentState;
// --- SBUS Prototypes ---
void setupSbus();
bool updateSbusData();

// --- VESC Controller Prototypes ---
void setupVesc();
void updateVescControl();

// --- ODrive Controller Prototypes ---
void setupOdrv();
void updateOdrvControl();

// --- UDP Telemetry Prototypes ---
// These functions are implemented in UDP_Telemetry.cpp.
void setupTelemetryUDP();
// --- Autonomous Mode Prototype ---
// Autonomous mode logic is implemented in AutonomousMode.cpp.
void updateAutonomousMode();

#endif