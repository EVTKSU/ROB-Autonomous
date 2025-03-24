#ifndef EVT_ODRIVER_H
#define EVT_ODRIVER_H

#include <Arduino.h>
#include <ODriveUART.h>
#include <SoftwareSerial.h>

#define STATUS_LED_PIN 13

// Global ODrive flag and debug string.
extern bool systemInitialized;
extern String odrvDebug;

// Declare the ODriveUART object so it can be used across modules.
extern ODriveUART odrive;

// ODrive function prototypes.
void setupOdrv();
void updateOdrvControl();

#endif // EVT_ODRIVER_H
