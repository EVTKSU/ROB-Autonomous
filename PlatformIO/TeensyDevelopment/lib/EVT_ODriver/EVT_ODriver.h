#ifndef EVT_ODRIVER_H
#define EVT_ODRIVER_H

#include <Arduino.h>
#include <ODriveUART.h>
#include <SoftwareSerial.h>
#include <map> // Include for std::map

#define STATUS_LED_PIN 13

// Global ODrive flag and debug string.
extern bool systemInitialized;
extern String odrvDebug;

// Declare the ODriveUART object so it can be used across modules.
extern ODriveUART odrive;

// ODrive function prototypes.
void setupOdrv();
void odrvErrorCheck() ;
void updateOdrvControl();
void printOdriveError();
void initCalibration();

// Map of error codes to their corresponding strings
static const std::map<uint32_t, String> odrvErrorMap = {
    {ODRIVE_ERROR_INITIALIZING, "Initializing"},
    {ODRIVE_ERROR_SYSTEM_LEVEL, "System Level"},
    {ODRIVE_ERROR_TIMING_ERROR, "Timing Error"},
    {ODRIVE_ERROR_MISSING_ESTIMATE, "Missing Estimate"},
    {ODRIVE_ERROR_BAD_CONFIG, "Bad Config"},
    {ODRIVE_ERROR_DRV_FAULT, "DRV Fault"},
    {ODRIVE_ERROR_MISSING_INPUT, "Missing Input"},
    {ODRIVE_ERROR_DC_BUS_OVER_VOLTAGE, "DC Bus Over Voltage"},
    {ODRIVE_ERROR_DC_BUS_UNDER_VOLTAGE, "DC Bus Under Voltage"},
    {ODRIVE_ERROR_DC_BUS_OVER_CURRENT, "DC Bus Over Current"},
    {ODRIVE_ERROR_DC_BUS_OVER_REGEN_CURRENT, "DC Bus Over Regen Current"},
    {ODRIVE_ERROR_CURRENT_LIMIT_VIOLATION, "Current Limit Violation"},
    {ODRIVE_ERROR_MOTOR_OVER_TEMP, "Motor Over Temp"},
    {ODRIVE_ERROR_INVERTER_OVER_TEMP, "Inverter Over Temp"},
    {ODRIVE_ERROR_VELOCITY_LIMIT_VIOLATION, "Velocity Limit Violation"},
    {ODRIVE_ERROR_POSITION_LIMIT_VIOLATION, "Position Limit Violation"},
    {ODRIVE_ERROR_WATCHDOG_TIMER_EXPIRED, "Watchdog Timer Expired"},
    {ODRIVE_ERROR_ESTOP_REQUESTED, "Estop Requested"},
    {ODRIVE_ERROR_SPINOUT_DETECTED, "Spinout Detected"},
    {ODRIVE_ERROR_BRAKE_RESISTOR_DISARMED, "Brake Resistor Disarmed"},
    {ODRIVE_ERROR_THERMISTOR_DISCONNECTED, "Thermistor Disconnected"},
    {ODRIVE_ERROR_CALIBRATION_ERROR, "Calibration Error"}
    };
#endif // EVT_ODRIVER_H
