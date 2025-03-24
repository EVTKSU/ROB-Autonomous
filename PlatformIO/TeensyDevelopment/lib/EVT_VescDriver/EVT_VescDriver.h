#ifndef EVT_VESCDRIVER_H
#define EVT_VESCDRIVER_H

#include <Arduino.h>
#include <VescUart.h>
#include <SoftwareSerial.h>

// VESC function prototypes.
void setupVesc();
void updateVescControl();

extern String vescDebug;

// VESC objects declared for external use.
extern VescUart vesc1;
extern VescUart vesc2;

#endif // EVT_VESCDRIVER_H
