#include <Arduino.h>



void setup()
{

pinMode(3, OUTPUT);
pinMode(4, OUTPUT);
pinMode(5, OUTPUT);


}


void loop(){


    
  digitalWrite(3, HIGH); // Turn on relay 1 (odrive)
  digitalWrite(4, HIGH); // Turn on relay 2 (vesc)
  digitalWrite(5, HIGH); // Turn on relay 3 (contactor)
  delay(1000);
  
  digitalWrite(3, LOW); // Turn on relay 1 (odrive)
  digitalWrite(4, LOW); // Turn on relay 2 (vesc)
  digitalWrite(5, LOW); // Turn on relay 3 (contactor)
  delay(1000);
}