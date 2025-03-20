#include "TheH-File.h"      // All headers and prototypes are declared here.
#include <SPI.h>            // In case needed by UDP functions
#include "VescController.cpp" 
#include "OdrvController.cpp"
#include "SbusController.cpp"
#include "udp_telemetry.cpp"

void updateAutonomousMode() {
    // Set debug string to indicate autonomous mode.
    odrvDebug = "Autonomous mode active.";
    ODriveFeedback fb = odrive.getFeedback();
    float steeringAngle = fb.pos;               // For example, steering position.

    // Read bus voltage and bus current from the ODrive object (per API):
    float odrvCurrent = odrive.getParameterAsFloat("ibus");
    float odrvVoltage = odrive.getParameterAsFloat("vbus_voltage");
    
    // Also get VESC telemetry data.
    vesc1.getVescValues();    // Update vesc1.data.
    float rpm = vesc1.data.rpm;
    float vescVoltage = vesc1.data.inpVoltage;
    float vescCurrent = vesc1.data.avgInputCurrent + vesc2.data.avgInputCurrent ; // current from both vescs added
           

    // Now call the new sendTelemetry function with both VESC and ODrive values.
    sendTelemetry(rpm, vescVoltage, odrvVoltage, vescCurrent, odrvCurrent, steeringAngle);
     receiveUdp();
}
