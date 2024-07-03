This directory contains the sio (serial interface IO) driver for PC-1403H and PC-1360
It will work on PC-1475 (=1360) also.
It is not tested on old 1403!

The precompiled files sio1403h.bas and sio1360.bas can be used immediately.
Start position in RAM is for both 32900.
sio1403 will start at 57400.
To compile the assemblies, use the included pasm.exe!


PREPARATION OF POCKET COMPUTER:

To move the BASIC area to make space for assembler programs do the following:
- PC-1403: POKE 65281,0,&E4 to get 1kB for assembler programs
- PC-1403H: POKE 65281,0,136 to get 2kB for assembler programs
- PC-1360: POKE &FFD7,0,136 (assuming 32kB RAM card) to get 2kB for assembler programs
	POKE &FFD7,0,&E4 (8kB card) to get 1kB for assembler programs
	POKE &FFD7,0,&C4 (16kB card) to get 1kB for assembler programs
- Enter NEW in PROGRAM MODE
- LOAD or input the .bas file and RUN it!
- NEW again
- connect the cable (Download instructions for making a serial cable for the 1403[H] here: http://www.lehmayr.de/1403H%20connect%20cable%20how%20to.txt)


DO A DATA TRANSFER:

Use PocketAsm from www.lehmayr.de for data transfers:
- Open PocketAsm (perhaps you'll have to select a language file)
- Right lower corner: select pocket model with right click
- Press F11 and config serial interface to 4800baud, no parity, 8 data bits, 1 stop bit, CTS active and ignore line end code (this is important!)

Now you can send and receive files (please note that receive and send window will only work with ASCII files)

To receive a BASIC program or VARIABLES from your pocket computer:
- Open PocketAsm
- Config serial interface (press F11) if needed
- Press Ctrl+F5 (Strg+F5)
- Type filename
- Enter CALL 32915 [ENTER] for BASIC transfer or CALL 32925 for VAR transfer
[1403: - Enter CALL 57415 [ENTER] for BASIC transfer or CALL 57425 for VAR transfer]
- Wait for transfer to finish (pocket computer will show ready prompt ">")
- PocketAsm: press Esc
- Done!

To send back BASIC programs and VAR:
- Open PocketAsm
- Config serial interface (press F11) if needed
- Press Ctrl+F4 (Strg+F4)
- Enter CALL 32920 [ENTER] for BASIC reception or CALL 32930 for VAR reception
[1403: - Enter CALL 57420 [ENTER] for BASIC reception or CALL 57430 for VAR reception]
- Select file
- Wait for transfer to finish (pocket computer will show ready prompt ">")
- Done!

To send a single byte:
- CALL 32900,<byte> (replace <byte> with a printable char that will be sent)
[1403: - CALL 57400,<byte> (replace <byte> with a printable char that will be sent)]
- CALL 32905 (POKE the byte into 32899 before that)
[1403: - CALL 57405 (POKE the byte into 57399 before that)]

To receive a single byte:
- CALL 32910 (the byte will be in PEEK 32899)
[1403: - CALL 57410 (the byte will be in PEEK 57399)]

To open and print a received BASIC program: 
- Open the freshly received binary BASIC dump file from you pocket computer. 
- Answer the question "BASIC?" with Yes (Ja) and select model from the appearing check list
- Then you program should appear in the window.
- ATTENTION: You must select the pocket computer model first! 1403 equals 1403H

To edit a BASIC program on the big PC:
- Preparations: You'll need a hex editor for minor adjustments: Get the free http://www.hhdsoftware.com/hexeditor.html and install it.
- Open PocketAsm
- Select pocket model
- Open file or input BASIC code
- Store file as *.td file
- Open td file with Hex Edit
- Remove the first 17 bytes by pressing del (it's important to get also the first FF before the basic data starts)
- Save the file as *.bin
- Use PocketAsm to send the binary file to your pocket


IMPORTANT NOTES:
Due to the usage of binary data, the transferred programs are only compatible with the same model!
To transfer a program between models you must use PocketAsm as a converter.



Have fun,
Simon

www.lehmayr.de
