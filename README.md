The 80's SHARP Pocketcomputers  used FSK Modulation for saving and loading basic programs to a cassette recorder. When you have the proper interface peripherals this is still a good way to archive your programs. Instead of a cassette recorder you can also use a pc with a mediaplayer like Audacity as a replacement. With software on the PC like pocket tools you can convert basic txt files into wav files for loading into to the SHARP pocket computer and vice versa. Troublesome is still getting the right audio levels and sampling rates to get it to work.

Alternativerly with modern micro controllers you can simplify the process and convert a basic file into machine readable tokens and simulate the tapeformat converted into pulse. This github directory contains a couple of Arduini INO files for your use. You will need some additional hardware like a breadboard, some components as well as a connector to the 11 pin socket on the pocket PC.
I have also added a CE-126P printer emulator that sends LLIST or LPRINT commands to a serial interface for for further processing e.g a serial printer.

Some machines like PC-1350, E-500, G850, etc,have a SIO interface on either the 11 pin or 15 pin socket. Much more convenient!
I have included a GUI program (Java or Python) for managing the transfer between a serial FTDI USB interface and the machine.
