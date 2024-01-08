#include <SoftwareSerial.h>



// Program Name : PC120x_CE126_Emulator
/*    
  Purpose   : Program to convert and load basic source programs into SHARP PC140x 
              and print data from LLIST of LPRINT command
  Prereqs   : PC126x pocket computer, board with schematic implemented, Arduino Mega, 
              serial to USB converter connected to board
  Comments  :
   	          Initial program and schematics by Cavefisher : 
  	          http://www.cavefischer.at/spc/html/PC-1401Key_Arduino.html
  	          I isolated the load part of the original program as it didn't work for me.
  						Corrected the errors, improved it and added the whole source file processing
  						and encoding to tokens to it
  						See for a description of the tapeformat and header and data type 
  						processing here :
  						https://edgar-pue.tripod.com/sharp/files/bigpc/sharplink.html 
Limitations :
              Filename is not supported yet.
              Arduino memory size.Array size limtied to 1000. 
  						Limited testing done.
Author      :	Levin de Nooij
Date        : October 26th 2023
*/


#define pinLed A15 // Led on the breadboard to signal in-operation
#define pinButton A13 // Button breadboard to start the load process

// breadboard : Pin A15 -> xOx pushbutton >- LED statuslight > 330 OHM resistor > PIN A13


const int pinBUSY = 4; // Pin D4 Busy Sharp PC-126x, pin 4 on 11 pin connector.
const int pinDout = 5; // Pin D5, Dout Sharp PC-126x, pin 5 on the 11 pin connector.
const int pinACK = 9; // Pin D9, ACK  Sharp PC-126x, pin 9 on the 11 pin connector.
const int pinSEL2 = 10; // Pin D10, ACK  Sharp PC-126x, pin 10 on the 11 pin connector.
const int pinSEL1 = 11; // Pin D11, ACK  Sharp PC-126x, pin 10 on the 11 pin connector.
const int InfoLED = 13;

// print setup
boolean Busy; // busy indicator
boolean SEL1; 
boolean SEL2;
int dataBit; // bit received
int dataByte; // byte received
long timeOut;
int i; //index


void setup() {
  // setup pinmode
pinMode(pinBUSY,INPUT);
pinMode(pinDout, INPUT);
pinMode(pinACK, OUTPUT);
pinMode(pinSEL2,INPUT);
pinMode(pinSEL1,INPUT);

Serial.begin(9600);
Serial.println();
Serial.println("Ready for Print:");
Serial.println();

}


void loop() {
  
digitalWrite(InfoLED, LOW);

SEL1 = digitalRead(pinSEL1);
SEL2 = digitalRead(pinSEL2);

if (SEL2 && SEL1) {
// DeviceSelect:
delayMicroseconds(50);
i = 0;
do {
SEL1 = digitalRead(pinSEL1);
} while (SEL1);
timeOut = millis();
do {
Busy = digitalRead(pinBUSY);
if (millis() - timeOut > 50) break;
} while (!Busy);
delayMicroseconds(50);
digitalWrite(pinACK, HIGH);
timeOut = millis();
do {
SEL1 = digitalRead(pinSEL1);
if (millis() - timeOut > 50) break;
} while (!SEL1);
delayMicroseconds(150);
digitalWrite(pinACK, LOW);
do {
SEL1 = digitalRead(pinSEL1);
} while (SEL1);
delayMicroseconds(150);
}

Busy = digitalRead(pinBUSY);

if (Busy) {
// Daten:
digitalWrite(InfoLED, HIGH);
i = 0;
dataByte = 0;
do {
do {
Busy = digitalRead(pinBUSY);
} while (!Busy);
delayMicroseconds(500);
dataBit = digitalRead(pinDout);
digitalWrite(pinACK, HIGH);
do {
Busy = digitalRead(pinBUSY);
} while (Busy);
delayMicroseconds(480);
digitalWrite(pinACK, LOW);
dataByte = dataByte | (dataBit << i);
i++;
} while (i < 8);
// Ausgabe:
switch (dataByte) {
case 13:
Serial.println();
break;
default:
if (dataByte > 31 && dataByte < 127) Serial.print(char(dataByte));
}
}
} // End Loop
