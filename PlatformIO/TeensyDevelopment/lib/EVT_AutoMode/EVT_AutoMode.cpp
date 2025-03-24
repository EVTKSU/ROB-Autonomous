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
}
