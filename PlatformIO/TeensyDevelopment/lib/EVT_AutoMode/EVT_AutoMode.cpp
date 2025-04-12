#include <SPI.h>
#include <sstream>
#include "EVT_VescDriver.h"
#include "EVT_StateMachine.h"
#include "EVT_ODriver.h"
#include "EVT_Ethernet.h"

// Global variable for UDP data processing.
// fixed here
float raw_steering_angle = 0.0;
float raw_throttle = 0.0;
bool emergency = false;

// old method for referrence

// void setDataFromUDP(const std::string &udpData) {
//     std::istringstream ss(udpData);
//     std::string token;
//     std::vector<std::string> tokens;
  
//     while (std::getline(ss, token, ',')) {
//       tokens.push_back(token);
//     }
  
//     if (tokens.size() >= 3) {
//       steering_angle = std::atof(tokens[0].c_str());
//       throttle = std::atof(tokens[1].c_str());
//       emergency = (std::atoi(tokens[2].c_str()) != 0);
//     } else {
//       Serial.print("Insufficient data received: ");
//       Serial.println(udpData.c_str());
//     }
//   }

// Function to parse UDP data and update control variables.
void setControls(const std::string &udpData) {
    std::istringstream ss(udpData);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 3) {
        // fixed here
        raw_steering_angle = std::atof(tokens[0].c_str());
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
        float MappedSteering = (raw_throttle); // raw throttle values should be from -2.4 to 2.4, the amount of turns in the steering gearbox.
        vesc1.setRPM(rpmCommand);
        vesc2.setRPM(rpmCommand);

        odrive.setPosition(MappedSteering, 15.0f); // setting the steering angle , with a velocity of 15 fasts.
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
    sendTelemetry();
    std::string rawCommands = receiveUdp();
    if (!rawCommands.empty()) {
        setControls(rawCommands);
    }
    runMappedControls();

    if (emergency){
        SetErrorState("AutoMode","Emergency Flag");
    }
}