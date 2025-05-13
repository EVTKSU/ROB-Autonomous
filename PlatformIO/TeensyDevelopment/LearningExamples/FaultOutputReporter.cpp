#include <Arduino.h>
#include "EVT_StateMachine.h"
#include "EVT_Ethernet.h"
#include "EVT_RC.h"
#include "EVT_VescDriver.h"
#include "EVT_ODriver.h"
#include "EVT_AutoMode.h"

void setup() {
  Serial.begin(9600);
  delay(1000); // Wait for Serial Monitor to open

  
  // Initialize modules.
  SetState(INIT);
  Serial.println("Initializing modules...");
  setupTelemetryUDP();
  setupSbus();
  setupVesc();
  setupOdrv();
  SetState(RC);
  delay(200);
}

void loop() {
    printOdriveError();
    delay(500);
    printVescError();
    delay(500);
}