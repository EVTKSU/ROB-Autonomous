#include "EVT_StateMachine.h"

// Define the global state variable.
STATE CurrentState = NONE;

STATE GetState() {
    return CurrentState;
}

void SetState(STATE newState) {
    CurrentState = newState;
    if (newState == ERR) {
        Serial.println("=========================== ERROR OCCURRED ===========================");
        Serial.println("=========================== ERROR OCCURRED ===========================");
        Serial.println("=========================== ERROR OCCURRED ===========================");
    } else {
        Serial.println("======================================================================");
        Serial.print("SETTING STATE: ");
        Serial.println(StateToString(CurrentState));  // Print the state as a string.
        Serial.println("======================================================================");
    }
}

void SetErrorState(const char* location, const char* reason) {
    CurrentState = ERR;
    Serial.println("=========================== ERROR OCCURRED ===========================");
    Serial.println("=========================== ERROR OCCURRED ===========================");
    Serial.println("=========================== ERROR OCCURRED ===========================");
    Serial.println("======================================================================");
    Serial.print("Reason: ");
    Serial.println(reason);
    Serial.print("Location: ");
    Serial.println(location);
    Serial.println("======================================================================");
    Serial.println("=========================== ERROR OCCURRED ===========================");
    Serial.println("=========================== ERROR OCCURRED ===========================");
    Serial.println("=========================== ERROR OCCURRED ===========================");
}

void PrintState(){
    Serial.println(StateToString(CurrentState)); 
}

const char* StateToString(STATE s) {
    switch(s) {
        case NONE:   return "NONE";
        case INIT:   return "INIT";
        case CALIB:  return "CALIB";
        case RC:     return "RC";
        case AUTO:   return "AUTO";
        case ERR:    return "ERR";
        default:     return "UNKNOWN";
    }
}
