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
#define ESTOP_PIN 7            // Digital pin to drive the relay for ebrake
#define ESTOP_THRESHOLD 1000   // SBUS channel 6 threshold for deadman switch activation
#define STATUS_LED_PIN 13
static const float MAX_CURRENT = 40.0f;
static const float MAX_STEERING_OFFSET = 1.5f;

typedef enum {
    STATE_IDLE,
    STATE_RC,
    STATE_AUTONOMOUS,
    STATE_EMERGENCY
} SystemState;


extern uint16_t channels[10];
extern SystemState currentState;
extern byte mac[];
extern IPAddress ip;


// --- State Machine ---
void updateStateMachine();

// --- SBUS  ---
void setupSbus();
bool updateSbusData();

// --- VESC Controller  ---
void setupVesc();
void updateVescControl();

// --- ODrive Controller  ---
void setupOdrv();
void updateOdrvControl();

// --- Telemetry Sending  ---
void sendTelemetry();
void setupTelemetryEthernet();
#endif



