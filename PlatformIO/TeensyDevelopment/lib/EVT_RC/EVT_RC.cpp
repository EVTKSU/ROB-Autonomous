#include "EVT_RC.h"

// Create SBUS instance on Serial2.
SBUS sbus(Serial2);
uint16_t channels[10] = {0};
static bool sbusFailSafe = false;
static bool sbusLostFrame = false;

void setupSbus() {
    Serial2.begin(100000, SERIAL_8E2);
    sbus.begin();
    delay(500);
}

bool updateSbusData() {
    return sbus.read(channels, &sbusFailSafe, &sbusLostFrame);
}
