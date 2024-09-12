Macros for PC-1360
	Number operations
		Addition|Addition of two numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). X = X + Y|The sum is saved to X in BCD.|0|CALL	&1E80|CALL	&1E80|
		Subtraction|The numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31) are subtracted. X = Y - X|BCD result in X.|0|CALL	&1E84|CALL	&1E84|
		Multiplication|Multiplication of the two BCD numbers in operation registers X (int. ram 16-23) and Y (24-31). X = Y * X|BCD result in X.|0|CALL	&1E88|CALL	&1E88|
		Division|Division of two BCD numbers in operation registers X (int. ram 16-23) and Y (24-31). X = Y / X|BCD result in X.|0|CALL	&1E8C|CALL	&1E8C|
		Square root|Square root of a number in BCD format in operation register X (int. ram 16-23). X = SQRT(X)|BCD result in X.|0|CALL	&1EF4|CALL	&1EF4|
		Logarithm LN|Natural logarithmus LN of a number in BCD format in operation register X (int. ram 16-23). X = LN X|BCD result in X.|0|CALL	&1EFC|CALL	&1EFC|
		Logarithm LOG|Logarithmus LOG of a number in BCD format in operation register X (int. ram 16-23). X = LOG X|BCD result in X.|0|CALL	&1EF8|CALL	&1EF8|
		Exponent|EXP e^X of a number in BCD format in operation register X (int. ram 16-23). X = e^X|BCD result in X.|0|CALL	&1EC0|CALL	&1EC0|
		Sinus|Sinus of a number in BCD format in operation register X (int. ram 16-23). X = SIN X|BCD result in X.|0|CALL	&1EC4|CALL	&1EC4|
		Cosinus|Cosinus of a number in BCD format in operation register X (int. ram 16-23). X = COS X|BCD result in X.|0|CALL	&1EC8|CALL	&1EC8|
		Tangens|Tangens of a number in BCD format in operation register X (int. ram 16-23). X = TAN X|BCD result in X.|0|CALL	&1ECC|CALL	&1ECC|
		Arcussinus|Arcussinus of a number in BCD format in operation register X (int. ram 16-23). X = ASN X|BCD result in X|0|CALL	&1ED0|CALL	&1ED0|
		Arcuscosinus|Arcuscosinus of a number in BCD format in operation register X (int. ram 16-23). X = ACS X|BCD result in X.|0|CALL	&1ED4|CALL	&1ED4|
		Arcustangens|Arcustangens of a number in BCD format in operation register X (int. ram 16-23). X = ATN X|BCD result in X.|0|CALL	&1ED8|CALL	&1ED8|
		DEG|Conversion from DMS to degrees of a number in BCD format in operation register X (int. ram 16-23). X = DEG X|BCD result in X.|0|CALL	&1EDC|CALL	&1EDC|
		DMS|Conversion of degree to DMS of a number in BCD format in operation register X (int. ram 16-23). X = DMS X|BCD result in X.|0|CALL	&1EE0|CALL	&1EE0|
		Absolute value|Absolute value of a number in BCD format in operation register X (int. ram 16-23). X = ABS X|BCD result in X.|0|CALL	&1EE4|CALL	&1EE4|
		Integer|Integer value (cut) in BCD format in operation register X (int. ram 16-23). X = INT X|Result as BCD number in X.|0|CALL	&1EE8|CALL	&1EE8|
		Sign|Sign of a number in BCD format in operation register X (int. ram 16-23). X = SGN X|The BCD result (-1, 0, +1) is in X.|0|CALL	&1EEC|CALL	&1EEC|
		Random number|Random number of a number in BCD format in operation register X (int. ram 16-23). X = RND X|BCD result in X. X between 0..1 results in a number of real, X >= 1 results random from 1..X.|0|CALL	&1EEC|CALL	&1EEC|
	Comparisons
		Y <> X|Y<>X: Numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). Strings as pointer in the last 4 bytes: &0D LB HB length. Routine in bank 0!|If true then BCD 1 in X else 0.|1|CALL	&6AA3|CALL	&6AA6|CALL	&6AAC|CALL	&6AAF|
		Y = X|Y=X: Numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). Strings as pointer in the last 4 bytes: &0D LB HB length. Routine in bank 0!|If true then BCD 1 in X else 0.|1|CALL	&6A87|CALL	&6A8A|CALL	&6A95|CALL	&6A98|
		Y > X|Y>X: Numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). Strings as pointer in the last 4 bytes: &0D LB HB length. Routine in bank 0!|If true then BCD 1 in X else 0.|1|CALL	&6A4C|CALL	&6A4F|CALL	&6A55|CALL	&6A58|
		Y < X|Y<X: Numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). Strings as pointer in the last 4 bytes: &0D LB HB length. Routine in bank 0!|If true then BCD 1 in X else 0.|1|CALL	&6A37|CALL	&6A3A|CALL	&6A42|CALL	&6A45|
		Y >= X|Y>=X: Numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). Strings as pointer in the last 4 bytes: &0D LB HB length. Routine in bank 0!|If true then BCD 1 in X else 0.|1|CALL	&6A72|CALL	&6A75|CALL	&6A7D|CALL	&6A80|
		Y <= X|Y<=X: Numbers in BCD format in operation registers X (int. ram 16-23) and Y (24-31). Strings as pointer in the last 4 bytes: &0D LB HB length. Routine in bank 0!|If true then BCD 1 in X else 0.|1|CALL	&6A5F|CALL	&6A62|CALL	&6A68|CALL	&6A6B|
	Conversions
		STR$ (BCD to string)|STR$ converts a BCD number to a string. Write the number in BCD format in operation register X (int. ram 16-23) and &60 to string buffer &FE79.|OR X contains the pointer to the string in the last 4 bytes: &0D LB HB length.|0|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F08|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F08|
		CHR$ (BCD to ASCII)|CHR$ converts a BCD number from 0..255 to a character (string). Write the number to operation register X (int. ram 16-23) and &60 to string buffer &FE79.|Carry = 0: OR X contains the pointer to the string in the last 4 bytes: &0D LB HB Length.\nCarry = 1: The number is not valid.|0|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F04|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F04|
		VAL (string to BCD)|VAL converts a string to a BCD number. Operation register X (int. ram 16-23) must have the pointer in the last 4 bytes (&0D &79 &FE length) and the string must be at &FE79 (string buffer).|Carry = 0: OR X contains the BCD result.\nCarry = 1: The conversion was not possible.|0|CALL	&1F0C|CALL	&1F0C|
		ASC (ASCII to BCD)|ASC converts a character to the BCD number of the ASCII code. Operation register X (int. ram 16-23) should have &0D &79 &FE &01 as content in the last 4 bytes The char must be at (string buffer) &FE79.|OR X has the resulting BCD number.|0|CALL	&1F00|CALL	&1F00|
		BCD to HB/LB|BCD->HB/LB converts a BCD number to two bytes. Operation register X (int. ram 16-23) has the number. Version 0 and 1 are the same but there are two versions of conversion: For numbers from 0..65535 version 1 and from -32768..32767 version 0.|Carry 0: LB in &18 and HB in &19 of internal RAM\nCarry 1: X is not valid|0|CALL	&0912|CALL	&091D|
		HB/LB to BCD|HB/LB->BCD converts two bytes to a BCD number. LB byte to &18 and HB to &19. Version 0 and 1 are the same but there are two converters: For result range from 0..65535 choose version 1 and from -32768..32767 version 0.|Operation register X (int. ram 16-23) contains the BCD result.|0|CALL	&0891|CALL	&088A|
	Search operations
		Search for line numbers|Version 1: Search for line numbers in HB/LB format: LB &18 and HB &19 int. RAM. Version 0: In OR X (16-23) is the line number in BCD format, internal RAM &36 OR 00000010 is executed.\nRoutine in bank 2!|Carry = 0: Line was found. Data in int. RAM:\n	&3A Address of line LB\n	&3B Address of line HB\n	&3C Line number HB\n	&3D Line number LB\nCarry = 1: Line was not found.\n	&3C and &3D = 0: Nothing similar found.\n	&3C or &3D <> 0: There exists a line number which is bigger.|0|LIP	&36\nORIM	2\nLIA	2\nLIDP	&3400\nSTD\nCALL	&5EBE\nLIA	0\nLIDP	&3400\nSTD|LIP	&36\nORIM	2\nLIA	2\nLIDP	&3400\nSTD\nCALL	&5ECB\nLIA	0\nLIDP	&3400\nSTD|
		Search for simple variable|Search for a single variable with two character name. First name byte to &0A int. RAM and second to &0B. Set &33 to 0.|The address of the first byte of the variable is stored at &06 LB and &07 HB (Y). A contains the length of the variable. If the variable doesn't exists the computer hangs.|0|LIP	&33\nANID	0\nCALL	&6E82|LIP	&33\nANID	0\nCALL	&6E85|
		Search for array variable|Search for an array. First name byte to &0A int. RAM and second to &0B. &33 must be 0. The sizeof the array must be saved to &0C and &0D.\nOne dimension: &0C contains size, &0D = 0\nTwo dimensions: &0D first size and &0C second|Carry = 0: The address of the first byte in the array is stored &06 LB and &07 HB (Y). A contains the length of a single unit.\nCarry = 1: An array fitting to the search criteria was not found.|0|CALL	&7135|CALL	&7138|
	Keyboard
		Get key|Get key|Carry = 0: No key was pressed.\nCarry = 1: Key code in A: 0..46, 47 when more keys were pressed.|0|CALL	&1E9C|CALL	&1E9C|
		ASCII key|ASCII key input: Waits for a character. You can use SML, DEF, etc, too!|Saves the ASCII code to B|0|CALL 18442|CALL 18442|
	Display
		Write to screen|Display line addresses:\n0 &6D00 / &FD97\n1 &6D18 / &FDAF\n2 &6D30 / &FDC7\n3 &6D48 / &FDDF\nRegister K=0, set bit 2 in &FD01, A = 0..3 for single lines, 4 to write whole display.|The lines are printed on the screen.|0|LP	8\nANIM	0\nLIDP	&FD01\nORID	4\nLIA	1\nLIDP	&3400\nSTD\nLIA	4\nCALL	&4004\nLIA	0\nLIDP	&3400\nSTD|LP	8\nANIM	0\nLIDP	&FD01\nORID	4\nLIA	1\nLIDP	&3400\nSTD\nLIA	4\nCALL	&4004\nLIA	0\nLIDP	&3400\nSTD|
		Single character|Print a single character on the screen at a defined position: The address &FD10 contains X, &FD11 Y coordinate (24*4 character, starting with 0).\nBit 0 in &FD01 must be set\nA = ASCII char\nRoutine in bank 1|The character is printed on the screen.|0|LIDP	&FD01\nORID	1\nLIA	1\nLIDP	&3400\nSTD\nLIA	65\nCALL	&615E\nLIA	0\nLIDP	&3400\nSTD|LIDP	&FD01\nORID	1\nLIA	1\nLIDP	&3400\nSTD\nLIA	65\nCALL	&615E\nLIA	0\nLIDP	&3400\nSTD|
		Scroll down|Scrolling:\nA = 4 (Routine in bank 1)|The screen scrolls so that the last line is empty.|0|LIA	1\nLIDP	&3400\nSTD\nLIA	4\nCALL	&4043\nLIA	0\nLIDP	&3400\nSTD|LIA	1\nLIDP	&3400\nSTD\nLIA	4\nCALL	&4043\nLIA	0\nLIDP	&3400\nSTD|
		Display off|Display off| |0|CALL	&1E98|CALL	&1E98|
		Display on|Display on| |0|CALL	&1E94|CALL	&1E94|
	Serial interface
		Open SIO|Open serial interface|ER signal becomes high. The interface can be accessed now.|0|CALL	&1F2C|CALL	&1F2C|
		Close SIO|Close SIO|All lines go low.|0|CALL	&1F30|CALL	&1F30|
		CS state|CS line monitor|Carry = 0: CS line is high.\nCarry = 1: Break was pressed.|0|CALL	&1EA0|CALL	&1EA0|
		CD state|CD line monitor|Carry = 0: CD line is high.\nCarry = 1: Break was pressed.\nOtherwise the routine doesn't stop.|0|CALL	&1EA4|CALL	&1EA4|
		Get SIO state|The conditions of SIO are being prepared: This routine must be called before sending or receiving occurs!|The conditions are stored in int. RAM:\nEOT code &0D\nBaud rate &0E\nBit conditions &0F|0|CALL	&1EA8|CALL	&1EA8|
		Send one byte|Send one byte: SIO parametres must be in int. RAM!\nA enthält das Byte.|The byte is sent.|0|CALL	&1F34|CALL	&1F34|
		Receive one byte|Receive one byte: SIO parametres must be in int. ram!|Received byte in B\nCarry = 0: No error, RR goes low\nCarry = 1: Bit 5 in &35 int. RAM = 1: Break pressed, otherwise parity error|0|CALL	&1F38|CALL	&1F38|
		Output termination code|Send the ending code: SIO parametres must be set in int. RAM!|Carry = 0: No error\nCarry = 1: Break was pressed|0|CALL	&1F3C|CALL	&1F3C|
		Internal format to ASCII|Convert internal format to ASCII sequenz: Register X contains the number in internal format.|The SIO buffer (&FB00) contains the resulting string terminated with CHR 13.|0|CALL	&1F40|CALL	&1F40|
		Output SIO buffer|Send SIO buffer: The buffer (&FB00-&FBFF) must be filled with data.|The buffer (&FB00-&FBFF) is sent until CHR 13. CHR 13 is sent, too!\nCarry = 0: No error\nCarry = 1: Break was pressed|0|CALL	&1F40|CALL	&1F40|
	Printer
		Reset printer|Reset printer|Printer ready.|0|CALL	&1F18|CALL	&1F18|
		Print line|Print a line: The line is stored at operation registers X (&10-&17), Y (&18-&1F) and Z (&20-&27)|The 24 bytes are printed and paper feed occurs.|0|CALL	&1F1C|CALL	&1F1C|
		Paper feed|Paper feed: Spaces are put in the operation registers X (&10-&17), Y (&18-&1F) and Z (&20-&27).|The spaces are printed and the paper is transported.|0|LP	&10\nLII	23\nLIA	32\nFILM\nCALL	&1F1C|LP	&10\nLII	23\nLIA	32\nFILM\nCALL	&1F1C|
	Cassette
		Remote on|Remote tape is activated|The tape starts when the computer is ready.|0|CALL	&1F24|CALL	&1F24|
		Remote off|Remote tape control deactivated|The tape plays always.|0|CALL	&1F28|CALL	&1F28|
		Output header|Tape header is saved on tape: File name in OR Z, starting with second byte (&21-&27)\n7 bytes maximum length, other bytes must be = &00\n&31 int. RAM must be 0|The header with filename is saved on tape.|0|LP	&31\nANIM	0\nCALL	&1F28|LP	&31\nANIM	0\nCALL	&1F28|
		Input header|Load header from tape: File name in OR Z, starting in second byte (&21-&27)\n7 bytes maximum length, other bytes = &00\nRoutine in bank 4!|The header is searched for on tape and read. If the file was found an * is being showed in the bottom right corner on the screen.|0|LIA	4\nLIDP	&3400\nSTD\nCALL	&68FD\nLIA	0\nLIDP	&3400\nSTD|LIA	4\nLIDP	&3400\nSTD\nCALL	&6902\nLIA	0\nLIDP	&3400\nSTD|
		Save one character|Save one byte to tape. Write the byte to A.\nRoutine in bank 4!|The byte is saved on tape.|0|LIA	4\nLIDP	&3400\nSTD\nCALL	&5BBF\nLIA	0\nLIDP	&3400\nSTD|LIA	4\nLIDP	&3400\nSTD\nCALL	&5BC4\nLIA	0\nLIDP	&3400\nSTD|
	Additional
		Switch ROM bank|Switch ROM bank: The bank has to be 0 when returning to BASIC!\nIn A is the bank number 0..7.|The bank is switched.|0|LIA	0\nLIDP	&3400\nSTD|LIA	0\nLIDP	&3400\nSTD|
	System variables
		A|Address of variable A||0|64152|64152|
		B|Address of variable B||0|64144|64144|
		C|Address of variable C||0|64136|64136|
		D|Address of variable D||0|64128|64128|
		E|Address of variable E||0|64120|64120|
		F|Address of variable F||0|64112|64112|
		G|Address of variable G||0|64104|64104|
		H|Address of variable H||0|64096|64096|
		I|Address of variable I||0|64088|64088|
		J|Address of variable J||0|64080|64080|
		K|Address of variable K||0|64072|64072|
		L|Address of variable L||0|64064|64064|
		M|Address of variable M||0|64056|64056|
		N|Address of variable N||0|64048|64048|
		O|Address of variable O||0|64040|64040|
		P|Address of variable P||0|64032|64032|
		Q|Address of variable Q||0|64024|64024|
		R|Address of variable R||0|64016|64016|
		S|Address of variable S||0|64008|64008|
		T|Address of variable T||0|64000|64000|
		U|Address of variable U||0|63992|63992|
		V|Address of variable V||0|63984|63984|
		W|Address of variable W||0|63976|63976|
		X|Address of variable X||0|63968|63968|
		Y|Address of variable Y||0|63960|63960|
		Z|Address of variable Z||0|63952|63952|
	Memory locations
		Basic start address|Basic start address Version 1=LB 2=HB||0|&FFD8|&FFD7|
		Basic end address|Basic end address Version 1=LB 0=HB||0|&FFDA|&FFD9|
		Merge address|Merge address Version 1=LB 0=HB||0|&FFDC|&FFDB|
		Graphic cursor X|Graphic cursor X Version 1=LB 0=HB||0|&FEE9|&FEE8|
		Graphic cursor Y|Graphic cursor Y Version 1=LB 0=HB||0|&FEEB|&FEEA|
		Text cursor X|Text cursor X (0-23)||0|&FD09|&FD09|
		Text cursor Y|Text cursor Y (0-3)||0|&FD0A|&FD0A|
		Display pointer X|Display pointer X (0-23)||0|&FD10|&FD10|
		Display pointer Y|Display pointer Y (0-3)||0|&FD11|&FD11|
		End position of last LINE X|End position of last LINE (X) Version 1=LB 0=HB||0|&FEED|&FEEC|
		End position of last LINE Y|End position of last LINE (Y) Version 1=LB 0=HB||0|&FEEF|&FEEE|
		Cursor position in input buffer|Cursor position in input buffer||0|&FD16|&FD16|
		WAIT value|WAIT value Version 1=LB 0=HB||0|&287F|&287E|
		Character at cursor|ASCII code of char below the cursor||0|&FF02|&FF02|
		Address of cursor|Address of cursor in display buffer Version 1=LB 0=HB||0|&FF01|&FF00|
		Variable start|Start of selfdefined variables Version 1=LB 0=HB||0|&FFDE|&FFDD|
		BREAK address|BREAK address Version 1=LB 0=HB||0|&FEF5|&FEF4|
		ERROR address|ERROR address Version 1=LB 0=HB||0|&FEF7|&FEF6|
		Auto power off counter|Auto power off counter Version 1=LB 0=HB||0|&2A7F|&2A7E|
		RAM card start address|RAM card start address Version 1=LB 0=HB||0|&FFF7|&FFF6|
		RAM card end address|RAM card end address Version 1=LB 0=HB||0|&FFF9|&FFF8|
		Reserve mode|Reserve mode Version 1=start byte 0=last byte||0|&FFCF|&FF40|
		Reserve mode length|Reserve mode length||0|&FF3F|&FF3F|
		Input buffer|Input buffer Version 1=start byte 0=last byte||0|&FD6F|&FD20|
		Display buffer|Display buffer in ASCII code Version 1=start byte 0=last byte||0|&FDDF|&FD80|
		String buffer|String buffer Version 1=start byte 0=last byte||0|&FCAF|&FC60|
		Display memory 1|Display memory column 1 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|10304|10240|
		Display memory 2|Display memory column 2 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|10816|10752|
		Display memory 3|Display memory column 3 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|11328|11264|
		Display memory 4|Display memory column 4 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|11840|11776|
		Display memory 5|Display memory column 5 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|12318|12288|
		Special symbols|The symbol byte|Bit 0=SHIFT\nBit 1=DEF\nBit 4=RUN\nBit 5=PRO\nBit 6=JAPAN\nBit 7=SML|0|12348|12348|
		Port address serial|Address for output at 15 pin interface|Bit 0=Pin 4\nBit 1=Pin 14\nBit 2=Pin 11|0|&3800|&3800|
		Port address printer|Address for output at 11 pin interface|Bit 0=Pin 11\nBit 1=Pin 10\nBit 2=Pin 5\nBit 3=Pin 8|0|&3A00|&3A00|
		B port|B port output: Bit 4=Pin 9 at printer, Bit 5=Pin 3, Bit 6=Pin 5 and Bit 7=Pin 8 at serial\nVersion 1=output, 0=input|Input: Bit 3=Pin 8 and Bit 4=Pin 9 at printer, Bit 5=Pin 3, Bit 6=Pin 5 and Bit 7=Pin 8 at serial|0|INB|LIP	&5D\nORIM	bit\nOUTB|
		F port|F port output: Bit 1=Pin 2 at serial||0|LIP	&5E\nORIM	2\nOUTF|LIP	&5E\nORIM	2\nOUTF|
PC-1350 Macros
	Sound
		BEEP short|Beep short: Changed registers: P,Q,A. Internal RAM: 2,95|If the bit 0 in A is set, 2 kHz else 4 kHz. To stop the beeper use LIP 95, ANIM 239, OUTC|0|CALL	2379|CALL	2379|
		BEEP long|Beep long: Changed registers: unknown. Internal RAM: unbekannt|Beeps with 4 kHz and doesn't need to be stopped.|0|CALL	49430|CALL	49430|
	Display
		Print|PRINT: Changed registers: P,Q,DP,A,B,X,Y,K,L,V. Internal RAM: 2..39|Prints the ASCII characters in the display buffer like CURSOR(K,A), you need to define A=1,2 for the lower two lines and A=3,4 the upper.|0|CALL	53958|CALL	53958|
		LPrint|LPRINT: Changed registers: P,Q,DP,A,B,K,L. Internal RAM: 2,3,8,9,92..95|Prints the characters in internal RAM addresses 16..39 on the CE-126P. Chr(13) makes a new line.|0|CALL	32852|CALL	32852|
	Keyboard
		Getkey|Get key|Carry = 0: No key was pressed.\nCarry = 1: Key code in A: 0..46, 47 when more keys were pressed.|0|CALL	&1E9C|CALL	&1E9C|
		Inkey|Inkey: Changed registers: P,Q,DP,A,B,X,Y,K,L,V. Internal RAM: 10..21,23,34,37..39,55..63,92|If you press a key the code will be stored at [++Y].|0|CALL	3003|CALL	3003|
	Other
		Computer off|OFF: Changed registers: alle. Internal RAM: all|Switches the computer off. Switch it on again with On/Brk.|0|CALL	1240|CALL	1240|
	System variables
		A|Address of variable A||0|27896|27896|
		B|Address of variable B||0|27888|27888|
		C|Address of variable C||0|27880|27880|
		D|Address of variable D||0|27872|27872|
		E|Address of variable E||0|27864|27864|
		F|Address of variable F||0|27856|27856|
		G|Address of variable G||0|27848|27848|
		H|Address of variable H||0|27840|27840|
		I|Address of variable I||0|27832|27832|
		J|Address of variable J||0|27824|27824|
		K|Address of variable K||0|27816|27816|
		L|Address of variable L||0|27808|27808|
		M|Address of variable M||0|27800|27800|
		N|Address of variable N||0|27792|27792|
		O|Address of variable O||0|27784|27784|
		P|Address of variable P||0|27776|27776|
		Q|Address of variable Q||0|27768|27768|
		R|Address of variable R||0|27760|27760|
		S|Address of variable S||0|27752|27752|
		T|Address of variable T||0|27744|27744|
		U|Address of variable U||0|27736|27736|
		V|Address of variable V||0|27728|27728|
		W|Address of variable W||0|27720|27720|
		X|Address of variable X||0|27712|27712|
		Y|Address of variable Y||0|27704|27704|
		Z|Address of variable Z||0|27696|27696|
	Memory locations
		Basic start address|Basic start address Version 1=LB 2=HB||0|28418|28417|
		Basic end address|Basic end address Version 1=LB 0=HB||0|28420|28419|
		Merge addresse|Merge address Version 1=LB 0=HB||0|28422|28421|
		WAIT value|WAIT value Version 1=LB 0=HB||0|28852|28851|
		WAIT state|WAIT state: 2=active, 6=off||0|28435|28435|
		Variable start|Start of selfdefined variables Version 1=LB 0=HB||0|28424|28423|
		Reserve mode|Reserve mode area Version 1=start byte 0=last byte||0|28671|28527|
		Reserve mode length|Reserve mode length||0|28526|28526|
		Input buffer|Input buffer Version 1=start byte 0=last byte||0|28415|28336|
		Last pressed key|Last pressed key||0|28503|28503|
		Display buffer|Display buffer in ASCII format Version 1=start byte 0=last byte||0|27999|27904|
		Password|Password in ASCII code Version 1=start byte 0=last byte||0|28432|28426|
		Password state|Password state: Bit 5=Password active||0|28436|28436|
		Display buffer 1|Display buffer column 1 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|28736|28672|
		Display buffer 2|Display buffer column 2 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|29248|29184|
		Display buffer 3|Display buffer column 3 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|29760|29696|
		Display buffer 4|Display buffer column 4 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|30272|30208|
		Display buffer 5|Display buffer column 5 Version 1=1./3. line 0=2./4. line|Every area is 60 bytes long. Each 30 bytes belongs to the two mentioned lines.|0|30784|30720|
		Special symbols|Special symbol byte|Bit 0=SHIFT\nBit 1=DEF\nBit 4=RUN\nBit 5=PRO\nBit 6=JAPAN\nBit 7=SML|0|30780|30780|
		B port|B port in and output: Bit 4=Pin 11, Bit 5=Pin 10, Bit 6=Pin 9, Bit 7=Pin 8 at printer interface\nVersion 1=output, 0=input||0|INB|LIP	&5D\nORIM	bit\nOUTB|
		F port|F port only output: Bit 1=Pin 5 and Bit 2=Pin 4 at 11 pin interface||0|LIP	&5E\nORIM	bit\nOUTF|LIP	&5E\nORIM	bit\nOUTF|
PC-1401/02 Macros
	ASM basic macros
		LD  Y  <- X|PC1401/02  LD Y,X: CPU exchange register content (no BCD)|Result: Y:=X, P=8, Q=6|0|LP	6				# LD Y,X\nLIQ	4\nMVB|CAL	&15EE				# LD Y,X|
		LD  X  <- Y|PC1401/02  LD X,Y: CPU exchange register content (no BCD)|Result: X:=Y, P=6, Q=8|0|LP	4				# LD X,Y\nLIQ	6\nMVB|CAL	&15F3				# LD X,Y|
		LD (P) <- X|PC1401/02  LD (P),X: CPU exchange register content (no BCD)|Result: (P):=X, P=P+2, Q=6|0|LIQ	4				# LD (P),X\nMVB|CAL	&15EF				# LD (P),X|
		LD (P) <- Y|PC1401/02  LD (P),Y: CPU exchange register content (no BCD)|Result: (P):=Y, P=P+2, Q=8|0|LIQ	6				# LD (P),Y\nMVB|CAL	&15F4				# LD (P),Y|
		LD  X  <- BA|PC1401/02  LD X,BA: CPU exchange register content (no BCD)|Result: X:=BA, P=6, Q=4|0|LP	4				# LD X,BA\nLIQ	2\nMVB|CAL	&267				# LD X,BA|
		LD  Y  <- BA|PC1401/02  LD Y,BA: CPU exchange register content (no BCD)|Result: Y:=BA, P=8, Q=4|0|LP	6				# LD Y,BA\nLIQ	2\nMVB|CAL	&264				# LD Y,BA|
		LD (P) <- BA|PC1401/02  LD (P),BA: CPU exchange register content (no BCD)|Result: (P):=BA, P=P+2, Q=4|0|LIQ	2				# LD (P),BA\nMVB|CAL	&268				# LD (P),BA|
		PUSH X|PC1401/02  PUSH X: CPU push register content (no BCD) into stack|Result: changed P, Q, A:=R(new)|0|LDR					# PUSH X\nDECA\nDECA	\nSTP\nLIQ	4\nMVB\n# if as a sub: STQ EXB\nSTR|CAL	&1321				# PUSH X|
		POP X|PC1401/02  POP X: CPU load register content (no BCD) from stack|Result: changed P, Q, A:=R(new), B at CAL version|0|LDR					# POP X, don't use as a sub\nLP	4\nSTQ	\nMVB\nINCA\nINCA	\nSTR|CAL	&132C				# POP X|
	BASIC
		Basic Main|PC1401/02 MAIN: BASIC interpreter stack reset|All open GOSUB/RETURN and FOR/NEXT blocks are reset|0|# CALL	&D031|CALL	&D031				# BASIC:Main|
	Display
		Display off|PC1401/02 Display off|The display has no power.|0|# CAL	&59E|CAL	&59E				# Display off|
		Display an|PC1401/02 Display on||0|# CAL	&5A2|CAL	&5A2				# Display on|
		Display and wait|PC1401/02 Display on and waiting|Waits like print |0|# CALL	&C53E|CALL	&C53E				# Display PRINT|
		Display off STR$|PC1401/02 PRINT Graphical conversion and output of the ASCII characters in CPU RAM &10-&1F|Changed: P,Q,DP, A,B,X,Y,L,W,V,RC,RD|0|# CALL	&8015|CALL	&8015				# PRINT Display off STR$|
	Printer
		Print a line|PC1401/02: Print a line on CP-126: Write data to OR X (&10-&17), Y (&18-&1F) and Z (&20-&27)|The 24 bytes are printed and paper is fed. Changed REG: P,Q, A,B,K,L|0|# CALL	&804E|CALL	&804E				# LPRINT|
	Cassette
		Remote start|PC1401/02: Starts recoder|Changed: P,DP, A,B, C flag is set when an error occurred.|0|# CALL	&803F|CALL	&803F				# TAPE ON|
		Remote stop|PC1401/02: Stops recorder|Changed: P,DP, A,B, C flag is set when an error occurred.|0|# CALL	&8042|CALL	&8042				# TAPE OFF|
	Other
		PC off|PC1401/02 switch off||0|# CAL	?|CAL	&59A                  # PC switch off|
		Switch RAM bank|PC1401/02: Switch an additional inserted RAM bank - Holger Meyer Pocket Computer Service|The switch comparison is used for number = bank A and string = bank B.|1|LIDP	&E800			# Switch RAM bank A\nSTD|LIDP	&E800			# Switch RAM bank A\nSTD|LIDP	&F000			# Switch RAM bank b\nSTD|LIDP	&F000			# Switch RAM bank B\nSTD|
PC-1403 Macros
	Display
		Display on|PC1403 Switch display on||0|CAL	&4B8				# Display on|CAL	&4B8				# Display on|
	Other
		PC off|PC1403 switch off||0|CAL	&4E3				# PC off|CAL	&4E3				# PC off|
	System variables
		A|Address of variable A||0|64472|64472|
		B|Address of variable B||0|64464|64464|
		C|Address of variable C||0|64456|64456|
		D|Address of variable D||0|64448|64448|
		E|Address of variable E||0|64440|64440|
		F|Address of variable F||0|64432|64432|
		G|Address of variable G||0|64424|64424|
		H|Address of variable H||0|64416|64416|
		I|Address of variable I||0|64408|64408|
		J|Address of variable J||0|64400|64400|
		K|Address of variable K||0|64392|64392|
		L|Address of variable L||0|64384|64384|
		M|Address of variable M||0|64376|64376|
		N|Address of variable N||0|64368|64368|
		O|Address of variable O||0|64360|64360|
		P|Address of variable P||0|64352|64352|
		Q|Address of variable Q||0|64344|64344|
		R|Address of variable R||0|64336|64336|
		S|Address of variable S||0|64328|64328|
		T|Address of variable T||0|64320|64320|
		U|Address of variable U||0|64312|64312|
		V|Address of variable V||0|64304|64304|
		W|Address of variable W||0|64296|64296|
		X|Address of variable X||0|64288|64288|
		Y|Address of variable Y||0|64280|64280|
		Z|Address of variable Z||0|64272|64272|
	Memory locations
		Basic start address|Basic start address Version 1=LB 2=HB||0|65282|65281|
		Basic end address|Basic end address Version 1=LB 0=HB||0|65284|65283|
		Merge address|Merge address Version 1=LB 0=HB||0|65286|65285|
		WAIT value|WAIT value Version 1=LB 0=HB||0|65352|65351|
		WAIT state|WAIT state: 2=active, 6=off||0|65299|65299|
		Variable start|Start of selfdefined variables Version 1=LB 0=HB||0|65288|65287|
		Input buffer|Input buffer Version 1=start byte 0=last byte||0|65279|65200|
		Display buffer|Display buffer in ASCII format Version 1=start byte 0=last byte||0|65143|65120|
		Password|Password in ASCII code Version 1=start byte 0=last byte||0|65296|65290|
		Password state|Password state: Bit 5=Password active||0|65300|65300|
		Display memory first 12 chars|Display memory first 12 chars Version 1=chars 1-6, 0=chars 7-12|Every char is 5 bytes long. So a 6 char block is 30 bytes long.|0|12333|12288|
		Display memory second 12 chars|Display memory second 12 chars Version 1=chars 13-18, 0=chars 19-24|Every char is 5 bytes long. So a 6 char block is 30 bytes long. !Attention! These chars are drawn backwards: The first byte is the last column!|0|12377|12392|
		Special symbols 1|Special symbol byte for µt, /l\, SML, STAT, MATRIX, right of PRO and left of CAL|Bit 0=µt\nBit 1=/l\\nBit 2=SML\nBit 3=STAT\nBit 4=MATRIX\nBit 5=right of PRO\nBit 6=left of CAL|0|12604|12348|
		Special symbols 2|Special symbol byte for BUSY, DEF, SHIFT, HYP, PRO, RUN and CAL|Bit 0=BUSY\nBit 1=DEF\nBit 2=SHIFT\nBit 3=HYP\nBit 4=PRO\nBit 5=RUN\nBit 6=CAL|0|12605|12349|
		Special symbols 3|Special symbol byte for E, M, (), RAD, G, DE and PRINT|Bit 0=E\nBit 1=M\nBit 2=()\nBit 3=RAD\nBit 4=G\nBit 5=DE\nBit 6=PRINT|0|12668|12412|
		Port address output|Address for output at 11 pin interface|Bit 0=Pin 11\nBit 1=Pin 10\nBit 2=Pin 5\nBit 3=Pin 8|0|&3A00|&3A00|
		Port address input|Address for input at 11 pin interface|Bit 7=Pin 9|0|&3C00|&3C00|
		B port|B port in and output: Bit 7=Pin 8 at 11 pin IO\nVersion 1=output, 0=input||0|INB|LIP	&5D\nORIM	128\nOUTB|
		F port|F port only output: Bit 2=Pin 4 at 11 pin IO||0|LIP	&5E\nORIM	4\nOUTF|LIP	&5E\nORIM	4\nOUTF|
PC-1475 Macros
	System variables
		A|Address of variable A||0|64152|64152|
		B|Address of variable B||0|64144|64144|
		C|Address of variable C||0|64136|64136|
		D|Address of variable D||0|64128|64128|
		E|Address of variable E||0|64120|64120|
		F|Address of variable F||0|64112|64112|
		G|Address of variable G||0|64104|64104|
		H|Address of variable H||0|64096|64096|
		I|Address of variable I||0|64088|64088|
		J|Address of variable J||0|64080|64080|
		K|Address of variable K||0|64072|64072|
		L|Address of variable L||0|64064|64064|
		M|Address of variable M||0|64056|64056|
		N|Address of variable N||0|64048|64048|
		O|Address of variable O||0|64040|64040|
		P|Address of variable P||0|64032|64032|
		Q|Address of variable Q||0|64024|64024|
		R|Address of variable R||0|64016|64016|
		S|Address of variable S||0|64008|64008|
		T|Address of variable T||0|64000|64000|
		U|Address of variable U||0|63992|63992|
		V|Address of variable V||0|63984|63984|
		W|Address of variable W||0|63976|63976|
		X|Address of variable X||0|63968|63968|
		Y|Address of variable Y||0|63960|63960|
		Z|Address of variable Z||0|63952|63952|
	Memory locations
		Basic start address|Basic start address Version 1=LB 2=HB||0|&FFD8|&FFD7|
		Basic end address|Basic end address Version 1=LB 0=HB||0|&FFDA|&FFD9|
		Merge address|Merge address Version 1=LB 0=HB||0|&FFDC|&FFDB|
		WAIT value|WAIT value Version 1=LB 0=HB||0|&2A7F|&2A7E|
		Character at cursor|ASCII code of character below the cursor||0|&FF02|&FF02|
		Address of cursor|Address of cursors in display buffer Version 1=LB 0=HB||0|&FF01|&FF00|
		Variable start|Start of selfdefined variables Version 1=LB 0=HB||0|&FFDE|&FFDD|
		RAM card start address|RAM card start address Version 1=LB 0=HB||0|&FFF7|&FFF6|
		RAM card end address|RAM card end address Version 1=LB 0=HB||0|&FFF9|&FFF8|
		Reserve mode|Reserve mode area Version 1=start byte 0=last byte||0|&FFCF|&FF40|
		Reserve mode length|Reserve mode length||0|&FF3F|&FF3F|
		Input buffer|Input buffer Version 1=start byte 0=last byte||0|&FD6F|&FD20|
		Display buffer|Display buffer in ASCII format Version 1=start byte 0=last byte||0|&FEAF|&FE80|
		String buffer|String buffer Version 1=start byte 0=last byte||0|&FCDF|&FC90|
		Display memory first line|Display memory first line Version 1=character 1-12, 0=character 13-24|Every char is 5 bytes long. So a 12 char block is 60 bytes long.|0|10752|10240|
		Display memory second line|Display memory second line Version 1=character 1-12, 0=character 13-24|Every char is 5 bytes long. So a 12 char block is 60 bytes long.|0|10816|10304|
		Special symbols 1|Special symbol byte for BATT, (), HYP, RSV, PRO, RUN and CAL|Bit 0=BATT\nBit 1=()\nBit 2=HYP\nBit 3=RSV\nBit 4=PRO\nBit 5=RUN\nBit 6=CAL|0|10556|10300|
		Special symbols 2|Special symbol byte for BUSY, SHIFT, SHIFT and DBL|Bit 0=BUSY\nBit 1=DEF\nBit 2=SHIFT\nBit 3=DBL|0|10557|10301|
		Special symbols 3|Special symbol byte for E, M, RAD, G, MATRIX, STAT and PRINT|Bit 0=E\nBit 1=M\nBit 2=RAD\nBit 3=G\nBit 4=MATRIX\nBit 5=STAT, Bit 6=PRINT|0|10620|10364|
		Special symbols 4|Special symbol byte for japan. symbols, SML and DE|Bit 0=µt\nBit 1=/l\\nBit 2=SML\nBit 3=DE|0|10621|10365|
		Port address seriell|saddress für output am 15-poligen Interface|Bit 0=Pin 4\nBit 1=Pin 14\nBit 2=Pin 11|0|&3800|&3800|
		Port address Drucker|saddress für output am 11-poligen Interface|Bit 0=Pin 11\nBit 1=Pin 10\nBit 2=Pin 5\nBit 3=Pin 8|0|&3A00|&3A00|
		B port|B port output: Bit 4=Pin 9 at printer, Bit 5=Pin 3, Bit 6=Pin 5 and Bit 7=Pin 8 am serial\nVersion 1=output, 0=input|input: Bit 3=Pin 8 and Bit 4=Pin 9 at printer, Bit 5=Pin 3, Bit 6=Pin 5 and Bit 7=Pin 8 at serial|0|INB|LIP	&5D\nORIM	bit\nOUTB|
		F port|F port only output: Bit 1=Pin 2 at serial||0|LIP	&5E\nORIM	2\nOUTF|LIP	&5E\nORIM	2\nOUTF|
