#ifndef EVT_VESCDRIVER_H
#define EVT_VESCDRIVER_H

#include <Arduino.h>
#include <VescUart.h>
#include <SoftwareSerial.h>
#include <map> // Include for std::map

// VESC function prototypes.
void setupVesc();
void vescErrorCheck();
void updateVescControl();
void printVescError();

extern String vescDebug;

// VESC objects declared for external use.
extern VescUart vesc1;
extern VescUart vesc2;

static const std::map<uint32_t, String> vescErrorMap = {
         { 0, "None" },
        {FAULT_CODE_OVER_VOLTAGE,"Overvoltage" },
        {FAULT_CODE_UNDER_VOLTAGE, "Undervoltage" },
        {FAULT_CODE_DRV, "DRV Fault" },
        {FAULT_CODE_ABS_OVER_CURRENT, "Absolute Overcurrent" },
        {FAULT_CODE_OVER_TEMP_FET, "Overtemperature FET" },
        {FAULT_CODE_OVER_TEMP_MOTOR, "Overtemperature Motor" },
        {FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE, "Gate Driver Overvoltage" },
        {FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE, "Gate Driver Undervoltage" },
        {FAULT_CODE_MCU_UNDER_VOLTAGE, "MCU Undervoltage" },
        {FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET, "Booting from Watchdog Reset" },
        {FAULT_CODE_ENCODER_SPI, "Encoder SPI Fault" },
        {FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE, "Encoder SinCos Below Min Amplitude" },
        {FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE, "Encoder SinCos Above Max Amplitude" },
        {FAULT_CODE_FLASH_CORRUPTION, "Flash Corruption" },
        {FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1, "High Offset Current Sensor 1" },
        {FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2, "High Offset Current Sensor 2" },
        {FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3, "High Offset Current Sensor 3" },
        {FAULT_CODE_UNBALANCED_CURRENTS, "Unbalanced Currents" },
        {FAULT_CODE_BRK, "Brake Fault" },
        {FAULT_CODE_RESOLVER_LOT, "Resolver Loss of Tracking" },
        {FAULT_CODE_RESOLVER_DOS, "Resolver Loss of Signal" },
        {FAULT_CODE_RESOLVER_LOS, "Resolver Loss of Signal" },
        {FAULT_CODE_FLASH_CORRUPTION_APP_CFG, "Flash Corruption App Config" },
        {FAULT_CODE_FLASH_CORRUPTION_MC_CFG, "Flash Corruption MC Config" },
        {FAULT_CODE_ENCODER_NO_MAGNET, "Encoder No Magnet" },
        {FAULT_CODE_ENCODER_MAGNET_TOO_STRONG, "Encoder Magnet Too Strong" },
        {FAULT_CODE_PHASE_FILTER, "Phase Filter Fault" },
    } ;
       
#endif // EVT_VESCDRIVER_H
