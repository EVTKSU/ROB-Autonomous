#include "ODriveASCII.h"

#define BAUD_RATE_ODRIVE 115200

void setup() {
  Serial.begin(9200);
  ODriveASCII od1 = ODriveASCII(Serial1, BAUD_RATE_ODRIVE, Serial);
  ODriveASCII od2 = ODriveASCII(Serial2, BAUD_RATE_ODRIVE, Serial);
  ODriveASCII od3 = ODriveASCII(Serial7, BAUD_RATE_ODRIVE, Serial);

  while(true) {
    Serial.print("1: ");
    Serial.print(od1.getBusVoltage());
    Serial.print('\t');
    Serial.print("2: ");
    Serial.print(od2.getBusVoltage());
    Serial.print('\t');
    Serial.print("3: ");
    Serial.print(od3.getBusVoltage());
    Serial.println();
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
