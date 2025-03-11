#ifndef INTEGRATED_CONTROLLER_H
#define INTEGRATED_CONTROLLER_H

#include <Arduino.h>
#include <VescUart.h>
#include <ODriveUART.h>
#include <SBUS.h>
#include <SoftwareSerial.h>

// Include Teensy Ethernet libraries.
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

// Ethernet configuration for Teensy.
extern byte mac[];       // Defined in a single source file.
extern IPAddress ip;     // Defined in a single source file.
#define UDP_REMOTE_IP IPAddress(192, 168, 0, 100)
#define UDP_REMOTE_PORT 8888

// Common definitions.
#define STATUS_LED_PIN 13
static const float MAX_CURRENT = 40.0f;
static const float MAX_STEERING_OFFSET = 1.5f;

// Global SBUS channel array.
extern uint16_t channels[10];

// --- SBUS Prototypes ---
void setupSbus();
bool updateSbusData();

// --- VESC Controller Prototypes ---
void setupVesc();
void updateVescControl();

// --- ODrive Controller Prototypes ---
void setupOdrv();
void updateOdrvControl();

// --- Telemetry Sending Prototype ---
void sendTelemetry();
void setupTelemetryEthernet();
#endif
