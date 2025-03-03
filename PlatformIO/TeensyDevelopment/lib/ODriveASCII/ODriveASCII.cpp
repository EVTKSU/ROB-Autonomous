#include "ODriveASCII.h"

template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

// alternate constructor
ODriveASCII::ODriveASCII(HardwareSerial & serial, uint32_t baud, usb_serial_class & usbSerial): mySerial(serial), myUSBSerial(usbSerial) {
    mySerial.begin(baud);   // usb_serial_class.begin() is run once in the main code, not here
}

// helper functions
String ODriveASCII::readString() {
    String str = "";
    static const unsigned long timeout = 10;    // for noisy UART, don't lower this too much
    unsigned long timeout_start = millis();
    char c;
    for (;;) {
        while (!mySerial.available()) {
            if (millis() - timeout_start >= timeout) {
                myUSBSerial.println("timed out\n");
                return str;
            }
        }
        c = mySerial.read();
        if (c == '\n') {
            break;
        }
        str += c;
    }
    return str;
}

float ODriveASCII::readFloat() {
    return readString().toFloat();
}

int32_t ODriveASCII::readInt() {
    return readString().toInt();
}

// system behaviors
bool ODriveASCII::ready() {              // checks for calibration of both axes and enters closed-loop control
    return ODriveASCII::isCalibrated(0) & ODriveASCII::isCalibrated(1);
    // if (ODriveASCII::isCalibrated(0) & ODriveASCII::isCalibrated(1)) {
    //     return ODriveASCII::runClosedLoopControl(0) & ODriveASCII::runClosedLoopControl(1);
    // } else {
    //     return false;
    // }
}

bool ODriveASCII::stop() {               // enters idle state on both axes
    return ODriveASCII::runIdle(0) & ODriveASCII::runIdle(1);
}

void ODriveASCII::reboot() {             // reboots the ODrive
    mySerial << "sr\n";
}

// system read functions
float ODriveASCII::getBusVoltage() {     // returns the bus voltage of the ODrive
    mySerial << "r vbus_voltage\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getBusCurrent() {     // returns the bus current of the ODrive
    mySerial << "r ibus\n";
    return ODriveASCII::readFloat();
}

// motor read functions
float ODriveASCII::getPosition(uint8_t axis) {       // returns the motor position
    mySerial << "r axis" << axis << ".encoder.pos_estimate\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getVelocity(uint8_t axis) {       // returns the motor velocity
    mySerial << "r axis" << axis << ".encoder.vel_estimate\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getTorque(uint8_t axis) {         // returns the motor torque
    mySerial << "r axis" << axis << ".motor.current_control.Iq_measured\n";
    float current = ODriveASCII::readFloat();

    mySerial << "r axis" << axis << ".motor.config.torque_constant\n";
    float torque_constant = ODriveASCII::readFloat();

    return current*torque_constant;
}

float ODriveASCII::getCurrent(uint8_t axis) {        // returns the motor current draw
    mySerial << "r axis" << axis << ".motor.current_control.Iq_measured\n";
    return ODriveASCII::readFloat();
}

// motor command functions
void ODriveASCII::setPosition(uint8_t axis, float pos) const {       // sends a position command to the ODrive
    mySerial << "q " << axis << " " << pos << "\n";
}

void ODriveASCII::setVelocity(uint8_t axis, float vel) const {       // sends a velocity command to the ODrive
    mySerial << "v " << axis << " " << vel << "\n";
}

void ODriveASCII::setTorque(uint8_t axis, float torque) const {      // sends a torque command to the ODrive
    mySerial << "c " << axis << " " << torque << "\n";
}

// state/controller read functions
uint8_t ODriveASCII::getCurrentState(uint8_t axis) { // returns the current axis state
    mySerial << "r axis" << axis << ".current_state\n";
    return ODriveASCII::readInt();
}

uint32_t ODriveASCII::getAxisError(uint8_t axis) { // returns the axis error if any
    mySerial << "r axis" << axis << ".error\n";
    return ODriveASCII::readInt();
}

bool ODriveASCII::isCalibrated(uint8_t axis) { // returns true if the motor and encoder are calibrated
    snprintf(sentData, sizeof(sentData), "Checking axis %d calibration...\n", axis+1);
    myUSBSerial.print(sentData);
    mySerial << "r axis" << axis << ".motor.is_calibrated\n";
    int motorCalibration = ODriveASCII::readInt();

    if (motorCalibration) {
        snprintf(sentData, sizeof(sentData), "Motor is calibrated.\n");
        myUSBSerial.print(sentData);
    } else {
        snprintf(sentData, sizeof(sentData), "Motor is not calibrated.\n");
        myUSBSerial.print(sentData);
    }

    mySerial << "r axis" << axis << ".encoder.is_ready\n";
    int encoderCalibration = ODriveASCII::readInt();

    if (encoderCalibration) {
        snprintf(sentData, sizeof(sentData), "Encoder is calibrated.\n\n");
        myUSBSerial.print(sentData);
    } else {
        snprintf(sentData, sizeof(sentData), "Encoder is not calibrated.\n\n");
        myUSBSerial.print(sentData);
    }
    return motorCalibration && encoderCalibration;
}

uint8_t ODriveASCII::getControlMode(uint8_t axis) {
    mySerial << "r axis" << axis << ".controller.config.control_mode\n";
    return ODriveASCII::readInt();
}

uint8_t ODriveASCII::getInputMode(uint8_t axis) {
    mySerial << "r axis" << axis << ".controller.config.input_mode\n";
    return ODriveASCII::readInt();
}

float ODriveASCII::getCurrentLim(uint8_t axis) {
    mySerial << "r axis" << axis << ".motor.config.current_lim\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getVelLim(uint8_t axis) {
    mySerial << "r axis" << axis << ".controller.config.vel_limit\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getPosGain(uint8_t axis) {
    mySerial << "r axis" << axis << ".controller.config.pos_gain\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getVelGain(uint8_t axis) {
    mySerial << "r axis" << axis << ".controller.config.vel_gain\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getVelIntGain(uint8_t axis) {
    mySerial << "r axis" << axis << ".controller.config.vel_integrator_gain\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getTrapAccelLim(uint8_t axis) {
    mySerial << "r axis" << axis << ".trap_traj.config.accel_limit\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getTrapDecelLim(uint8_t axis) {
    mySerial << "r axis" << axis << ".trap_traj.config.decel_limit\n";
    return ODriveASCII::readFloat();
}

float ODriveASCII::getTrapVelLim(uint8_t axis) {
    mySerial << "r axis" << axis << ".trap_traj.config.vel_limit\n";
    return ODriveASCII::readFloat();
}

// state/controller write functions
bool ODriveASCII::runState(uint8_t axis, uint8_t requested_state, bool wait_for_idle, float timeout) {
    int timeout_ctr = (int)(timeout * 10.0f);
    mySerial << "w axis" << axis << ".requested_state " << requested_state << '\n';
    if (wait_for_idle) {
        do {
            delay(100);
            mySerial << "r axis" << axis << ".current_state\n";
        } while (readInt() != AXIS_STATE_IDLE && --timeout_ctr > 0);
    }

    return timeout_ctr > 0;
}

bool ODriveASCII::runIdle(uint8_t axis) {
    bool success =  ODriveASCII::runState(axis, 1, true, 10.0f);

    if (success) {
        snprintf(sentData, sizeof(sentData), "Axis %d in idle state.\n", axis+1);
        myUSBSerial.print(sentData);
    } else {
        snprintf(sentData, sizeof(sentData), "Axis %d failed to enter idle state.\n", axis+1);
        myUSBSerial.print(sentData);
    }
    return success;
}

bool ODriveASCII::runFullCalibration(uint8_t axis) {
    bool success = ODriveASCII::runState(axis, 3, true, 10.0f);
    
    if (success) {
        snprintf(sentData, sizeof(sentData), "Axis %d calibrated successfully.\n", axis+1);
        myUSBSerial.print(sentData);

        mySerial << "w axis" << axis << ".motor.config.pre_calibrated " << 1 << '\n';
        mySerial << "w axis" << axis << ".encoder.config.pre_calibrated " << 1 << '\n';

    } else {
        snprintf(sentData, sizeof(sentData), "Axis %d failed to calibrate.\n", axis+1);
        myUSBSerial.print(sentData);
    }
    return success;
}

bool ODriveASCII::runClosedLoopControl(uint8_t axis) {
    bool success = ODriveASCII::runState(axis, 8, false);
    
    if (success) {
        snprintf(sentData, sizeof(sentData), "Axis %d in closed-loop control.\n", axis+1);
        myUSBSerial.print(sentData);
    } else {
        snprintf(sentData, sizeof(sentData), "Axis %d failed to enter closed-loop control.\n", axis+1);
        myUSBSerial.print(sentData);
    }
    return success;
}

bool ODriveASCII::runHoming(uint8_t axis, float homingVelocity, float homingOffset) {
    // return ODriveASCII::runState(axis, 11, true, 30.0f);      // ODrive's homing feature requires limit sensors
    snprintf(sentData, sizeof(sentData), "Homing axis %d...\n", axis+1);
    myUSBSerial.print(sentData);

    ODriveASCII::switchToVelocityControl(axis);  // switch to velocity control mode for homing

    if (ODriveASCII::getCurrentState(axis) != 8) {
        ODriveASCII::runClosedLoopControl(axis);
    }

    if (ODriveASCII::getCurrentState(axis) == 8) {
        ODriveASCII::setVelocity(axis, homingVelocity);      // move slowly towards the joint limit
        snprintf(sentData, sizeof(sentData), "Moving to joint limit...\n\n");
        myUSBSerial.print(sentData);
        delay(300);                                     // don't check endstop condition when the motor starts
        while (true) {
            if (abs(ODriveASCII::getVelocity(axis)) < 0.05) { // if the motor has slowed due to the joint limit
                myUSBSerial.print("At joint limit.\n");
                ODriveASCII::setVelocity(axis, 0);           // stop the motor
                ODriveASCII::switchToPositionControl(axis);

                myUSBSerial.print("Moving to homing offset...\n\n");
                snprintf(sentData, sizeof(sentData), "Currently at position: %f\n", ODriveASCII::getPosition(axis));
                myUSBSerial.print(sentData);
                float newPos = ODriveASCII::getPosition(axis) + homingOffset;
                snprintf(sentData, sizeof(sentData), "Moving to: %f\n", newPos);
                myUSBSerial.print(sentData);

                ODriveASCII::setPosition(axis, newPos);    // move to the homing offset position
                delay(200);
                while (abs(ODriveASCII::getPosition(axis) - newPos) > 0.15) {    // wait for the motor to move to the offset
                    delay(10);
                }
                ODriveASCII::runIdle(axis);  // enter idle mode after homing

                snprintf(sentData, sizeof(sentData), "Axis %d successfully homed.\n\n", axis+1);
                myUSBSerial.print(sentData);
                return true;
            }
            delay(10);
        }
    } else{
        return false;
    }
    


}

void ODriveASCII::clearErrors() {                     // clears any ODrive errors
    mySerial << "w axis" << 0 << "error " << 0 << '\n';
    mySerial << "w axis" << 1 << "error " << 0 << '\n';
}

void ODriveASCII::switchToPositionControl(uint8_t axis) {
    ODriveASCII::setControlMode(axis, 3);
    ODriveASCII::setInputMode(axis, 5);
    snprintf(sentData, sizeof(sentData), "Control mode set to position control.\nInput mode set to trapezoidal trajectory.\n\n");
    myUSBSerial.print(sentData);
}

void ODriveASCII::switchToVelocityControl(uint8_t axis) {
    ODriveASCII::setControlMode(axis, 2);
    ODriveASCII::setInputMode(axis, 1);
    snprintf(sentData, sizeof(sentData), "Control mode set to velocity control.\nInput mode set to passthrough.\n\n");
    myUSBSerial.print(sentData);
}

void ODriveASCII::switchToTorqueControl(uint8_t axis) {
    ODriveASCII::setControlMode(axis, 1);
    ODriveASCII::setInputMode(axis, 1);
    snprintf(sentData, sizeof(sentData), "Control mode set to torque control.\nInput mode set to passthrough.\n\n");
    myUSBSerial.print(sentData);
}

void ODriveASCII::setControlMode(uint8_t axis, uint8_t mode) {
    mySerial << "w axis" << axis << ".controller.config.control_mode " << mode << '\n';
}

void ODriveASCII::setInputMode(uint8_t axis, uint8_t mode) {
    mySerial << "w axis" << axis << ".controller.config.input_mode " << mode << '\n';
}

void ODriveASCII::setCurrentLim(uint8_t axis, float current) {
    mySerial << "w axis" << axis << ".motor.config.current_lim " << current << '\n';
}

void ODriveASCII::setVelLim(uint8_t axis, float vel) {
    mySerial << "w axis" << axis << ".controller.config.vel_limit " << vel << '\n';
}

void ODriveASCII::setPosGain(uint8_t axis, float gain) {
    mySerial << "w axis" << axis << ".controller.config.pos_gain " << gain << '\n';
}

void ODriveASCII::setVelGain(uint8_t axis, float gain) {
    mySerial << "w axis" << axis << ".controller.config.vel_gain " << gain << '\n';
}

void ODriveASCII::setVelIntGain(uint8_t axis, float gain) {
    mySerial << "w axis" << axis << ".controller.config.vel_integrator_gain " << gain << '\n';
}

void ODriveASCII::setTrapAccelLim(uint8_t axis, float accel) {
    mySerial << "w axis" << axis << ".trap_traj.config.accel_limit " << accel << '\n';
}

void ODriveASCII::setTrapDecelLim(uint8_t axis, float decel) {
    mySerial << "w axis" << axis << ".trap_traj.config.decel_limit " << decel << '\n';
}

void ODriveASCII::setTrapVelLim(uint8_t axis, float vel) {
    mySerial << "w axis" << axis << ".trap_traj.config.vel_limit " << vel << '\n';
}