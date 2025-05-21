#include <Arduino.h>
#include "EVT_StateMachine.h"
#include "EVT_Ethernet.h"
#include "EVT_ErrorHandler.h"
#include "EVT_RC.h"
#include "EVT_VescDriver.h"
#include "EVT_AutoMode.h"
#include "EVT_ODriver.h"


void setup() {
  Serial.begin(9600);
  delay(1000); // Wait for Serial Monitor to open

  
  // Initialize modules.
  SetState(RC);
  Serial.println("Initializing modules...");
  setupTelemetryUDP();
  setupSbus();
  setupVesc();
  setupOdrv();
  delay(200);
  updateSbusData();
  digitalWrite(3, HIGH); // Turn on relay 1 (odrive)
  digitalWrite(4, HIGH); // Turn on relay 2 (vesc)
  digitalWrite(5, HIGH); // Turn on relay 3 (contactor)
}

void loop() {


  CheckForErrors();  
  updateSbusData();
  
  switch (GetState())
  {
  case RC:
    if (channels[7] > 1000) {
      SetState(AUTO);
    } else {
      updateVescControl();
      updateOdrvControl();
      
    }
    break;

  case AUTO:
    if (channels[7] < 1000) {
      SetState(RC);
    } else {
      //updateAutonomousMode();
      SetErrorState("Main","Do not be alarmed this is just a test");
    break;

  case ERR:
      digitalWrite(3, LOW); // Turn off relay 1 (odrive)
      digitalWrite(4, LOW); // Turn off relay 2 (vesc)
      digitalWrite(5, LOW); // Turn off relay 3 (contactor)
    // check for reset
    if (channels[4] > 1000){
      // COLIN LOOK HERE!! we need to set this to not be channel 4 since that will cause issues down the line with our encoder.
      //check auto switch
      digitalWrite(3, HIGH); // Turn off relay 1 (odrive)
      digitalWrite(4, HIGH); // Turn off relay 2 (vesc)
      digitalWrite(5, HIGH); // Turn off relay 3 (contactor)
      Serial.println("Attempting to clear errors...");
      if (channels[7] > 1000) {
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
    // Check if the system is idle and not in error state. if idle, it waits for commands.
    if (channels[8] > 400) {
      SetState(RC);
    } else {
      Serial.println("System is idle. Waiting for commands...");
      delay(1000); // Add a delay to avoid flooding the serial output
    }
    break;

  default:
      Serial.println("Warning: Unknown state encountered. Defaulting to IDLE.");
      SetState(IDLE);
      PrintState();
    break;
  }

  }
}
// i put this here in case i need to test something in the future and replace the main file during testing.