#include "TheH-File.h"

void setup() {
    Serial.begin(9600);

    // Initialize modules.
    setupVesc();
    setupOdrv();
    setupSbus();
    setupTelemetryEthernet();

    // Configure the emergency (E-stop) relay pin.
    pinMode(ESTOP_PIN, OUTPUT);
    digitalWrite(ESTOP_PIN, LOW);
}

void loop() {
    // Update SBUS channels and then control modules.
    if (updateSbusData()) {
        updateVescControl();
        updateOdrvControl();
    }
    
    // Update the state machine (includes emergency check).
    updateStateMachine();
    
    // Other tasks (e.g., telemetry) follow.
}
