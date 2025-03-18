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
// VESC RPM Control (using SBUS channel 1) WITH REVERSE
// SBUS range: ~350..1700, neutral ~990, deadband ±20.
// For values above (neutral + deadband), map linearly to 0 - 7500 RPM (forward).
// For values below (neutral - deadband), map linearly to 0 to -7500 RPM (reverse).
// Within deadband, command 0 RPM.
/////////////////////////////////////////////////////////////////////////////////////
void updateVescControl() {
    // VESC RPM control uses SBUS channel 1.
    int ch_vesc = channels[1];
    const int neutral = 990;  // throttle value at zero position
    const int deadband = 20;  // +/- deadband around neutral
    
    float rpmCommand = 0.0f;
    
    if (ch_vesc > (neutral + deadband)) {
        float forwardRange = 1700.0f - (neutral + deadband);
        rpmCommand = ((float)ch_vesc - (neutral + deadband)) / forwardRange * 7500.0f;
        if (rpmCommand < 0.0f) rpmCommand = 0.0f;
        if (rpmCommand > 7500.0f) rpmCommand = 7500.0f;
    } else if (ch_vesc < (neutral - deadband)) {
        float reverseRange = (neutral - deadband) - 350.0f;
        float reverseProportion = ((neutral - deadband) - (float)ch_vesc) / reverseRange;
        if (reverseProportion < 0.0f) reverseProportion = 0.0f;
        if (reverseProportion > 1.0f) reverseProportion = 1.0f;
        rpmCommand = -reverseProportion * 7500.0f;
    } else {
        rpmCommand = 0.0f;
    }
    
    // Issue RPM command to both VESCs.
    vesc1.setRPM(rpmCommand);
    vesc2.setRPM(rpmCommand);
    
    // Instead of rolling prints, update the debug string if new values are available.
    if (vesc1.getVescValues()) {
        vescDebug = "RPM: " + String(vesc1.data.rpm) + "\r\n" +
                    "Input Voltage: " + String(vesc1.data.inpVoltage);
    }
}
