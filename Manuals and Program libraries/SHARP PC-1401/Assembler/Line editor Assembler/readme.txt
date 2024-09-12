Instructions on how to load the machine language editor
-------------------------------------------------------
-First load the machine language part with CLOAD M "assembler"
-On the PC-1401 and 1402, the machine language part is loaded at memory address 8192 to 8364,
 which is right at the start of the BASIC memory. So you need to adjust the BASIC pointer upwards
 so the machine language cannot be overwritten by a BASIC program:
 PC-1401 : POKE 18145,172,56
 PC-1402 : POKE 18145,172,32
-Type NEW
-Load the BASIC part with CLOAD "assembler"

There are also versions available for the PC-1260, 1261 and 1350. 
If you need one of these versions, the source code is available in the Data Becker Book

Short instructions:
------------------
C-CE		Input start address
A-Z, 1-9, ENTER	Input mnemonics
Arrow down	Read next mnemonic
Arrow up	Read previous mnemonic (only works once)
<		Backspace
SHIFT+L		Print a listing
BRK		End program

When you made a typo or mistake, it sounds a BEEP 