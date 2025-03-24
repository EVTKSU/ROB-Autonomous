#include <Arduino.h>
#include "EVT_Telemetry.h"
#include "EVT_RC.h"
#include "EVT_VescDriver.h"
#include "EVT_ODriver.h"
#include "EVT_AutoMode.h"

void setup() {
  Serial.begin(9600);
  
  // Initialize modules.
  setupTelemetryUDP();
  setupSbus();
  setupVesc();
  setupOdrv();
}

void loop() {
  // If RC data is available and channel 6 exceeds the threshold, run autonomous mode.
  if (updateSbusData() && channels[6] > 1000) {
    updateAutonomousMode();
  } else {
    updateVescControl();
    updateOdrvControl();
  }
}
