
#include <Arduino.h>
#include <ODriveUART.h>
#include <SoftwareSerial.h>

#define STATUS_LED_PIN 13 // sets the onboard teensy 4.1 LED pin to 13, which is the default pin for the onboard LED.


// Declare the ODriveUART object so it can be used across modules.
extern ODriveUART odrive;

// these are functions being setup for the odrive control code, where setupodrv is the setup function
// and updateodrvcontrol is the function that will be called in the main loop to update the ODrive control.
void setupOdrv();
void updateOdrvControl();

// The Teensy 4.1 has 8 hardware serial ports, and Serial6 is used for ODrive communication.
// The ODrive is connected to Serial6, which is the sixth hardware serial port on the Teensy 4.1.
// HardwareSerial allows us to initializze the serial port for ODrive communication.

// Use Serial6 for ODrive communication.
HardwareSerial &odrive_serial = Serial6; 
ODriveUART odrive(odrive_serial);

float currentSpeed = 0.0;

void setup() {
    Serial.begin(115200); // Initialize Serial monitor for debugging.
    delay(1000); // Wait for Serial Monitor to open

    // Initialize Serial6 for ODrive communication.
    odrive_serial.begin(115200);

    Serial.println("ODrive Minimal Example: Setup complete.");


    odrive_serial.begin(115200);
        Serial.println("Established ODrive communication");
        delay(500);
        Serial.println("Waiting for ODrive...");
        unsigned long startTimeOD = millis();
        while (odrive.getState() == AXIS_STATE_UNDEFINED) { //waits for the odrive to respond
            //undefined is the default state when not responding
    Serial.println("waiting on odrive's state...");
            delay(1500);
     }

     Serial.println("Found and calibrating the ODrive!");
     odrive.setState(AXIS_STATE_MOTOR_CALIBRATION); // odrive.Setstate allows you to set the state
     // states can be, but are not limited to:
     //     AXIS_STATE_IDLE
     //     AXIS_STATE_CLOSED_LOOP_CONTROL (this one is the one you use once done calibrating)
     //     AXIS_STATE_ENCODER_OFFSET_CALIBRATION
     //     AXIS_STATE_MOTOR_CALIBRATION
     delay(4000);
     odrive.clearErrors();

     Serial.println("Calibrating steering (encoder offset calibration)...");
     odrive.setState(AXIS_STATE_ENCODER_OFFSET_CALIBRATION);
     delay(4000);
     odrive.clearErrors();
        }
    
// So far, we have setup communication on the hardware side, 
// and setup the odrive to be easily called
// we also calibrated the motor and encoder, which is important for the odrive to know where the motor is at all times.
// now we can actually set the motor to a position, or velocity, or current, etc.

void loop() {
    
    //now we need to set the ODrive to closed loop control, which is the state that allows us to set the motor to a position or velocity.
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL); // this is the state that allows us to set the motor to a position or velocity.
    // this is the state that it will be in for the rest of the program.

    // Increase speed gradually and reset after reaching 100.
    currentSpeed += 1.0;
    if (currentSpeed > 100.0) {
        currentSpeed = 0.0;
    }

    // Send a velocity command to ODrive on axis 0.
    // Replace this with the correct API if different.
    odrive.setPosition(currentSpeed, 27.0f); // Set position with velocity feed-forward.
    // alternatively, you can use the setVelocity function to set the velocity directly.
    //  odrive.setVelocity(currentSpeed, 10.0f);
    Serial.print("Current speed set: ");
    Serial.println(currentSpeed);

    delay(100);
}