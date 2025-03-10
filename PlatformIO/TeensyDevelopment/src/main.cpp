#include "TheH-File.h"

void setup() {
    // Initialize each module.
    setupVesc();
    setupOdrv();
    setupSbus();
}

void loop() {
    // Update SBUS channels and then control VESC and ODrive independently.
    if (updateSbusData()) {
        updateVescControl();
        updateOdrvControl();
    }
}
