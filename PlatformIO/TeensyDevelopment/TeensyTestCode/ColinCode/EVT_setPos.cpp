#include <ODriveUART.h>
#include <SoftwareSerial.h>

// For Teensy 3/4, we use the hardware serial port (Serial3 in this example)
// (If you’re using another board, adjust accordingly)
HardwareSerial &odrive_serial = Serial3;
int baudrate = 115200; // Must match your ODrive’s UART baudrate

ODriveUART odrive(odrive_serial);

void setup() {
  // Start UART communication with ODrive
  odrive_serial.begin(baudrate);
  // For debugging via USB serial monitor
  Serial.begin(115200);
  delay(10);

  Serial.println("Waiting for ODrive...");
  // Wait until ODrive is discovered (its state is not UNDEFINED)
  while (odrive.getState() == AXIS_STATE_UNDEFINED) {
    delay(100);
  }
  Serial.println("Found ODrive!");
  
  Serial.print("DC voltage: ");
  Serial.println(odrive.getParameterAsFloat("vbus_voltage"));
  
  // Enable closed-loop control
  Serial.println("Enabling closed loop control...");
  while (odrive.getState() != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrive.clearErrors();
    odrive.setState(AXIS_STATE_CLOSED_LOOP_CONTROL);
    Serial.println("Trying again to enable closed loop control...");
    delay(10);
  }
  Serial.println("ODrive running!");

}

// real retard shit
void EVT_setPos(float pos, float vel){
  ODriveFeedback feedback = odrive.getFeedback();
  float velocity;

  // if actual position is lower than current, velocity needs to be negative
  if (feedback.pos > pos){
    velocity = vel*-1;
  }else{
    velocity = vel;
  }
    Serial.print("Setting velocity to: ");
    Serial.println(velocity);
    // Set velocity (second parameter is optional torque feedforward, here 0.0)
    odrive.setVelocity(velocity, 0.0f);

    // wait until pos is reached then set velocity to o
    Serial.print(feedback.pos);
    if (feedback.pos < pos){
      while(feedback.pos < pos){
        Serial.println(feedback.pos);
        delay(10);

        feedback = odrive.getFeedback();
      }
    }else{
      while(feedback.pos > pos){
        Serial.println(feedback.pos);
        delay(10);
        feedback = odrive.getFeedback();
      }
    }
    float threashold = 2;

    //recurse if not in threshold
    delay(60);
    if (feedback.pos > pos + threashold || feedback.pos < pos - threashold){
      if (vel < 1){
        Serial.println("THRESHOLD NOT MET");
        odrive.setVelocity(0, 0.0f);
        Serial.println("DONE :(");
        Serial.println(feedback.pos);
        delay(10);
        return;
      }
      Serial.println("Fuck, gotta correct");
      EVT_setPos(pos,vel/4);
    }

  odrive.setVelocity(0, 0.0f);
  Serial.println("DONE :D");
  Serial.println(feedback.pos);

}

void loop() {
ODriveFeedback feedb = odrive.getFeedback();
Serial.println("trying 1 (pos 50, vel 20)");
EVT_setPos(50,20);
delay(500);
feedb = odrive.getFeedback();
Serial.println(feedb.pos);
delay(5000);

Serial.println("trying 2 (pos 200, vel 10)");
EVT_setPos(200,10);
delay(500);
feedb = odrive.getFeedback();
Serial.println(feedb.pos);
delay(5000);

Serial.println("trying 3 (pos -10, vel 7)");
EVT_setPos(-10,7);
delay(500);
feedb = odrive.getFeedback();
Serial.println(feedb.pos);
delay(5000);

}
