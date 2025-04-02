#include <SPI.h>
#include <sstream>
#include "EVT_VescDriver.h"
#include "EVT_ODriver.h"
#include "EVT_Ethernet.h"

// Global variable for UDP data processing.
float raw_throttle = 0.0;
float raw_steering = 0.0;
bool emergency = false;

// Function to parse UDP data and update control variables.
// Note: The UDP steering value is ignored because we rely on ODrive's feedback.
void setControls(const std::string &udpData) {
    std::istringstream ss(udpData);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 3) {
        // Skip the UDP steering value (tokens[0]) and use only throttle and emergency.
        raw_throttle = std::atof(tokens[1].c_str());
        emergency = (std::atoi(tokens[2].c_str()) != 0);
    } else {
        Serial.print("Insufficient data received to set controls: ");
        Serial.println(udpData.c_str());
    }
}

// Runs the mapped control commands using the RC center steering value captured from ODrive feedback.
void runMappedControls() {
    // Capture the RC center steering value upon entering autonomous mode.
    static bool centerCaptured = false;
    static float autoCenterSteering = 0.0f;

    
    if (!centerCaptured) {
        // Capture the center from ODrive's current reported steering position.
        ODriveFeedback fb = odrive.getFeedback();
        autoCenterSteering = fb.pos;
        centerCaptured = true;
        Serial.print("Captured autonomous center steering value from ODrive: ");
        Serial.println(autoCenterSteering);
    }

    if (!emergency) {
        // Map throttle percentage (0-100) to VESC RPM command (0-7500 RPM).
        float rpmCommand = (raw_throttle / 100.0f) * 7500.0f;
        vesc1.setRPM(rpmCommand);
        vesc2.setRPM(rpmCommand);

        // map the steering value to ODrive position command 
        float targetPosition = autoCenterSteering + raw_steering;
    } else {
        // In an emergency, stop throttle and hold the steering at the captured center.
        vesc1.setRPM(0);
        vesc2.setRPM(0);
        odrive.setPosition(autoCenterSteering, 27.0f);
    }
}

void updateAutonomousMode() {
    // Set autonomous mode debug message.
    odrvDebug = "Autonomous mode active.";

    // Retrieve current ODrive feedback.
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
    std::string rawCommands = receiveUdp();
    if (!rawCommands.empty()) {
        setControls(rawCommands);
    }
    runMappedControls();
}