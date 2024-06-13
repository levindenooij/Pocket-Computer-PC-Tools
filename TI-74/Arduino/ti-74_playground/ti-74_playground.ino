#define INPUT_PIN 6 // same on Arduino UNO
#define OUTPUT_PIN 3 // same on Arduino UNO
#include <PinChangeInterrupt.h>

volatile int pwm_value = 0;
volatile int prev_time = 0;
volatile boolean state = 0;
volatile int byte_bit = 0;
volatile byte reverse_byte = 0;  // incoming byte need to be rotated 01000011 becomes 11000010
volatile byte normal_byte = 0;  // incoming byte need to be rotated 01000011 becomes 11000010

void setup() {
  // put your setup code here, to run once:
Serial.begin(500000);
pinMode(INPUT_PIN, INPUT);
// use pullup resistor
digitalWrite(INPUT_PIN, HIGH);
// attach a PinChange Interrupt to our pin on the rising edge
attachPCINT(digitalPinToPCINT(INPUT_PIN),change,CHANGE); 
}

void loop() {

}

void change()
{
  pwm_value = micros()-prev_time;
  prev_time = micros();
  state = not(digitalRead(INPUT_PIN));
  if (byte_bit==8) {
    Serial.write(reverse_byte);
    byte_bit=0;
    reverse_byte=0;
    normal_byte=0;

  }
  // long high or low pulse is a zero
  if ((pwm_value > 400) && (pwm_value < 750)) {
    byte_bit++;
  }
  else if ((pwm_value > 0) && (pwm_value < 400) && (state == 1)) {
    bitSet(reverse_byte,7-byte_bit);
    bitSet(normal_byte,byte_bit);
    byte_bit++;
  }
}
