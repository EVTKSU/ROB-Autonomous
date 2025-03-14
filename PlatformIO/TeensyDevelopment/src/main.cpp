#include "TheH-File.h"



void setup() {
    Serial.begin(9600);
    
    // Initialize control modules.
    setupVesc();
    setupOdrv();
    setupSbus();
    
    // Initialize Ethernet for telemetry.
    setupTelemetryEthernet();
}

void loop() {
    // Update SBUS channels and then control VESC and ODrive independently.
    if (updateSbusData()) {
        updateVescControl();
        updateOdrvControl();
    }
    
    // Send telemetry every 100 milliseconds.
    static unsigned long lastTelemetryTime = 0;
    if (millis() - lastTelemetryTime >= 100) {
        sendTelemetry();
        lastTelemetryTime = millis();
    }
}
