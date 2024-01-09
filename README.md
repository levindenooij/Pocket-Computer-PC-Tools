This repository contains tools and programs for interfacing vintage SHARP pocketcomputers to your PC as an alternative to what is already offered by Peil & Partners https://www.peil-partner.de/ifhe.de/sharp/ in combination with a vintage CE-126P printer cassette interface.
Also for CASIO some interfacing options are available.

** SHARP Interfacing to a PC ** 

The 80's SHARP and CASIO Pocketcomputers  used FSK Modulation for saving and loading basic programs to a cassette recorder. When you have the proper interface peripherals this is still a good way to archive your programs. Instead of a cassette recorder you can also use a pc with a mediaplayer like Audacity as a replacement. With software on the PC like Pocket Tools for SHARP Pocket Computers you can convert basic txt files into wav files for loading into to the SHARP pocket computer and vice versa. Troublesome is still getting the right audio levels and sampling rates to get it to work.

** SHARP 11-PIN HW replacement for CE-126P **

I have added two schematics as an alternative for a real CE-126P one with passive and one with active components. Just link with ear and mic cables to a USB audio interface to your computer and use e.g Audacity in combination with Pocket Tools for SHARP Pocket Computers. Audiolevels are much better than the real CE-126P.

**SHARP 11-PIN Arduino as Emulator for the CE-26P cassette interface **

Alternatively with modern micro controllers you can simplify the process and convert a basic file into machine readable tokens and simulate the tapeformat converted into pulse. This github directory contains a couple of Arduino INO files for your use. Next to an ARDUINO MEGA (due to memory requirements) you will need some additional hardware like a breadboard, some components as well as a connector to the 11 pin socket on the pocket PC.

For cassette interface breadboard guidelines please have a look at: http://www.cavefischer.at/spc/html/PC-1401Key_Arduino.html. Don't use the source code mentioned there as it is not fit for purpose!
For this cassette interface you only need the upper part of the breadboard with push button, led and resistor.

** SHARP 11-PIN Arduino as Emulator for the CE-26P printer interface **

I have also added a SHARP CE-126P printer emulator that sends LLIST or LPRINT commands to a serial interface for for further processing e.g a serial printer. For the printer emulator breadboard wiring guidance please have look here: http://www.cavefischer.at/spc/html/CE-126P_Emulator.html
I build a permanent board with a Arduino Nano, pull down resistors and a 11 pin-connector. Be aware different source listings for 14xx vs 12xx types. As there a limited memory requirements a NANO is sufficient.

**SHARP 11 Pin & 15 pin SIO Interface**

Some machines like PC-1350, E-500, G850, etc,have a SIO interface on either the 11 pin or 15 pin socket. Much more convenient!
I have included a GUI programs (Java or Python) for managing the transfer between a serial FTDI USB interface and the machine. I have built a permanent board with a cheap FTDI clone and a inverter chip Sn74ls04n (the original FTDI can  be programmed to invert, the cheap ones not). Both for 11 as well 15 pin (different pin configuration)
Please have a look here for connector guidance : http://www.silicium.org/forum/viewtopic.php?t=42285

**CASIO 30 Pin connector for serial interface (FX850P, FX880P, VX4, ..)**

Some CASIO machines with a 30 pin connector have TTL level serial communication built in. 
Pin 9 RX, P14 TX, Pin30 GND (count top row, 1,3,5,7,9 and bottom row 2,4,6,8,10,14,...30
I have included a CASIOtransfer program in Python. Easy to use with CASIO commands like SAVE "COM0:4,N,8,1" or LOAD "COM0,4,N.8.1". For more info on  serial connections, see the respective manuals.



