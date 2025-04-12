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
  updateSbusData();
 
}

void loop() {

    
  updateSbusData();

  switch (GetState())
  {
  case RC:
    if (channels[6] > 1000) {
      SetState(AUTO);
    } else {
      updateVescControl();
      updateOdrvControl();
    }
    break;

  case AUTO:
    if (channels[6] < 1000) {
      SetState(RC);
    } else {
      //updateAutonomousMode();
      SetErrorState("Main","Do not be alarmed this is just a test");
    break;

  case ERR:

    // check for reset
    if (channels[4] > 1000){
      // COLIN LOOK HERE!! we need to set this to not be channel 4 since that will cause issues down the line with our encoder.
      //check auto switch
      if (channels[6] > 1000) {
        Serial.println("TURN OFF AUTO SWITCH BEFORE ATTEMPTING TO CLEAR ERRORS");
      }else{
        Serial.println();
        Serial.println("yay! Errors cleared :D");
        SetState(IDLE);
        odrive.setState(AXIS_STATE_UNDEFINED);
      }
    }
  break;
  
  case IDLE:
    // Check if the system is idle and not in error state
    if (channels[8] > 400) {
      SetState(RC);
    } else {
      Serial.println("System is idle. Waiting for commands...");
      delay(250); // Add a delay to avoid flooding the serial output
    }
    break;

  default:
      Serial.println("AUTO and RC disabled due to state:");
      PrintState();
    break;
  }

  }
}
// i put this here in case i need to test something in the future and replace the main file during testing.