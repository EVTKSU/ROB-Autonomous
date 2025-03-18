#include "TheH-File.h"

// Define debug strings so that both update functions can write to them.
String vescDebug = "";
String odrvDebug = "";

void setup() {
    Serial.begin(9600);
    
    // Initialize control modules.
    setupVesc();
    setupOdrv();
    setupSbus();
}

void loop() {
    if (updateSbusData()) {
         updateVescControl();
         updateOdrvControl();
         
         // Clear the screen (ANSI escape sequence) and move the cursor to top-left.
         Serial.write("\033[2J");  // Clear screen
         Serial.write("\033[H");   // Move cursor to home position
         
         // Print one solid block of debug info.
         Serial.print(vescDebug + "\r\n" + odrvDebug + "\r\n");
    }
}
