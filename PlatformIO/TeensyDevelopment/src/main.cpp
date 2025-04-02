#include <Arduino.h>
#include "EVT_Ethernet.h"
#include "EVT_RC.h"
#include "EVT_VescDriver.h"
#include "EVT_ODriver.h"
#include "EVT_AutoMode.h"

void setup() {
  Serial.begin(9600);
  delay(1000); // Wait for Serial Monitor to open

  
  // Initialize modules.
  Serial.println("Initializing modules...");
  setupTelemetryUDP();
  setupSbus();
  setupVesc();
  setupOdrv();
}

void loop() {
  updateSbusData();
  // If RC data is available and channel 6 exceeds the threshold, run autonomous mode.
  if (channels[6] > 1000) {
    updateAutonomousMode();
  } else {
    updateVescControl();
    updateOdrvControl();
  }
}
