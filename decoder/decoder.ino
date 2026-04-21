/*
   -Arduino 433MHz RF Sniffer
   -Based on an example from the RCSwitch Library
   -https://github.com/sui77/rc-switch/
   -Hardware: Arduino Nano & Generic 433MHz RF Receiver
   -T.K.Hareendran/2018
*/


#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  mySwitch.enableReceive(0);  // Receiver input on interrupt 0 (D2)
  pinMode(13, OUTPUT);        // D13 as output- Optional
  // Button
  pinMode(3, INPUT);

  // Transmitter is connected to Arduino Pin #10
  mySwitch.enableTransmit(4);

  // Optional set pulse length.
  mySwitch.setPulseLength(394);
}

void loop() {

  if (mySwitch.available()) {
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(), mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
  }

  // On button click
  if (digitalRead(3) == HIGH) {
    mySwitch.send("00000001101000010000000111111110");
  }
}
