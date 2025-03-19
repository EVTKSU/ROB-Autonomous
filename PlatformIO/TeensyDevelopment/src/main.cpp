#include "TheH-File.h"
#include <SPI.h>            // In case needed by UDP functions

EVTstate currentState = Idle; // state setup for natural state machine :(

// Define debug strings so that both update functions can write to them.
String vescDebug = "";
String odrvDebug = "";

void setup() {
    Serial.begin(9600);
    
    // functions called to setup the controls for vescs, the odrive, the UDP setup process, and input from the SBUS receiver
    setupVesc();
    setupOdrv();
    setupSbus();
    setupTelemetryUDP();

}

void loop() {
    if (updateSbusData()) {
        if (channels[6] > 1000) {
            updateAutonomousMode();
            odrvDebug = "Autonomous mode active.";
        } else {
            updateVescControl();
            updateOdrvControl();
        }
                 
     

    }

    }
