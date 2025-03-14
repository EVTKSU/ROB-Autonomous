#include "TheH-File.h"



void setup() {
    Serial.begin(9600);
    
    // Initialize control modules.
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

