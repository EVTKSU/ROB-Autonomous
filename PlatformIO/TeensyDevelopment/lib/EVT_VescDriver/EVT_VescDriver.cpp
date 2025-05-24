#include "EVT_VescDriver.h"
#include "EVT_RC.h"
#include "EVT_StateMachine.h"

VescUart vesc1;
VescUart vesc2;
String vescDebug = "";

// Rolling window for smoothing
constexpr int FILTER_WINDOW_SIZE = 5;
int channel1_window[FILTER_WINDOW_SIZE] = {0};
int window_index = 0;
bool window_filled = false;

void setupVesc() {
    Serial1.begin(115200);
    vesc1.setSerialPort(&Serial1);
    
    Serial5.begin(115200);
    vesc2.setSerialPort(&Serial5);
}

void printVescError() {
    vesc1.getVescValues();
    vesc2.getVescValues();
    
    Serial.print("VESC1 error: ");
    Serial.println(vesc1.data.error);
    
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

float getSmoothedChannel1() {
    channel1_window[window_index] = channels[1];
    window_index = (window_index + 1) % FILTER_WINDOW_SIZE;

    if (!window_filled && window_index == 0)
        window_filled = true;

    int count = window_filled ? FILTER_WINDOW_SIZE : window_index;
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += channel1_window[i];
    }
    return float(sum) / count;
}

void updateVescControl() {
    // Get smoothed SBUS throttle channel
    float ch_vesc = getSmoothedChannel1();

    const int neutral = 990;
    const int deadband = 80;
    
    float rpmCommand = 0.0f;

    if (ch_vesc > (neutral + deadband)) {
        float forwardRange = 1700.0f - (neutral + deadband);
        rpmCommand = (ch_vesc - (neutral + deadband)) / forwardRange * 7500.0f;
        rpmCommand = constrain(rpmCommand, 0.0f, 7500.0f);
    } 
    else if (ch_vesc < (neutral - deadband)) {
        float reverseRange = (neutral - deadband) - 350.0f;
        float reverseProportion = (neutral - deadband - ch_vesc) / reverseRange;
        rpmCommand = -constrain(reverseProportion, 0.0f, 1.0f) * 7500.0f;
    } 
    else {
        rpmCommand = 0.0f;
    }

    // Send command
    vesc1.setRPM(rpmCommand);
    // vesc2.setRPM(rpmCommand); // Enable if you want both active

    // Optional debug
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 500) {
        Serial.print("Filtered Throttle: "); Serial.println(ch_vesc);
        Serial.print("rpmCommand: "); Serial.println(rpmCommand);
        lastPrint = millis();
    }
}






// below is the non expiremental code
// ==========================================================================================






// #include "EVT_VescDriver.h"
// #include "EVT_RC.h"
// #include "EVT_StateMachine.h"
// VescUart vesc1;
// VescUart vesc2;
// String vescDebug = "";

// int i = 500;

// void setupVesc() {
//     Serial1.begin(115200);
//     vesc1.setSerialPort(&Serial1);
    
//     Serial5.begin(115200);
//     vesc2.setSerialPort(&Serial5);
// }
// void printVescError() {
//     // Update the VESC values first
//     vesc1.getVescValues();
//     vesc2.getVescValues();
    
//     // Print error information for VESC1.
//     Serial.print("VESC1 error: ");
//     Serial.println(vesc1.data.error);
    
//     // Print error information for VESC2.
//     Serial.print("VESC2 error: ");
//     Serial.println(vesc2.data.error);
// }
// void vescErrorCheck() {
//     vesc1.getVescValues();
//     vesc2.getVescValues();
    
//     if (vesc1.data.error > 0) {
//         SetErrorState(ERR_VESC, String(vesc1.data.error).c_str());
//     }
//     if (vesc2.data.error > 0) {
//         SetErrorState(ERR_VESC, String(vesc2.data.error).c_str());
//     }
// }
// void updateVescControl() {

//     // [1] get vesc values  ========================================================================
//    // vesc1.getVescValues();
//     // [2] check error  ============================================================================
   

//     int ch_vesc = channels[1];
//     const int neutral = 990;
//     const int deadband = 80;
    
//     float rpmCommand = 0.0f;

//     // [3] map channel value -> rpmCommand  ========================================================
    
//     if (ch_vesc > (neutral + deadband)) {
//         float forwardRange = 1700.0f - (neutral + deadband);
//         rpmCommand = ((float)ch_vesc - (neutral + deadband)) / forwardRange * 7500.0f;
//         Serial.println("forwarding!!");
//         if (rpmCommand < 0.0f) rpmCommand = 0.0f;
//         if (rpmCommand > 7500.0f) rpmCommand = 7500.0f;
//     } else if (ch_vesc < (neutral - deadband)) {
//         float reverseRange = (neutral - deadband) - 350.0f;
//         float reverseProportion = ((neutral - deadband) - (float)ch_vesc) / reverseRange;
//         if (reverseProportion < 0.0f) reverseProportion = 0.0f;
//         if (reverseProportion > 1.0f) reverseProportion = 1.0f;
//         rpmCommand = -reverseProportion * 7500.0f;
//             Serial.println("reversing!!");

//     } else {
//         rpmCommand = 0.0f;
        
//     Serial.println("no throttle yay" );
//     Serial.println(channels[1]);
//     }
//     Serial.println("RC Throttle: " + String(channels[1]));
//     Serial.println("rpmCommand: " + String(rpmCommand));
//     // {4] send command to vesc1  ====================================================================
//     vesc1.setRPM(rpmCommand);
//     // [5] send command to vesc2 =====================================================================
//     //vesc2.setRPM(rpmCommand);

//     // demo 1
//     // for (int i = 500; i < 1400; i+= 50) {
//     //     vesc1.setRPM(i);
//     //     Serial.println(i);
//     //     delay(200);
//     // }

//     // demo 2
//     // if (i > 1700) {
//     //     i = 500;
//     // } else {
//     //     i += 50;
//     // }
//     // vesc1.setRPM(i);

    
    
// }
