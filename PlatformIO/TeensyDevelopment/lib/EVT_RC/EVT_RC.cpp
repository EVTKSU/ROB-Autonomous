// EVT_RC.cpp
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
    // How many bytes are queued before decoding?
    size_t bufBefore = Serial2.available();

    // Try to read one full SBUS frame
    bool ok = sbus.read(channels, &sbusFailSafe, &sbusLostFrame);

    // How many remain afterwards?
    size_t bufAfter = Serial2.available();

    // Debug print: success flag, FIFO depths, and status flags
    Serial.printf(
      "SBUS dbg: ok=%d | before=%u | after=%u | lostFrame=%d | failSafe=%d\n",
      ok, bufBefore, bufAfter, sbusLostFrame, sbusFailSafe
    );

    return ok;
}
