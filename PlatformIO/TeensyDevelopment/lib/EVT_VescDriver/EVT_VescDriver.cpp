#include "EVT_VescDriver.h"
#include "EVT_RC.h"
#include "EVT_StateMachine.h"
VescUart vesc1;
VescUart vesc2;
String vescDebug = "";

void setupVesc() {
    Serial1.begin(115200);
    vesc1.setSerialPort(&Serial1);
    
    Serial5.begin(115200);
    vesc2.setSerialPort(&Serial5);
}
void printVescError() {
    // Update the VESC values first
    vesc1.getVescValues();
    vesc2.getVescValues();
    
    // Print error information for VESC1.
    Serial.print("VESC1 error: ");
    Serial.println(vesc1.data.error);
    
    // Print error information for VESC2.
    Serial.print("VESC2 error: ");
    Serial.println(vesc2.data.error);
}
void vescErrorCheck() {
    vesc1.getVescValues();
    vesc2.getVescValues();
    
    if (vesc1.data.error > 0) {
        SetErrorState(ERR_VESC, String(vesc1.data.error).c_str());
    }
    if (vesc2.data.error > 0) {
        SetErrorState(ERR_VESC, String(vesc2.data.error).c_str());
    }
}
void updateVescControl() {

    // [1] get vesc values  ========================================================================
   // vesc1.getVescValues();
    // [2] check error  ============================================================================
   

    int ch_vesc = channels[1];
    const int neutral = 990;
    const int deadband = 80;
    
    float rpmCommand = 0.0f;

    // [3] map channel value -> rpmCommand  ========================================================
    
    if (ch_vesc > (neutral + deadband)) {
        float forwardRange = 1700.0f - (neutral + deadband);
        rpmCommand = ((float)ch_vesc - (neutral + deadband)) / forwardRange * 7500.0f;
        Serial.println("forwarding!!");
        if (rpmCommand < 0.0f) rpmCommand = 0.0f;
        if (rpmCommand > 7500.0f) rpmCommand = 7500.0f;
    } else if (ch_vesc < (neutral - deadband)) {
        float reverseRange = (neutral - deadband) - 350.0f;
        float reverseProportion = ((neutral - deadband) - (float)ch_vesc) / reverseRange;
        if (reverseProportion < 0.0f) reverseProportion = 0.0f;
        if (reverseProportion > 1.0f) reverseProportion = 1.0f;
        rpmCommand = -reverseProportion * 7500.0f;
            Serial.println("reversing!!");

    } else {
        rpmCommand = 0.0f;
        
    Serial.println("no throttle yay" );
    Serial.println(channels[1]);
    }

    // {4] send command to vesc1  ====================================================================
    vesc1.setRPM(rpmCommand);
    // [5] send command to vesc2 =====================================================================
    vesc2.setRPM(rpmCommand);
    
    
}
