#include <SoftwareSerial.h>



// Program Name : PC1401_CLOAD_Arduino_Megav2
/*    
  Purpose   : Program to convert and load basic source programs into SHARP PC140x
  Prereqs   : PC1401 pocket computer, board with schematic implemented, Arduino Mega, 
              serial to USB converter connected to board
  Comments  :
   	          Initial program and schematics by Cavefisher : 
  	          http://www.cavefischer.at/spc/html/PC-1401Key_Arduino.html
  	          I isolated the load part of the original program as it didn't work for me.
  						Corrected the errors, improved it and added the whole source file processing
  						and encoding to binary to it
  						See for a description of the tapeformat and header and data type 
  						processing here :
  						https://edgar-pue.tripod.com/sharp/files/bigpc/sharplink.html 
Limitations :
              Filename is not supported yet.
              Arduino memory size. 
  						Limited testing done.
Author      :	Levin de Nooij
Date        : Aug 16 th 2023
*/

// Board setup 
// on the PC1401 only 
// pin 1 MTout2 for buzzer suppression, 
// pin 6 for data input
// pin 3 for GND 
// are used

#define statusLed A15 // Led on the breadboard to signal in-operation
#define startButton A13 // Button breadboard to start the load process

// breadboard : Pin A15 -> xOx pushbutton >- LED statuslight > 330 OHM resistor > PIN A13

const int pinXin =  6; // Pin D6 Sharp PC-1401 , pin 6 on the 11 pin connector
const int pinMTout2 =  2; // Pin D2 Sharp PC-1401, pin 1 on the 11 pin connector


// serial inputstream setup
char inputChar; // input character from serial
char sourceSegment; // what kind of source is it, row, source, text, data?
String segmentString; // string read before a control character is encountere
char sourceChar; // individual character in the source string
int outputDecimal; // decimal value of the command,verb, function or character


const int refSize=89; // size of the reference table for commands, verbs and functions
const String refKey[refSize] = 
  {"AREAD","AND","ABS","ATN","ASN","ACS","AHS","AHC","AHT", 
"BEEP","CONT","CLEAR","CLOAD","CSAVE","COS","CHR$", "CUR","CALL",
"DIM","DEGREE","DEG","DMS","DECI","DATA","END","EXP","FOR","FACT",
"GOTO","GOSUB","GRAD","HEX","HSN","HCS","HTN","INPUT","IF","INT",
"INKEY$","LIST","LLIST","LPRINT","LOG","LN","LET","LEN","LEFT$","MEM","MID$",
"NEXT","NOT","NEW","ON","OR","PRINT","PASS","PI","POL","PEEK","POKE","PAUSE",
"RUN","RETURN","READ","RESTORE","RND","RANDOM","RIGHT$","RADIAN","REM","REC","ROT","RCP",
"STOP","SQR","SIN","SGN","STR$","STEP","SQU","TAN","TRON","TROFF","TO","TEN","THEN","USING","VAL","WAIT"
};

const int refValue[refSize] = 
{225,161,153,159,157,158,141,142,143, 
196,178,201,183,182,150,168,137,204,
203,193,155,156,132,220,216,147,213,144,
198,224,195,133,138,139,140,223,212,152,
173,180,181,226,146,145,214,166,171,175,170,
217,163,177,211,162,222,179,174,130,167,205,221,
176,227,219,228,160,192,172,194,215,129,131,135,
218,148,149,154,169,209,136,151,199,200,208,134,210,202,165,197
};

int refIndex; // index of  both arrays

// program data array setup
const int programSize=1000; // for most programs this can be enough 
byte programData[programSize]; // array for tokenized program
int codeIndex; // index of a byte stored in program data
int rowStart; // where does a row start in the byte array


// basic file name encoding setup
const byte fileType = 112; // 0x70 is basic program 
const int nameLength = 7; // // size of filename in bytes stored 1 + 7 + 1 + 1
const byte fileData[nameLength] = {0,0,0,0,0,0,0}; // file name is empty
const byte nameEOF = 95; // 0x5F after filename

// program constant
const byte programEOF = 255; // 0xFF two times after program

// checksum encoding setup
byte lowNibble; // lower 4 bits
byte highNibble; // higher 4 bits
byte fileChecksum; // calculated checksum for filename
const int numBlocks = (programSize/120) + 1;
byte blockChecksum[numBlocks]; // array for storing every checksum after 120 bytes
int thisBlock; // the block nummer of the current block
int mod120; // have we reached a new block
int div120; // value for thisBlock

// setup pulse generation
const int pulse8 = 125;  // μs, 8 x pulse8 = HIGH 
const int pulse4 = 250;  // μs, 4 x pulse4 = LOW 


void setup() {
  // setup pinmode
  pinMode(pinXin,OUTPUT);
  pinMode(pinMTout2,OUTPUT);
  pinMode(statusLed,OUTPUT); // status led
  pinMode(startButton,INPUT_PULLUP); //startbutton
  
 // start serial and fileserial communication
  Serial.begin(9600); // serial monitor for program notifications
  Serial1.begin(9600); // Serial input for source file
 
// initialize a few variables
  segmentString='\0';
  sourceSegment='R'; // S is default, R is row, D is data, T is text
  outputDecimal = 0;
  codeIndex=0;
  rowStart=0;
  
// have the user start the program.
  Serial.println();
  Serial.println(F("When ready, send basic source file via serial input"));


}


void loop() { 
    processInputFile();

   if (digitalRead(startButton) == LOW) {
    digitalWrite(statusLed, HIGH);
    delay(500);

    calcFileChecksum();
    calcProgramChecksum();
    digitalWrite(pinMTout2, HIGH); // mute the buzzer
    loadProgram();
    digitalWrite(pinMTout2, LOW);// // unmute the buzzer

    // End of sequence, return to initial state.
    digitalWrite(statusLed, LOW);
    Serial.println(F("That's all folks!"));
    Serial.println();

}  
} 

// ************* functions related to inputfile processing ***************

  
void processInputFile() {
// Processing the inputfile...
  if (Serial1.available() > 0 ) {
    inputChar = Serial1.read();
    switch (inputChar) {
      case 10 : // end of file?
        processInputString();
        programData[codeIndex]=13;
        programData[rowStart+2]=codeIndex - rowStart-2; // add the # bytes to the second position

        Serial.println(F("Done, this is the generated code with comma's for readability"));
        // print the array
        for (int x=0;x <= codeIndex; x++) {
            Serial.print(programData[x]);
            addComma();
        }
        Serial.println();
        Serial.print(F("Number of bytes generated is : ")); Serial.print(codeIndex);
        Serial.println();
        Serial.println (F("Now enter CLOAD, press ENTER on the PC140x, press the start button on the board"));
        // initialize a few variables
        segmentString='\0';
        sourceSegment='R'; // S is default, R is row, D is data, T is text
        outputDecimal = 0;
        rowStart=0;
        break;
      case 13 : // end of row
        processInputString();
        programData[codeIndex]=13; // add the carriage return
        programData[rowStart+2]=codeIndex-rowStart-2; // add the # bytes to the second position
        rowStart = codeIndex+1; // the next bytes are a new row.
        sourceSegment = 'R';
        break;
      case 32 : // space encountered
        if (sourceSegment == 'T' ) {
          segmentString.concat(inputChar);  // space is part of the text or code, so just add it
        } else
        {processInputString();
        }
        break;
      case 34 : // doube quote encountered
        if (sourceSegment ==  'S')  {
          sourceSegment = 'T';}  // put into text mode at first double quote
          else {
            if (sourceSegment ==  'T')  sourceSegment = 'S'; // put into source mode at 2nd double quote
         }
        segmentString.concat(inputChar);
        break;
      case 40 : // open bracket encountered , could have a verb before
        if (sourceSegment == 'T') {
          segmentString.concat(inputChar);  // colon is part of the text, so just add it.
        } else
        {
        processInputString(); // process before open bracket
        segmentString.concat(inputChar);
        processInputString(); // process the open bracket
        }
        break;
      case 58 : // colon encountered, text or next statement due
        if (sourceSegment == 'T') {
          segmentString.concat(inputChar);  // colon is part of the text, so just add it.
        } else
        {
        if (sourceSegment =='R') { // omit colon when rowsegment
        processInputString(); // process row before the colon
        } else 
        {
        processInputString(); // process before the colon
        segmentString.concat(inputChar);
        processInputString(); // process the colon
        }
        }
        break;
       case 59 : // semicolon encountered
        if (sourceSegment ==  'T')  {
          segmentString.concat(inputChar);
        } else
        { 
          processInputString(); // process before the semicolon
          segmentString.concat(inputChar);
          processInputString(); // process the semicolon
        }
        break;
       case 42 : // * encountered
        if (sourceSegment ==  'T')  {
          segmentString.concat(inputChar);
        } else
        { 
          processInputString(); // process before *
          segmentString.concat(inputChar);
          processInputString(); // process *
        }
        break;
       case 43 : // + encountered
        if (sourceSegment ==  'T')  {
          segmentString.concat(inputChar);
        } else
        { 
          processInputString(); // process before +
          segmentString.concat(inputChar);
          processInputString(); // process the +
        }
        break;
       case 45 : // - encountered
        if (sourceSegment ==  'T')  {
          segmentString.concat(inputChar);
        } else
        { 
          processInputString(); // process before the -
          segmentString.concat(inputChar);
          processInputString(); // process the -
        }
        break;
       case 47 : // / encountered
        if (sourceSegment ==  'T')  {
          segmentString.concat(inputChar);
        } else
        { 
          processInputString(); // process before the /
          segmentString.concat(inputChar);
          processInputString(); // process the /
        }
        break;
      case 61 : // = encountered
        if (sourceSegment ==  'T')  {
          segmentString.concat(inputChar);
        } else
        { 
          processInputString(); // process before the =
          segmentString.concat(inputChar);
          processInputString(); // process the =
        }
        break;
      default :
        segmentString.concat(inputChar);
        break;
    }
  }
}


void processInputString() {
    
  if (sourceSegment=='R') {
    processRowSegment();
    sourceSegment = 'S';
  }
  else {
    if (segmentString.length() > 1) {
      processComVerbFunc();
    }
      if (outputDecimal != 0) { // found the command, verb or function
      programData[codeIndex]=outputDecimal;
      if (segmentString == 'REM') {sourceSegment = 'T';}
      codeIndex += 1; // move to the next slot in programData
      }
      else { // must be some basic ref
      processCharSegment();
      sourceSegment = 'S';     
      }
  }
  sourceSegment = 'S'; 
  segmentString ='\0';
  outputDecimal=0;
  sourceChar='0';
}

void processRowSegment() {
  codeIndex=rowStart; // each row starts with a 0 // leave next slot open for count of number of bytes in the row.
  outputDecimal=segmentString.toInt(); // should be numeric;
  programData[codeIndex]=outputDecimal/256; // calculate high nibble rownumber
  codeIndex+=1; // next slot is row number
  programData[codeIndex]=outputDecimal%256; // calculate low nibble rownumber
  codeIndex +=2; //move to the next slot, skip the count slot
  sourceSegment ==  'S';
}    

void processComVerbFunc() { // process if command, verb or function
      refIndex=0;
        while ((outputDecimal == 0) && (refIndex < refSize)) { // process the ref ref array
    
        if (segmentString==refKey[refIndex]) {
          outputDecimal=refValue[refIndex];
          }
        refIndex+=1;
        }
}
  
  
void processCharSegment()  {
  for (int i=0 ; i < segmentString.length();i++) {
  sourceChar = segmentString[i];
  outputDecimal=(int)sourceChar;
  programData[codeIndex]=outputDecimal;
  codeIndex += 1;
  }
}


void addComma() {
   Serial.print(',');
}

// ************* functions related to load process ***************
          
  // Checksum calculation for filename
void calcFileChecksum() {
  Serial.println();
  Serial.println(F("Filename Checksum processing started..."));
  fileChecksum=20; // We always need to add 0x5F = high5 and low 15
  
  for (int x = 0; x < 7; x++) {
    lowNibble=fileData[x]&B00001111;
    highNibble=fileData[x]>>4;
    if(fileChecksum + highNibble > 255) fileChecksum+= 1;
    fileChecksum+=highNibble;
    fileChecksum+=lowNibble;
  }
  
}  
// End Checksum calculation for filename
  
  
  // Checksum calculation
void calcProgramChecksum() {
  Serial.println();
  Serial.println(F("Program Checksum processing started...")); 
  thisBlock=0;
  blockChecksum[thisBlock]=0;
  for (int x = 0; x <= codeIndex; x++) { // codeIndex instead of ProgramSize
    div120 = x/120;
    mod120 = x%120;
    if ((div120 >= 1) && (mod120 == 0))  {
      thisBlock=div120;
      blockChecksum[thisBlock]=0;
    }
    lowNibble=programData[x]&B00001111;
    highNibble=programData[x]>>4;
    if(blockChecksum[thisBlock] + highNibble > 255) blockChecksum[thisBlock]+= 1;
    blockChecksum[thisBlock]+=highNibble;
    blockChecksum[thisBlock]+=lowNibble;
  }
    // Add EOF 55 to checksum.
    lowNibble=programEOF&B00001111;
    highNibble=programEOF>>4;
    if(blockChecksum[thisBlock] + highNibble > 255) blockChecksum[thisBlock]+= 1;
    blockChecksum[thisBlock]+=highNibble;
    blockChecksum[thisBlock]+=lowNibble;
}

// End Checksum calculation

// Send Filedata  and Programdata to PC140x
void loadProgram() {  
  Serial.println();
  Serial.println(F("Load Process started... "));
  
  // Initalizing the sequence with a bunch of high bits:
  Serial.println();
  Serial.println(F("Initalizing the load sequence ... "));
  for (int x = 0; x < 1200; x++) sendBit(1); // 1200 High Bits 

  // Send 1 D-Type Header for File Type to PC140x (Basic program is 0x70 112d)
    sendBit(0);
    for (int y = 4; y < 8; y++) {
      sendBit(bitRead(fileType, y));
    }
    sendBit(1);
    sendBit(0);
    for (int y = 0; y < 4 ; y++) {
      sendBit(bitRead(fileType, y));
    }
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);
    
  //Send 7  H-Type Headers for FileName Bytes to PC140x
  for (int x = 6; x >= 0 ; x--) {
    sendBit(0);
    for (int y = 0; y < 4 ; y++) {
      sendBit(bitRead(fileData[x], y));
    }
    sendBit(1);
    sendBit(0);
    for (int y = 4; y < 8; y++) {
      sendBit(bitRead(fileData[x], y));
    }
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);
  }
  //Send 1 D-Type for end of filename to PC140x (0x5F=95d)
    sendBit(0);
    for (int y = 4; y < 8; y++) {
      sendBit(bitRead(nameEOF, y));
    }
    sendBit(1);
    sendBit(0);
    for (int y = 0; y < 4 ; y++) {
      sendBit(bitRead(nameEOF, y));
    }
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);

  //Send 1 D-Type for checksum of filename to PC140x
    sendBit(0);
    for (int y = 4; y < 8 ; y++) {
      sendBit(bitRead(fileChecksum, y));
    }
    sendBit(1);
    sendBit(0);
    for (int y = 0; y < 4; y++) {
      sendBit(bitRead(fileChecksum, y));
    }
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);
    sendBit(1);

  // Send programdata to PC140x:
  // Send D-Types for Programbyte to PC140x
  thisBlock=0;
  for (int x = 0; x <= codeIndex; x++) { // codeIndex instead of programSize
    div120 = x/120;
    mod120 = x%120;
    if ((div120 >= 1) && (mod120 == 0))  {
      writeProgramChecksum (); // write intermediate checksum for 120 bytes
      thisBlock=div120; // start a new block
    }
    
    sendBit(0);
    for (int y = 4; y < 8 ; y++) {
      sendBit(bitRead(programData[x], y)); // send highNibble in reverse
    }
    sendBit(1);
    sendBit(0);
    for (int y = 0; y < 4; y++) {
      sendBit(bitRead(programData[x], y)); // send low Nibbler in reverse
    }
    sendBit(1);
    sendBit(1);
  }
  
  // Send 2 D-Types for End of program to PC140x
  for (int i = 0; i < 2; i++) {
    sendBit(0);
    for (int y = 4; y < 8 ; y++) {
      sendBit(bitRead(programEOF, y));
    }
    sendBit(1);
    sendBit(0);
    for (int y = 0; y < 4; y++) {
      sendBit(bitRead(programEOF, y));
    }
    sendBit(1);
    sendBit(1);
  }

    // Send last D-type checksum to PC
    writeProgramChecksum ();
    
    sendBit(1); // send final stopbits
    sendBit(1);
} 


void writeProgramChecksum () { // Send D type program checksum per block
    sendBit(0);
    for (int y = 4; y < 8; y++) {
      sendBit(bitRead(blockChecksum[thisBlock], y));
    }
    sendBit(1);
    sendBit(0);
    for (int y = 0; y < 4; y++) {
      sendBit(bitRead(blockChecksum[thisBlock], y));
    }
    sendBit(1);
    sendBit(1);
}

void sendBit(int b) {
  if (b == 1) { // Bit = 1
    for (int z = 0; z < 8; z++) pulseOut(pulse8); // 8 short Pulses = HIGH
  }
  if (b == 0) { // Bit = 0
    for (int z = 0; z < 4; z++) pulseOut(pulse4); // 4 long Pulses = LOW
  }
}

void pulseOut(int d) {
  digitalWrite(pinXin, HIGH);
  delayMicroseconds(d);
  digitalWrite(pinXin, LOW);
  delayMicroseconds(d);
}        

  
  
