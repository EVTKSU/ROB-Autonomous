#include <Arduino.h>
#include "EVT_StateMachine.h"
#include "EVT_Ethernet.h"
#include "EVT_ErrorHandler.h"
#include "EVT_RC.h"
#include "EVT_VescDriver.h"
#include "EVT_AutoMode.h"
#include "EVT_ODriver.h"


    // NONE,   < No state defined.
    // INIT,   < Initialization state.
    // IDLE, < Idle state.  
    // CALIB,  < Calibration state.
    // RC,     < Remote Control state.
    // AUTO,   < Autonomous state.
    // ERR,     < Error state.

  uint16_t auto_switch;
  uint16_t calibration_switch; 
  uint16_t reset_switch;
  

    unsigned long logLineCounter = 0;  // counts log lines

// Timestamp macro (prints micros())
#define TIMESTAMP()            \
  do {                         \
    Serial.print(micros());    \
    Serial.print(" us | ");    \
  } while (0)

// Log macro: line#, timestamp, message
#define LOG(msg)                                 \
  do {                                           \
    Serial.print(logLineCounter++);             \
    Serial.print(" | ");                        \
    TIMESTAMP();                                 \
    Serial.println(msg);                         \
  } while (0)

  
void setup() {
  
  Serial.begin(9600);
  SetState(NONE);
  delay(1000); // Wait for Serial Monitor to open
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  
  digitalWrite(3, HIGH); // Turn on relay 1 (odrive)
  digitalWrite(4, HIGH); // Turn on relay 2 (vesc)
  digitalWrite(5, HIGH); // Turn on relay 3 (contactor)
  
  // Initialize modules.
  SetState(INIT);
  Serial.println("Initializing modules...");
  setupTelemetryUDP();
  setupSbus();
  setupVesc();
  setupOdrv();
  delay(200);
  updateSbusData();
  //set to idle state after setup
  SetState(IDLE);
}

void loop() {

  LOG("Loop start");
  updateSbusData();
  Serial.printf("calib: %d, auto: %d, reset: %d\n", channels[5], channels[7], channels[4]);

  CheckForErrors();
  LOG("CHECKING FOR ERRORS");
  // Check for errors in the system.
  

  // add switches to corresponding RC channels here
  auto_switch = channels[7];
  calibration_switch = channels[5]; // just for calibration out of idle on starup and starts RC
  reset_switch = channels[4]; // runs odrive calibration and clears errors from err state
  
  switch (GetState())
  {
  case RC:
    updateSbusData();
      LOG("UPDATING SBUS DATA INSIDE RC CASE");
    if (auto_switch > 1000) {
      SetState(AUTO);
    } else {
      updateVescControl();
      updateOdrvControl();
      
    }
    break;

  case AUTO:
    if (auto_switch < 1000) {
      SetState(RC);
    } else {
      //updateAutonomousMode();
      SetErrorState("Main","Do not be alarmed this is just a test");
    }
    break;

  case ERR:
      digitalWrite(3, LOW); // Turn off relay 1 (odrive)
      digitalWrite(4, LOW); // Turn off relay 2 (vesc)
      digitalWrite(5, LOW); // Turn off relay 3 (contactor)
    // check for reset
    if (reset_switch > 1000){

      digitalWrite(3, HIGH); // Turn on relay 1 (odrive)
      digitalWrite(4, HIGH); // Turn on relay 2 (vesc)
      digitalWrite(5, HIGH); // Turn on relay 3 (contactor)
      Serial.println("Attempting to clear errors...");
      if (auto_switch > 1000) {
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
      updateSbusData();
    // Check if the system is idle and not in error state. if idle, it waits for commands.
    if (calibration_switch > 400 && auto_switch < 1000) {
      SetState(RC);
    } else {
      updateSbusData();
      LOG("System is idle. Waiting for commands...");
       if (auto_switch > 1000) {
      Serial.println("[Auto Switch is on ya dingus]");
      }
      
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

// i put this here in case i need to test something in the future and replace the main file during testing.