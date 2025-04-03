#ifndef EVT_STATEMACHINE_H
#define EVT_STATEMACHINE_H

#include <Arduino.h>

/**
 * @brief Enum representing the different states of the system.
 */
enum STATE {
    NONE,   ///< No state defined.
    INIT,   ///< Initialization state.
    CALIB,  ///< Calibration state.
    RC,     ///< Remote Control state.
    AUTO,   ///< Autonomous state.
    ERR     ///< Error state.
};

/**
 * @brief The current state of the system.
 *
 * This global variable holds the current state and can be accessed by modules that
 * include this header.
 */
extern STATE CurrentState;

/**
 * @brief Retrieves the current state.
 * 
 * @return The current system state.
 */
STATE GetState();

/**
 * @brief Sets the system state.
 * 
 * This function updates the global state and prints a message via Serial.
 * If the new state is ERR, it prints an error message.
 * 
 * @param newState The new state to set.
 */
void SetState(STATE newState);

/**
 * @brief Sets the error state and prints detailed error information.
 * 
 * @param location A C-style string indicating where the error occurred.
 * @param reason A C-style string explaining the reason for the error.
 */
void SetErrorState(const char* location, const char* reason);

/**
 * @brief Converts a STATE enum value to a C-style string.
 *
 * This helper function is used to print the state as a string instead of an integer.
 *
 * @param s The state enum value.
 * @return A C-style string representing the state.
 */
const char* StateToString(STATE s);
void PrintState();

#endif // EVT_STATEMACHINE_H
