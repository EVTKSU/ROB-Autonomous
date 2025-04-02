#ifndef EVT_AUTOMODE_H
#define EVT_AUTOMODE_H

#include <Arduino.h>

// Autonomous mode function prototype.
void updateAutonomousMode();
void setControls(const std::string &udpData);
void runMappedControls();

#endif // EVT_AUTOMODE_H
