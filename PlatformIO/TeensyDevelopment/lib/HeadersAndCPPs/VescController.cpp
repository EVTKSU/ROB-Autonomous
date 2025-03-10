#include "TheH-File.h"

// VESC objects on Serial1 and Serial5.
VescUart vesc1;
VescUart vesc2;

void setupVesc() {
    // Initialize VESC1 on Serial1.
    Serial1.begin(115200); 
    vesc1.setSerialPort(&Serial1);
    
    // Initialize VESC2 on Serial5.
    Serial5.begin(115200);
    vesc2.setSerialPort(&Serial5);
}

/////////////////////////////////////////////////////////////////////////////////////
// VESC Current Control (using SBUS channel 1) WITH REVERSE
// SBUS range: ~350..1700, neutral ~990, deadband ±20.
// If above (neutral+deadband), forward current up to MAX_CURRENT.
// If below (neutral-deadband), reverse current down to -MAX_CURRENT.
// Otherwise, coast at 0 A.
/////////////////////////////////////////////////////////////////////////////////////
void updateVescControl() {
    // VESC current control uses SBUS channel 1.
    int ch_vesc = channels[1];
    const int neutral = 990;  // throttle value at zero position
    const int deadband = 20;  // +/- deadband around neutral
    
    float currentCommand = 0.0f;
    
    if (ch_vesc > (neutral + deadband)) {
        float forwardRange = (1700.0f - (neutral + deadband));
        currentCommand = (float)(ch_vesc - (neutral + deadband)) / forwardRange;
        if (currentCommand < 0.0f) currentCommand = 0.0f;
        if (currentCommand > 1.0f) currentCommand = 1.0f;
        currentCommand *= MAX_CURRENT;
    } else if (ch_vesc < (neutral - deadband)) {
        float reverseRange = (float)((neutral - deadband) - 350);
        float proportion = (float)((neutral - deadband) - ch_vesc) / reverseRange;
        if (proportion < 0.0f) proportion = 0.0f;
        if (proportion > 1.0f) proportion = 1.0f;
        currentCommand = -proportion * MAX_CURRENT;
    } else {
        // Within deadband, coast.
        currentCommand = 0.0f;
    }
    
    // Issue current command to both VESCs.
    vesc1.setCurrent(currentCommand);
    vesc2.setCurrent(currentCommand);
}
