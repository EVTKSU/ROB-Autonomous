#ifndef ODriveASCII_h
#define ODriveASCII_h

#include "Arduino.h"
#include "ODriveEnums.h"

class ODriveASCII {

    private:
        HardwareSerial & mySerial;
        usb_serial_class & myUSBSerial;
        char sentData[128] = "";

        String readString();

    public:
        ODriveASCII(HardwareSerial & serial, uint32_t baud, usb_serial_class & usbSerial);

        // helper functions
        float readFloat();
        int32_t readInt();

        // system behaviors
        bool ready();     // checks for calibration of both axes and enters closed-loop control
        bool stop();      // enters idle state on both axes
        void reboot();    // reboots the ODrive

        // system read functions
        float getBusVoltage();                  // returns the bus voltage of the ODrive
        float getBusCurrent();                  // returns the bus current of the ODrive

        // motor read functions
        float getPosition(uint8_t axis);        // returns the motor position
        float getVelocity(uint8_t axis);        // returns the motor velocity
        float getTorque(uint8_t axis);          // returns the motor torque
        float getCurrent(uint8_t axis);         // returns the motor current draw

        // motor command functions
        void setPosition(uint8_t axis, float pos) const;            // sends a position command to the ODrive
        void setVelocity(uint8_t axis, float vel) const;            // sends a velocity command to the ODrive
        void setTorque(uint8_t axis, float torque) const;           // sends a torque command to the ODrive

        // state/controller read functions
        uint8_t getCurrentState(uint8_t axis);  // returns the current axis state
        uint32_t getAxisError(uint8_t axis);    // returns the axis error if any
        bool isCalibrated(uint8_t axis);        // returns true if the motor and encoder are calibrated

        uint8_t getControlMode(uint8_t axis);
        uint8_t getInputMode(uint8_t axis);

        float getCurrentLim(uint8_t axis);
        float getVelLim(uint8_t axis);
        float getPosGain(uint8_t axis);
        float getVelGain(uint8_t axis);
        float getVelIntGain(uint8_t axis);

        float getTrapAccelLim(uint8_t axis);
        float getTrapDecelLim(uint8_t axis);
        float getTrapVelLim(uint8_t axis);
        
        // state/controller write functions
        bool runState(uint8_t axis, uint8_t requested_state, bool wait_for_idle, float timeout = 10.0f);
        bool runIdle(uint8_t axis);
        bool runFullCalibration(uint8_t axis);
        bool runClosedLoopControl(uint8_t axis);
        bool runHoming(uint8_t axis, float homingVelocity, float homingOffset);
        void clearErrors();                     // clears errors if any

        void switchToPositionControl(uint8_t axis);
        void switchToVelocityControl(uint8_t axis);
        void switchToTorqueControl(uint8_t axis);

        void setControlMode(uint8_t axis, uint8_t mode);
        void setInputMode(uint8_t axis, uint8_t mode);
        
        void setCurrentLim(uint8_t axis, float current);
        void setVelLim(uint8_t axis, float vel);
        void setPosGain(uint8_t axis, float gain);
        void setVelGain(uint8_t axis, float gain);
        void setVelIntGain(uint8_t axis, float gain);

        void setTrapAccelLim(uint8_t axis, float accel);
        void setTrapDecelLim(uint8_t axis, float decel);
        void setTrapVelLim(uint8_t axis, float vel);

        
};

#endif