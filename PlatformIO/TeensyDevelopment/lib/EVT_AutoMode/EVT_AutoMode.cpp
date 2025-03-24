#include <SPI.h>
#include "EVT_VescDriver.h"
#include "EVT_ODriver.h"
#include "EVT_Telemetry.h"



void updateAutonomousMode() {
    // Set autonomous mode debug message.
    odrvDebug = "Autonomous mode active.";
    
    // Retrieve ODrive feedback via the extern 'odrive'.
    ODriveFeedback fb = odrive.getFeedback();
    float steeringAngle = fb.pos;
    
    // Get ODrive parameters.
    float odrvCurrent = odrive.getParameterAsFloat("ibus");
    float odrvVoltage = odrive.getParameterAsFloat("vbus_voltage");
    
    // Update VESC telemetry.
    vesc1.getVescValues();
    float rpm = vesc1.data.rpm;
    float vescVoltage = vesc1.data.inpVoltage;
    float vescCurrent = vesc1.data.avgInputCurrent + vesc2.data.avgInputCurrent;
    
    // Send telemetry packet and check for incoming UDP commands.
    sendTelemetry(rpm, vescVoltage, odrvVoltage, vescCurrent, odrvCurrent, steeringAngle);
    receiveUdp();
   ///void setAutoControl(){
// //shit goes here
// if (receiveUdp) {
//     // Map throttle percentage to VESC RPM command.
//     // 0 maps to 0 RPM and 100 maps to 7500 RPM.
//     float rpmCommand = (throttle / 100.0f) * 7500.0f;
//     vesc1.setRPM(rpmCommand);
//     vesc2.setRPM(rpmCommand);

//     // Map steering value to ODrive position command.
//     // Here, steeringZeroOffset is the calibrated center and maxSteeringOffset is the maximum deviation.
//     float maxSteeringOffset = 2.4f; // Maximum offset from RC mapping.
//     float steeringCommand = steeringZeroOffset + ((steering_angle / 50.0f) * maxSteeringOffset);
//     odrive.setPosition(steeringCommand, 27.0f);
// } else {
//     // In case of an emergency, stop throttle and center the steering.
//     vesc1.setRPM(0);
//     vesc2.setRPM(0);
//     odrive.setPosition(steeringZeroOffset, 27.0f);
// }
// // --- Autonomous Control Mapping End --
    
}
