// Emulator Sharp CE-126P für PC-1401, PC-1403 u. PC-E500
#include <SoftwareSerial.h>

const int pinBusy = 4; // Pin D4 Busy Sharp PC-126x, pin 4 on 11 pin connector
const int pinDout = 5; // Pin D5, Dout Sharp PC-126x, pin 5 on the 11 pin connector.
const int pinXout = 7; // Pin D7, Xout Sharp PC-126x, pin 7 on the 11 pin connector.
const int pinACK = 9;  // Pin D9, ACK  Sharp PC-126x, pin 9 on the 11 pin connector.
const int InfoLED = 13;

// print setup
boolean Busy; // busy indicator
boolean Xout;
int dataBit; // bit received
int dataByte; // byte received
int i;

void setup() {
// setup pinmode
pinMode(pinBusy, INPUT);
pinMode(pinDout, INPUT);
pinMode(pinXout, INPUT);
pinMode(pinACK, OUTPUT);
pinMode(InfoLED, OUTPUT);

Serial.begin(9600);
Serial.println();
Serial.println("Ready for Print:");
Serial.println();
}

void loop() {

digitalWrite(InfoLED, LOW);

Xout = digitalRead(pinXout);

if (Xout) {
// DeviceSelect:
digitalWrite(InfoLED, HIGH);
delayMicroseconds(50);
i = 0;
do {
digitalWrite(pinACK, HIGH);
do {
Busy = digitalRead(pinBusy);
} while (!Busy);
delayMicroseconds(50);
digitalWrite(pinACK, LOW);
do {
Busy = digitalRead(pinBusy);
} while (Busy);
delayMicroseconds(150);
i++;
} while (i < 8);
do {
Xout = digitalRead(pinXout);
} while (Xout);
digitalWrite(pinACK, HIGH);
delayMicroseconds(500);
digitalWrite(pinACK, LOW);
}

Busy = digitalRead(pinBusy);

if (Busy && !Xout) {
// Daten:
digitalWrite(InfoLED, HIGH);
i = 0;
dataByte = 0;
do {
do {
Busy = digitalRead(pinBusy);
} while (!Busy);
delayMicroseconds(50);
dataBit = digitalRead(pinDout);
digitalWrite(pinACK, HIGH);
do {
Busy = digitalRead(pinBusy);
} while (Busy);
delayMicroseconds(50);
digitalWrite(pinACK, LOW);
dataByte = dataByte | (dataBit << i);
i++;
} while (i < 8);
// Ausgabe:
switch (dataByte) {
case 13:
Serial.println();
break;
case 48:
Serial.print("O"); // Buchstabe "O"
break;
case 240:
Serial.print("0"); // Ziffer Null
break;
default:
if (dataByte > 31 && dataByte < 127) Serial.print(char(dataByte));
}
}

} // End loop
