#include "EVT_Errorhandler.h"
#include "EVT_VescDriver.h"
#include "EVT_ODriver.h"
void CheckForErrors() {
    checkConnection(); // Check Ethernet connection status.
    // Check for ODrive errors.
    odrvErrorCheck();
    // Check for VESC errors.
    vescErrorCheck();
}