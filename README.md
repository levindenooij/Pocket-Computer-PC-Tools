This repository contains tools and programs for interfacing vintage SHARP pocketcomputers to your PC as an alternative to what is already offered by Peil & Partners https://www.peil-partner.de/ifhe.de/sharp/ in combination with a vintage CE-126P printer cassette interface.
Also for CASIO some interfacing options are available.

** SHARP Interfacing to a PC ** 

The 80's SHARP and CASIO Pocketcomputers  used FSK Modulation for saving and loading basic programs to a cassette recorder. When you have the proper interface peripherals this is still a good way to archive your programs. Instead of a cassette recorder you can also use a pc with a mediaplayer like Audacity as a replacement. With software on the PC like Pocket Tools for SHARP Pocket Computers you can convert basic txt files into wav files for loading into to the SHARP pocket computer and vice versa. Troublesome is still getting the right audio levels and sampling rates to get it to work.

** SHARP 11-PIN HW replacement for CE-126P **

I have added two schematics as an alternative for a real CE-126P one with passive and one with active components. Just link with ear and mic cables to a USB audio interface to your computer and use e.g Audacity in combination with Pocket Tools for SHARP Pocket Computers. Audiolevels are much better than the real CE-126P.

**SHARP 11-PIN Arduino as Emulator for the CE-26P cassette interface **

Alternatively with modern micro controllers you can simplify the process and convert a basic file into machine readable tokens and simulate the tapeformat converted into pulse. This github directory contains a couple of Arduino INO files for your use. Next to an ARDUINO MEGA (due to memory requirements) you will need some additional hardware like a breadboard, some components as well as a connector to the 11 pin socket on the pocket PC.

For cassette interface breadboard wiring guidance please have a look at: http://www.cavefischer.at/spc/html/PC-1401Key_Arduino.html. Don't use the source code mentioned there as it is not fit for purpose!
For this cassette interface you only need the upper part of the breadboard with push button, led and resistor.

** SHARP 11-PIN Arduino as Emulator for the CE-26P printer interface **

I have also added a SHARP CE-126P printer emulator that sends LLIST or LPRINT commands to a serial interface for for further processing e.g a serial printer. For the printer emulator breadboard wiring guidance please have look here: http://www.cavefischer.at/spc/html/CE-126P_Emulator.html
I build a permanent board with a Arduino Nano, pull down resistors and a 11 pin-connector. Be aware different source listings for 14xx vs 12xx types. As there a limited memory requirements a NANO is sufficient.

** SHARP 11 Pin & 15 pin SIO Interface **

Some machines like PC-1350, E-500, G850, etc,have a SIO interface on either the 11 pin or 15 pin socket. Much more convenient!
I have included a GUI programs (Java or Python) for managing the transfer between a serial FTDI USB interface and the machine. I have built a permanent board with a cheap FTDI clone and a inverter chip Sn74ls04n (the original FTDI can  be programmed to invert, the cheap ones not). Both for 11 as well 15 pin (different pin configuration)
Please have a look here for connector guidance : http://www.silicium.org/forum/viewtopic.php?t=42285

** CASIO 30 Pin connector for serial interface (FX850P, FX880P, VX4, ..) **

Some CASIO machines with a 30 pin connector have TTL level serial communication built in. 
Pin 9 RX, P14 TX, Pin30 GND (count top row, 1,3,5,7,9 and bottom row 2,4,6,8,10,14,...30
I have included a CASIOtransfer program in Python. Easy to use with CASIO commands like SAVE "COM0:4,N,8,1" or LOAD "COM0,4,N.8.1". For more info on  serial connections, see the respective manuals.

** CASIO FX 702P, FX-700/PB-100, PB-700 and the like **
There are multiple cassette interfaces available 
FA-1 : FX-502P, FX-602P
FA-2 : FX 602P, FX702p
FA-3 : FX 702P/PB-100
FA-4 : PB-700

I own the FA-2 and FA-3 but was missing an interface for the PB-700. After some search on the internet I came accross the following website from Piotr Piatek, http://www.pisi.com.pl/piotr433/index.htm. A great source for knowledge on CASIO machines, data representation, basic program structures as well as alternative self built interfaces. As I was missing an interface for the PB-700 I replicated the circuit http://www.pisi.com.pl/piotr433/pb700tae.htm. It uses an old micro controller and has connections for RS-232 and I2C. A warning is in place the whole exercise is not for the faint-hearted 
- you need to program an old controller
- somewhat more complicated signal path than the SHARP one.
- the fact that the source code is all in assembler
- the wiring and soldering on a board could become messy and confusing
- quite a few points of failure possible in the process

Nevertheless I sourced the components and built the circuit and with some further guidance from Piotr I got it to work. The big tip was to omit RS-232 overall and link a terminal program (9600,8,1) and USB serial interface's TXD to PD1 and RXD to PD0 directly. You can simplify the the curcuit by omitting the RS-232 part in the schematic as well as the I2C (but leave pull up resistors for PD4 and PD5)

The interface will create a ASCII file that can be stored on the PC and can also be used as input via terminal program for a LOAD command on the PB-100 If you only want to SAVE and LOAD programs, this will suffice. Example output ASCII is:

oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooo@bPn0i0i0i0i0i0i0i0i
0i0i0ingngngngngngngng0`0`0`0`0`0`T`0`0`0`0`0`lcRo0`oooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
oooooooooooooooooooooooooooooooo8bPh0`6eBjjiRiVmXilo8mBjng0i0`0e
RiPangPg


The are additional tools available (similar to pocket tools for the SHARP) that allow you to create ASCII files from a basic file or vice versa. So you can write a basic source file on the computer and make it available to the CASIO. A good thing is that if you have the original cassette interfaces you can also use the WAV file to create a basic file or the other way around.
See https://www.mvcsys.de/doc/casioutil.html?fbclid=IwAR3BQInIuff2lOjGMDEnFQwnSZRRsfY0eNkHBMZhDxatF_floZaMOhNQ3fk

** CASIO FX 602P **

I own a FA-2 cassette interface and was looking for tools to convert a keystroke source into a wav file and vice versa. The only tool I could find was a Delphi7 built program running on Windows. http://frickelfritze.bplaced.net/casio/wavetobin/index.html. I have used the insights from this program as well the generic source code for KCS encoding and decoding built by David Beazly https://www.dabeaz.com/py-kcs/.

There are four python programs with general usage python3 fx602pxxx2xxx.py arg=filename without extension. It assumes the following subdirectories in you working directory 
- utils: where the four programs are located
- resources: token translation tables for alpha and code mode
- src: your keystroke source program
- bin: the tokenized programs
- wav: the wav file generated or recorded.

Enjoy!


