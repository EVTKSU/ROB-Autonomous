#include "ODriveASCII.h"

// serial communication parameters
#define SERIAL_USB Serial       // Serial (USB), Serial<x> (Hardware serial) e.g. Serial5
#define SERIAL_OD1 Serial1      // hardware serial port for ODrive 1 (1 rear, 2 front)
#define SERIAL_OD2 Serial2      // hardware serial port for ODrive 2 (1 left, 2 right)
#define SERIAL_OD3 Serial7      // hardware serial port for ODrive 3 (1 translate, 2 yaw)
#define N_ODRIVE 3              // number of ODrive motor controllers
#define BAUD_RATE_SERIAL 230400 // baud rate for USB serial
#define BAUD_RATE_ODRIVE 115200 // baud rate for ODrive

#define MAX_DATA_SIZE 128       // max number of bytes for a data array

// ODrive variables
ODriveASCII myODrives[N_ODRIVE] = {ODriveASCII(SERIAL_OD1, BAUD_RATE_ODRIVE, SERIAL_USB),
                              ODriveASCII(SERIAL_OD2, BAUD_RATE_ODRIVE, SERIAL_USB),
                              ODriveASCII(SERIAL_OD3, BAUD_RATE_ODRIVE, SERIAL_USB)};  // array of ODrive objects

// communication variables
char receivedData[MAX_DATA_SIZE] = "";
char sentData[MAX_DATA_SIZE] = "";

//////////////////////////////////////////////////////////////////////////
void setup() {
  SERIAL_USB.begin(BAUD_RATE_SERIAL);
  delay(100);
  snprintf(sentData, sizeof(sentData), "Teensy started.\n\n");
  transmitMsg(sentData);

  initODrives();
  printAndSetParams();

}

void loop() {
  
}

void initODrives() {
  // check motor/encocder calibration and enter closed-loop control
  for (uint8_t i = 0; i < N_ODRIVE; i++) {
    snprintf(sentData, sizeof(sentData), "Initializing ODrive %d...\n", i+1);
    transmitMsg(sentData);
    myODrives[i].ready();
    SERIAL_USB.println("---------------------------------------------\n");
    delay(200);
  }
}

void stopODrives() {
  for (uint8_t i = 0; i < N_ODRIVE; i++) {
    snprintf(sentData, sizeof(sentData), "Stopping ODrive %d...\n", i+1);
    transmitMsg(sentData);
    myODrives[i].stop();
    SERIAL_USB.println("---------------------------------------------\n");
    delay(200);
  }
}

void printAndSetParams() {
  snprintf(sentData, sizeof(sentData), "position 1: %f\n", myODrives[0].getPosition(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "position 2: %f\n", myODrives[0].getPosition(1));
  transmitMsg(sentData);

  snprintf(sentData, sizeof(sentData), "velocity 1: %f\n", myODrives[0].getVelocity(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "velocity 2: %f\n", myODrives[0].getVelocity(1));
  transmitMsg(sentData);

  snprintf(sentData, sizeof(sentData), "torque 1: %f\n", myODrives[0].getTorque(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "torque 2: %f\n", myODrives[0].getTorque(1));
  transmitMsg(sentData);

  snprintf(sentData, sizeof(sentData), "state 1: %d\n", myODrives[0].getCurrentState(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "state 2: %d\n", myODrives[0].getCurrentState(1));
  transmitMsg(sentData);

  snprintf(sentData, sizeof(sentData), "error 1: %lu\n", myODrives[0].getAxisError(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "error 2: %lu\n", myODrives[0].getAxisError(1));
  transmitMsg(sentData);

  myODrives[0].setControlMode(0, 3);
  myODrives[0].setControlMode(1, 3);

  snprintf(sentData, sizeof(sentData), "control mode 1: %d\n", myODrives[0].getControlMode(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "control mode 2: %d\n", myODrives[0].getControlMode(1));
  transmitMsg(sentData);

  myODrives[0].setInputMode(0, 5);
  myODrives[0].setInputMode(1, 5);

  snprintf(sentData, sizeof(sentData), "input mode 1: %d\n", myODrives[0].getInputMode(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "input mode 2: %d\n", myODrives[0].getInputMode(1));
  transmitMsg(sentData);

  myODrives[0].setCurrentLim(0, 40);
  myODrives[0].setCurrentLim(1, 40);

  snprintf(sentData, sizeof(sentData), "current limit 1: %f\n", myODrives[0].getCurrentLim(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "current limit 2: %f\n", myODrives[0].getCurrentLim(1));
  transmitMsg(sentData);

  myODrives[0].setVelLim(0, 90);
  myODrives[0].setVelLim(1, 90);

  snprintf(sentData, sizeof(sentData), "velocity limit 1: %f\n", myODrives[0].getVelLim(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "velocity  limit 2: %f\n", myODrives[0].getVelLim(1));
  transmitMsg(sentData);

  myODrives[0].setPosGain(0, 20);
  myODrives[0].setPosGain(1, 20);

  snprintf(sentData, sizeof(sentData), "position gain 1: %f\n", myODrives[0].getPosGain(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "position gain 2: %f\n", myODrives[0].getPosGain(1));
  transmitMsg(sentData);

  myODrives[0].setVelGain(0, 0.07);
  myODrives[0].setVelGain(1, 0.07);

  snprintf(sentData, sizeof(sentData), "velocity gain 1: %f\n", myODrives[0].getVelGain(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "velocity gain 2: %f\n", myODrives[0].getVelGain(1));
  transmitMsg(sentData);

  myODrives[0].setVelIntGain(0, 0.02);
  myODrives[0].setVelIntGain(1, 0.02);

  snprintf(sentData, sizeof(sentData), "velocity int gain 1: %f\n", myODrives[0].getVelIntGain(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "velocity int gain 2: %f\n", myODrives[0].getVelIntGain(1));
  transmitMsg(sentData);

  myODrives[0].setTrapAccelLim(0, 25);
  myODrives[0].setTrapAccelLim(1, 25);

  snprintf(sentData, sizeof(sentData), "trap accel limit 1: %f\n", myODrives[0].getTrapAccelLim(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "trap accel limit 2: %f\n", myODrives[0].getTrapAccelLim(1));
  transmitMsg(sentData);

  myODrives[0].setTrapDecelLim(0, 25);
  myODrives[0].setTrapDecelLim(1, 25);

  snprintf(sentData, sizeof(sentData), "trap decel limit 1: %f\n", myODrives[0].getTrapDecelLim(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "trap decel limit 2: %f\n", myODrives[0].getTrapDecelLim(1));
  transmitMsg(sentData);

  myODrives[0].setTrapVelLim(0, 69);
  myODrives[0].setTrapVelLim(1, 69);

  snprintf(sentData, sizeof(sentData), "trap vel limit 1: %f\n", myODrives[0].getTrapVelLim(0));
  transmitMsg(sentData);
  snprintf(sentData, sizeof(sentData), "trap vel limit 2: %f\n", myODrives[0].getTrapVelLim(1));
  transmitMsg(sentData);
}

void transmitMsg(char data[]) {
  strcpy(sentData, data);
  SERIAL_USB.print(sentData);
}
