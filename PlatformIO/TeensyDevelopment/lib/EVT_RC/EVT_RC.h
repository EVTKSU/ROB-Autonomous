#ifndef EVT_RC_H
#define EVT_RC_H

#include <Arduino.h>
#include <SBUS.h>

// Global SBUS channel array.
extern uint16_t channels[10];

// SBUS function prototypes.
void setupSbus();
bool updateSbusData();

#endif // EVT_RC_H
