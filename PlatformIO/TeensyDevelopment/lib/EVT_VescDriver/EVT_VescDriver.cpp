#include "EVT_VescDriver.h"
#include "EVT_RC.h"

VescUart vesc1;
VescUart vesc2;
String vescDebug = "";

void setupVesc() {
    Serial1.begin(115200);
    vesc1.setSerialPort(&Serial1);
    
    Serial5.begin(115200);
    vesc2.setSerialPort(&Serial5);
}

void updateVescControl() {
    int ch_vesc = channels[1];
    const int neutral = 990;
    const int deadband = 20;
    
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
    
    vesc1.setRPM(rpmCommand);
    vesc2.setRPM(rpmCommand);
    
    if (vesc1.getVescValues()) {
        vescDebug = "RPM: " + String(vesc1.data.rpm) + "\r\n" +
                    "Input Voltage: " + String(vesc1.data.inpVoltage);
    }
}
