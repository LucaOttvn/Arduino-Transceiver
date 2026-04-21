/*
  Based on the SendDemo example from the RC Switch library
  https://github.com/sui77/rc-switch/
*/

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  
  // Transmitter is connected to Arduino Pin #10  
  mySwitch.enableTransmit(10);

  // Optional set pulse length.
  mySwitch.setPulseLength(394);
}

void loop() {
  // Binary code - button 3
  mySwitch.send("00000001101000010000000111111110");
  delay(1000);
}