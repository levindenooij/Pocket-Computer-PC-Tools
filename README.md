** Introduction **

This repository contains tools and programs for interfacing vintage SHARP Casio and TI pocketcomputers to your PC. If you have a vintage cassette interface compatible with the machine, you can load and store programs on a cassetterecorder using FSK modulation. Basically converting a bit patterns into audio wave patterns e.g 2400 HZ for a 1 and 1200 HZ for a 0.  Instead of a cassette recorder, nowadays, you can also use a PC with a mediaplayer like Audacity. In the latter the programs are stored as WAV files. 

In the early days of the PC, efforts have been made by several hobbyists or commercial parties to offer programs to generate WAV files from a source file and vice versa. 


** SHARP Interfacing to a PC ** 

The 80's SHARP Pocketcomputers also use FSK Modulation for saving and loading basic programs to a cassette recorder. When you have the proper interface peripherals this is still a good way to archive your programs. Most models can use the CE-126P printer/cassette interface. When connecting the CE-126P to the PC and using e.g. Audacity you can save programs to the PC and load to the pocket computer from the PC. If your PC is not equiped with a soundcards with 3.5 jack sockets or the soundcard performs bad, you might need one of these to usb to audio cables
https://nl.ugreen.com/products/ugreen-externe-stereo-geluidskaart-usb-audio-adapter-plug?currency=EUR&variant=43580130558151&utm_source=google&utm_medium=cpc&utm_campaign=Google%20Shopping&stkn=90f9b0511a45&gad_source=1&gclid=Cj0KCQjw_sq2BhCUARIsAIVqmQthwzPWHAI4QXc5N7Y2UEUyvNOM0l37cEibm85EqKGUeDFbKGYBL1saAvqoEALw_wcB

With software on the PC like Pocket Tools for SHARP Pocket Computers offered by Peil & Partners https://www.peil-partner.de/ifhe.de/sharp/, you can convert basic txt files into wav files for loading into to the SHARP pocket computer and vice versa. Troublesome with the CE-126P is to get the right audio levels and sampling rates to get it to work.


** SHARP 11-PIN HW replacement for CE-126P **

If you can't get your hands on a CE-126P or keep having troubles with the audio levels, despite amplification, you can built your own CE-126p cassette interface. I have added two schematics as an alternative for a real CE-126P one with passive and one with active components. Just link with ear and mic cables to a USB audio interface to your computer and use e.g Audacity in combination with Pocket Tools for SHARP Pocket Computers in the same way. Audiolevels are much better than the real CE-126P.

**SHARP 11-PIN Arduino as Emulator for the CE-126P cassette interface **

Alternatively with modern micro controllers you can simplify the process and convert a basic file into machine readable tokens and simulate the tapeformat converted into pulse. This github directory contains a couple of Arduino INO files for your use. Next to an ARDUINO MEGA (due to memory requirements) you will need some additional hardware like a breadboard, some components as well as a connector to the 11 pin socket on the pocket PC.

For cassette interface breadboard wiring guidance please have a look at: http://www.cavefischer.at/spc/html/PC-1401Key_Arduino.html. Don't use the source code mentioned there as it is not fit for purpose!
For this cassette interface you only need the upper part of the breadboard with push button, led and resistor.


** SHARP 11-PIN Arduino as Emulator for the CE-26P printer interface **

I have also added a SHARP CE-126P printer emulator that sends LLIST or LPRINT commands to a serial interface for for further processing e.g a serial printer. For the printer emulator breadboard wiring guidance please have look here: http://www.cavefischer.at/spc/html/CE-126P_Emulator.html

I build a permanent board with a Arduino Nano, pull down resistors and a 11 pin-connector. Be aware different source listings for 14xx vs 12xx types. As there a limited memory requirements a NANO is sufficient.

** SHARP 11 Pin & 15 pin SIO Interface **

Some machines like PC-1350, E-500, G850, etc,have a SIO interface on either the 11 pin or 15 pin socket. Much more convenient: you don't need software to convert into source code: you get ASCII directly!
I have included a few GUI programs (Java or Python) for managing the transfer between a serial FTDI USB interface and the machine. I have built a permanent board with a cheap FTDI clone and a inverter chip Sn74ls04n (the original FTDI can  be programmed to invert, the cheap ones not). Both for 11 as well 15 pin (different pin configuration)
Please have a look here for connector guidance : http://www.silicium.org/forum/viewtopic.php?t=42285

** CASIO 30 Pin connector for serial interface (FX850P, FX880P, VX4, ..) **

Some CASIO machines with a 30 pin connector have TTL level serial communication built in. 
Pin 9 RX, P14 TX, Pin30 GND (count top row, 1,3,5,7,9 and bottom row 2,4,6,8,10,14,...30
I have included a CASIO transfer program in Python. Easy to use with CASIO commands like SAVE "COM0:4,N,8,1" or LOAD "COM0,4,N.8.1". For more info on  serial connections, see the respective manuals. Also here the output is readable ASCII

** CASIO FX 702P, FX-700/PB-100, PB-700 and similar BASIC machines **

Cassete interfaces you will find are ( there are also printer/cassette interfaces like FA-10)
- FA-2 : FX702p
- FA-3 : FX 702P/PB-100
- FA-4 : PB-700

To store and load programs is a similar process as for the SHARP models. The are additional tools available (similar to pocket tools for the SHARP) that allow you to create ASCII files from a WAV file or vice versa. So you can write a basic source file on the computer and make it available to the CASIO or convert a stored program (WAV)) as source file. 
See https://www.mvcsys.de/doc/casioutil.html?fbclid=IwAR3BQInIuff2lOjGMDEnFQwnSZRRsfY0eNkHBMZhDxatF_floZaMOhNQ3fk

** Alternative circuit for the FA-4 cassette interface **

I own the FA-2 and FA-3 but was missing an interface for the PB-700 (FA-4). After some search on the internet I came accross the following website from Piotr Piatek, http://www.pisi.com.pl/piotr433/index.htm. A great source for knowledge on CASIO machines, data representation, basic program structures as well as alternative self built interfaces. As I was missing an interface for the PB-700 I replicated the circuit http://www.pisi.com.pl/piotr433/pb700tae.htm. It uses an old micro controller and has connections for RS-232 and I2C. A warning is in place the whole exercise is not for the faint-hearted 
- you need to program an old controller
- somewhat more complicated signal path than the SHARP one.
- the fact that the source code is all in assembler
- the wiring and soldering on a board could become messy and confusing
- quite a few points of failure possible in the process

Nevertheless I sourced the components and built the circuit and with some further guidance from Piotr I got it to work. The big tip was to omit RS-232 overall and link a terminal program (9600,8,1) and USB serial interface's TXD to PD1 and RXD to PD0 directly. You can simplify the circuit by omitting the RS-232 part in the schematic as well as the I2C (but leave pull up resistors for PD4 and PD5)

The interface will create an ASCII file that can be stored on the PC and can also be used as input via terminal program for a LOAD command on the PB-700 If you only want to SAVE and LOAD programs, this will suffice. Example output ASCII is:

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

This type of file can also be used by the CASIO utilities mentioned above in order to create a basic source file from the ASCII file or a ASCII file from the basic source file


** CASIO FX 602P **

The FX602p is not a basic pocket computer but a very sought after keystroke programmable calculator (like the HP15C)
I own a FA-2 cassette interface (compatible for FX602P and FX702P) and was looking for tools to convert a keystroke source into a wav file and vice versa. The only tool I could find that converted a WAV file , was a Delphi7 built program running on Windows. http://frickelfritze.bplaced.net/casio/wavetobin/index.html. I have used the insights from this program as well the generic source code for KCS encoding and decoding built by David Beazly https://www.dabeaz.com/py-kcs/ and built my own toolset for creating a source to bin, bin to wav, wav to bin and bin to source.

There are four python programs with general usage python3 fx602pxxx2xxx.py arg=filename without extension. It assumes the following subdirectories in you working directory 
- src: where the four programs are located
- res: token translation tables for alpha and code mode
- lst: your keystroke source program
- bin: the tokenized programs
- wav: the wav file generated or recorded.

Known issues:
x is sometimes duplicated as the letter x and the multiply sign look similar but do have a different binary value in the look up tables. 

** TI-74 **

I have made a attempt to provide some wav2bin and bin2src tools for the TI-74. The latter is no joke to figure out. I had a look at some work from .. 
https://www.mvcsys.de/doc/casioutil.html#mozTocId818664 but found it hard to digest. The wav2bin program is a simplified version of earlier work by Abraham Moller. # CIduino7: TI-95 Arduino Replacement Cassette Interface. Watch the demo here! https://www.youtube.com/watch?v=DMReYWH7o-4. The Arduino sends data to serial interface and the file will be stored as bin file. The bin2src python program is built from scratch uses the bin file along with a token conversion table to generate the original source code. The program provide good results with a simple factorial program but is in no way extensively tested. I the near future I might spend some more time on it but I passed the initial thrill of getting it to work to some extent.  

Known issues: Variable names are reissued starting with A. I could't figure out how variable names are dealt with in the bin file. 

Next on the bench maybe a CASIO FX-502P.

Enjoy!



