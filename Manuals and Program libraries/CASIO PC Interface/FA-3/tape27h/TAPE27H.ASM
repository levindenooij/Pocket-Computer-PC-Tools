; Tape <=> serial interface for the Casio FX-700P programmable calculator,
; supports an optional parallel port expansion.
; Serial printing and EEPROM support removed.
;
; Xtal frequency 9.8304MHz
;
; Serial port settings: 9600bps, no parity, Xon/Xoff flow control
;
; Data format:
; The 12-bit data word is split in two 6-bit chunks, then a fixed value
; of 0x30 is added to each to make them printable characters. The least
; significant character is transferred first.
; Characters outside the 0x30-0x6F range are allowed between a pair of valid
; characters. They can be used to synchronize the data stream (to mark the
; least significant character of the pair).

.include "2313def.inc"

; port B
.equ	DATA4		=7
.equ	DATA3		=6
.equ	DATA2		=5
.equ	DATA1		=4
.equ	OP		=3
.equ	CE1		=2
.equ	CE2		=1
.equ	KEY		=0

; port D
.equ	LED		=6
.equ	SDA		=5
.equ	SCL		=4
.equ	CK1		=3	;INT1
.equ	CK2		=2	;INT0
.equ	TXD		=1
.equ	RXD		=0

; Optional parallel printer port using I2C bus expanders PCF8574, data
; port on address $42, control and status port on address $40.
; parallel port control and status bits assignments:
.equ	PAR_ACK		=0	;status
.equ	PAR_BUSY	=1	;status
.equ	PAR_PE		=2	;status
.equ	PAR_SLCT	=3	;status
.equ	PAR_SLCTIN	=4	;control
.equ	PAR_INIT	=5	;control
.equ	PAR_ERROR	=6	;status
.equ	PAR_STROBE	=7	;control


; R0 is reserved for the LPM instruction,
; also occasionally as a temporary variable

.def	Const0x80	=r8	;constant, fixed value 0x80

.def	FlowContrChar	=r9	;sent or received flow control character
.def	SavedCounter	=r10	;counter of saved data words
.def	PrnData		=r11	;data printed
.def	PrevPrnData	=r12	;previous printed data

; 16 bits transferred between the calculator and the serial port
.def	DataWordLow	=r13
.def	DataWordHigh	=r14

; variables used exclusively in the IRQ1 interrupt handle routine
.def	IntSregSave	=r15	;stores SREG
.def	IntTemp		=r16	;temporary variable 

.def	Flags		=r17	;interface state
; meaning of individual bits:
.equ	TAPE_BUSY	=7	;SAVE mode: bit set after receiving data
				;from the calculator, cleared after sending
				;them through the serial port
				;LOAD mode: bit cleared when data was
				;retrieved from the buffer,
				;set after sending to the calculator,
				;bit numbers of TAPE_BUSY and DATA4
				;intentionally matched
.equ	FORCE_BUSY	=6	;set when Timer1 counting, forces the tape
				;interface to respond "not ready",
				;bit number selected intentionally
.equ	LOAD		=5	;set when LOAD, clr when SAVE
.equ	DATA_MODE	=4	;set when data transfer
.equ	STATUS_MODE	=3	;set when status read
.equ	TAPE_ACTIVE	=2	;set when tape communication
.equ	PRN_REQUEST	=1	;set when valid data to be printed in PrnData

.def	WrPortB		=r18	;data sent to port B
.def	Counter		=r20	;local general purpose counter
.def	Temp1		=r21	;local temporary variable
.def	Skipped		=r22	;counter of skipped data words after the
				;"open for read" command
.def	DataByte	=r23	;data transferred through the I2C bus,
				;data sent to the calculator's bus

; X register is the write pointer to the data transfer buffer
; Y register is the read pointer to the data transfer buffer
; Z register is used to walk through the bus_handle code

.equ	LoadBuffer	=0x60	;circular buffer in the RAM
.equ	LOADBUFSIZE	=0x60	;size of the buffer

; stack area: $C0 to $DF

; The flow control character codes
.equ	XON		=0x11
.equ	XOFF		=0x13


.org	0x0000

		rjmp	start		;Reset Handle
		reti			;IRQ0 Handle
		ijmp			;IRQ1 Handle
		reti			;Timer1 capture Handle
		reti			;Timer1 compare Handle
		reti			;Timer1 overflow Handle
		reti			;Timer0 overflow Handle
		reti			;UART RX Complete Handle
		reti			;UDR Empty Handle
		reti			;UART TX Complete Handle
		reti			;Analog Comparator Handle


; Casio to ASCII character conversion table, codes that cannot be printed
; as a single character are marked with ~,
; table intentionally placed in first 256 words of the program memory,
; so it can be addressed with only 8 bits of the Z register
; WARNING: the IAR assembler doesn't seem to like certain characters in a
; string. The semicolon must be coded as 0x3B, otherwise it will be treated
; as a comment. The colon after this semicolon must be coded as 0x3A,
; otherwise the assembler incorrectly calculates the address of the label
; bus_handle and all following.
casio2ascii:
.db		" +-*/^!", 0x22, "#$>~=~<~"
.db		"0123456789.~)(~E"
.db		"ABCDEFGHIJKLMNOP"
.db		"QRSTUVWXYZ      "
.db		"abcdefghijklmnop"
.db		"qrstuvwxyz  ?,", 0x3B, 0x3A
.db		"o ' @x: <   u v>"
.db		"%Y [&_'.]       "

casio_strings:
.db		0x0D, 0x0A
.db		">="
.db		"<="
.db		"<>"
.db		"PI"
.db		"E-"
.db		"= "


; IRQ1 handle routine, used to handle the calculator's bus traffic,
; executed piece by piece at every CK1 pulse,
; code intentionally placed in first 256 words of the program memory,
; so it can be addressed with only 8 bits of the Z register
bus_handle:	in	IntSregSave,SREG
		sbis	PINB,CE2
		rjmp	tapei_access
		sbis	PINB,CE1
		rjmp	bus_handle_1	;RAM access
		sbis	PINB,OP
		rjmp	prni_access
bus_handle_1:	ldi	ZL,low (bus_handle)
		out	SREG,IntSregSave
		reti


; tape interface access

tapei_access:	sbis	PINB,OP
		rjmp	tapei_cmd

; OP=1
		sbrs	Flags,TAPE_ACTIVE
		rjmp	tapei_end_1
		sbrs	Flags,DATA_MODE
		rjmp	tapei_stat
; data transfer
		sbrs	Flags,LOAD
		rjmp	tapei_save

; LOAD mode
		out	PORTB,WrPortB
		sbi	DDRB,DATA4
		ldi	IntTemp,16		;bit counter
		ldi	ZL,low (tapei_load_4)
		out	SREG,IntSregSave
		reti

tapei_load_2:	out	PORTB,WrPortB
		in	IntSregSave,SREG
		sbic	PINB,CE2
		rjmp	tapei_end_1
		ldi	ZL,low (tapei_load_4)
		out	SREG,IntSregSave
		reti

tapei_load_4:	in	IntSregSave,SREG
		sbic	PINB,CE2
		rjmp	tapei_end_1
		lsr	DataWordHigh
		ror	DataWordLow
		ldi	WrPortB,((1<<KEY)<<1)
		ror	WrPortB
		dec	IntTemp
		brne	tapei_load_5
		sbr	Flags,(1<<TAPE_BUSY)
		rjmp	tapei_end_1

tapei_load_5:	ldi	ZL,low (tapei_load_2)
		out	SREG,IntSregSave
		reti

; SAVE mode
tapei_save:	mov	DataWordHigh,Const0x80	;shifted right bit=1 used
		clr	DataWordLow		;as bit counter
		rjmp	tapei_save_3

tapei_save_2:	in	IntSregSave,SREG
		sbic	PINB,CE2
		rjmp	tapei_end_2
tapei_save_3:	ldi	ZL,low (tapei_save_4)
		out	SREG,IntSregSave
		reti

tapei_save_4:	in	IntSregSave,SREG
		sbic	PINB,CE2
		rjmp	tapei_end_2
		lsr	DataWordHigh
		ror	DataWordLow
		sbis	PINB,DATA4
		or	DataWordHigh,Const0x80
		brcc	tapei_save_5
		sbr	Flags,(1<<TAPE_BUSY)
		rjmp	tapei_end_2

tapei_save_5:	ldi	ZL,low (tapei_save_2)
		out	SREG,IntSregSave
		reti

; read status
tapei_stat:	sbrs	Flags,STATUS_MODE
		rjmp	tapei_end_2
		mov	IntTemp,Flags
		lsl	IntTemp
		or	IntTemp,Flags		;bit7 = TAPE_BUSY | FORCE_BUSY
		andi	IntTemp,(1<<TAPE_BUSY)	;copy the TAPE_BUSY flag to
						;the IntTemp,DATA4
		ori	IntTemp,(1<<KEY)
		out	PORTB,IntTemp
		sbi	DDRB,DATA4
		ldi	ZL,low (tapei_stat_1)
		out	SREG,IntSregSave
		reti

tapei_stat_1:	in	IntSregSave,SREG
tapei_end_1:	cbi	DDRB,DATA4
		cbi	PORTB,DATA4		;disable pull-up on DATA4 pin
		cbr	Flags,(1<<DATA_MODE)+(1<<STATUS_MODE)
; wait for CE2=1
tapei_end_2:	sbic	PINB,CE2
		rjmp	bus_handle_1
		ldi	ZL,low (tapei_end_3)
		out	SREG,IntSregSave
		reti

tapei_end_3:	in	IntSregSave,SREG
		rjmp	tapei_end_2


; OP=0, read command code to IntTemp
tapei_cmd:	ldi	IntTemp,0x20		;received byte, shifted right
						;bit=1 used to count bits
		ldi	ZL,low (tapei_cmd_4)
		out	SREG,IntSregSave
		reti

tapei_cmd_2:	in	IntSregSave,SREG
		sbic	PINB,CE2
		rjmp	tapei_end_2
		ldi	ZL,low (tapei_cmd_4)
		out	SREG,IntSregSave
		reti

tapei_cmd_4:	in	IntSregSave,SREG
		sbic	PINB,CE2
		rjmp	tapei_end_2
		lsr	IntTemp
		sbis	PINB,DATA4
		ori	IntTemp,0x20
		brcs	tapei_eval
		ldi	ZL,low (tapei_cmd_2)
		out	SREG,IntSregSave
		reti

; command evaluate, command code in IntTemp on bits 5-0
tapei_eval:	cpi	IntTemp,0x05		;open for write ?
		brne	tapei_eval_1
		ldi	Flags,(1<<TAPE_ACTIVE)
		rjmp	tapei_end_2
tapei_eval_1:	cpi	IntTemp,0x06		;open for read ?
		brne	tapei_eval_2
		ldi	Flags,(1<<TAPE_BUSY)+(1<<LOAD)+(1<<TAPE_ACTIVE)
		rjmp	tapei_end_2
tapei_eval_2:	cpi	IntTemp,0x08		;read status ?
		brne	tapei_eval_3
		sbr	Flags,(1<<STATUS_MODE)
		cbr	Flags,(1<<DATA_MODE)
		rjmp	tapei_end_2
tapei_eval_3:	cpi	IntTemp,0x12		;transfer data ?
		brne	tapei_eval_4
		sbr	Flags,(1<<DATA_MODE)
		cbr	Flags,(1<<STATUS_MODE)
		rjmp	tapei_end_2
tapei_eval_4:	cpi	IntTemp,0x00		;reset ?
		breq	tapei_eval_5
		cpi	IntTemp,0x0F		;close ?
		brne	tapei_eval_6
tapei_eval_5:	clr	Flags
		rjmp	tapei_end_2
tapei_eval_6:	cpi	IntTemp,0x0C		;wait ?
		brne	tapei_end_1		;unknown command
		sbr	Flags,(1<<FORCE_BUSY)+(1<<STATUS_MODE)
		cbr	Flags,(1<<DATA_MODE)
		ldi	IntTemp,0x05		;CK/1024
		out	TCCR1B,IntTemp		;start the Timer1
		rjmp	tapei_end_2


; printer interface access

prni_access:	ldi	IntTemp,3
prni_access_1:	ldi	ZL,low (prni_access_2)
		out	SREG,IntSregSave
		reti

prni_access_2:	in	IntSregSave,SREG
		dec	IntTemp
		brne	prni_access_1
		in	IntTemp,PINB
		andi	IntTemp,0xF8	;isolate the Data and OP bits
		cpi	IntTemp,0x08	;control command $F ?
		brne	prni_access_3
		sbr	Flags,(1<<PRN_REQUEST)
		rjmp	bus_handle_1
; PRN_REQUEST should be cleared by the printer interface handle routine,
; this is an "emergency exit" when printing timed out due to some printer
; error (not connected, offline, out of paper...)
prni_access_3:	cpi	IntTemp,0x18	;control command $E ?
		brne	prni_access_4
		cbr	Flags,(1<<PRN_REQUEST)
prni_access_4:	rjmp	bus_handle_1


; Slave mode port setting
slave_mode:	ldi	Temp1,0b00000000
		out	DDRB,Temp1
		ldi	Temp1,(1<<KEY)	;pull-up on the KEY input
		out	PORTB,Temp1
		ldi	Temp1,(1<<TXD)
		out	DDRD,Temp1
		out	PORTD,Temp1
		ret

; Master mode port setting
master_mode:	ldi	Temp1,0b11111111
		out	PORTB,Temp1
		ldi	Temp1,0b11111110
		out	DDRB,Temp1
		ldi	Temp1,(1<<TXD)+(1<<CK1)+(1<<CK2)
		out	PORTD,Temp1
		out	DDRD,Temp1
		ret


start:		cli
		ldi	Temp1,0xDF
		out	SPL,Temp1
		rcall	slave_mode

		clr	Flags
		ldi	Temp1,0x80
		mov	Const0x80,Temp1
		ldi	XL,low (LoadBuffer)
		ldi	XH,high (LoadBuffer)
		ldi	YL,low (LoadBuffer)
		ldi	YH,high (LoadBuffer)
		ldi	ZL,low (bus_handle)
		ldi	ZH,high (bus_handle)

; timer0 used to count the SCL frequency
		ldi	Temp1,0x02	;CK/8
		out	TCCR0,Temp1

; UART parameters: 9600 baud, 8-bit, interrupts disabled
		ldi	Temp1,63	;or 64 when fck=10MHz
		out	UBRR,Temp1
		ldi	Temp1,0x18	;Transmitter & Receiver Enable
		out	UCR,Temp1

; falling edge of INT1 should generate an interrupt request
		ldi	Temp1,0x0A
		out	MCUCR,Temp1
		ldi	Temp1,0x80	;external interrupt 1 enable
		out	GIMSK,Temp1
		sei

		rcall	i2c_reset

main_loop:	sbrc	Flags,TAPE_ACTIVE	;tape access request?
		rcall	tape
		sbrc	Flags,PRN_REQUEST	;printer access request?
		rcall	print
		rjmp	main_loop


; printer interface handle
print:
; take control over the bus
		rcall	wait_50us
		cli
		rcall	master_mode
; reset command
		ldi	DataByte,0x0C
		rcall	bus_nibble
		rcall	bus_nibble
		rcall	bus_nibble
		ldi	DataByte,0x00	;Data=0000, OP=0, CE1=0
		rcall	bus_nibble
; read command followed by $0132 address
		ldi	DataByte,0x0C
		rcall	bus_nibble
		rcall	bus_nibble
		rcall	bus_nibble
		rcall	bus_nibble
		ldi	DataByte,0x00	;RAM read, OP=0, CE1=0
		rcall	bus_nibble
		ldi	DataByte,0x20
		rcall	bus_nibble
		ldi	DataByte,0x30
		rcall	bus_nibble
		ldi	DataByte,0x10
		rcall	bus_nibble
		ldi	DataByte,0x00
		rcall	bus_nibble
		ldi	DataByte,0x0C
		rcall	bus_nibble
		rcall	bus_nibble
		rcall	bus_nibble
; read data from calculator's RAM
		mov	PrevPrnData,PrnData
		ldi	Temp1,0b00001110
		out	DDRB,Temp1
		ldi	DataByte,0xF8	;OP=1, CE1=0
		rcall	bus_nibble
		mov	PrnData,Temp1
		swap	PrnData
		rcall	bus_nibble
		or	PrnData,Temp1
		ldi	Temp1,0b11111110
		out	DDRB,Temp1
		ldi	DataByte,0x0C
		rcall	bus_nibble
		rcall	bus_nibble
		rcall	bus_nibble
; send data through the parallel port
		mov	Temp1,PrnData
		cp	Temp1,PrevPrnData
		breq	print_1
		cpi	Temp1,0xFE
		breq	print_1
; releasing the bus allows the calculator to terminate the printing
		rcall	slave_mode
		sei
		mov	Temp1,PrnData
		rcall	print_ascii
		sbrs	Flags,PRN_REQUEST
		ret
		cli
		rcall	master_mode
; let the calculator know that the bus will be released
print_1:	ldi	DataByte,0xE4	;OP=0
		rcall	bus_nibble
		ldi	DataByte,0xEC	;OP=1
		rcall	bus_nibble
; release the bus
		rcall	slave_mode
		sei
		cbr	Flags,(1<<PRN_REQUEST)
		ret


; convert the Temp1 byte to ASCII and print through the parallel port,
; returns Carry set when the parallel interface is not available,
; Counter misused as a temporary variable
print_ascii:	ldi	Counter,casio_strings
		mov	r0,Counter
		cpi	Temp1,0xFF
		breq	print_ascii_2
		inc	r0
		cpi	Temp1,0x0B
		breq	print_ascii_2
		inc	r0
		cpi	Temp1,0x0D
		breq	print_ascii_2
		inc	r0
		cpi	Temp1,0x0F
		breq	print_ascii_2
		inc	r0
		cpi	Temp1,0x1B
		breq	print_ascii_2
		inc	r0
		cpi	Temp1,0x1E
		breq	print_ascii_2
		inc	r0
		cpi	Temp1,0x80
		breq	print_ascii_2
		brcc	print_ascii_1
		subi	Temp1,-2*casio2ascii
		mov	r0,Temp1
		rcall	transmit_ptr
print_ascii_1:	ret

print_ascii_2:	lsl	r0
		push	r0
		rcall	transmit_ptr
		pop	r0
		brcs	print_ascii_3
		inc	r0
		rcall	transmit_ptr
print_ascii_3:	ret

; transmit the byte pointed to by R0 from the first page of the program memory 
transmit_ptr:	cli
		mov	Temp1,ZL	;save register Z low
		mov	ZL,r0
		lpm
		mov	ZL,Temp1	;restore register Z low
		sei
; transmit the R0 byte through the parallel port,
; returns Carry set when the parallel interface is not available
; PRN_REQUEST clear when timed up by the calculator
transmit_par:
; prepare data on the control/status port
		rcall	i2c_start
		ldi	DataByte,0x40
		sec
		rcall	i2c_data
		brcs	transmit_par_3
		ldi	DataByte,0b11101111	;SLCTIN low
		sec
		rcall	i2c_data
		brcs	transmit_par_3
		rcall	i2c_stop
; prepare data on the data port
		rcall	i2c_start
		ldi	DataByte,0x42
		sec
		rcall	i2c_data
		brcs	transmit_par_3
		mov	DataByte,r0
		sec
		rcall	i2c_data
		brcs	transmit_par_3
		rcall	i2c_stop
; wait for Busy low
		rcall	i2c_start
		ldi	DataByte,0x41
		sec
		rcall	i2c_data
		brcs	transmit_par_3
transmit_par_1:	ldi	DataByte,0xFF
		clc
		rcall	i2c_data
		sbrs	DataByte,PAR_BUSY
		rjmp	transmit_par_2		;BUSY low detected
		sbrc	Flags,PRN_REQUEST
		rjmp	transmit_par_1
; printing timed up by the calculator
		ldi	DataByte,0xFF
		sec
		rcall	i2c_data		;dummy read with NOACK
		rcall	i2c_stop
		clc
		ret
; BUSY low detected
transmit_par_2:	ldi	DataByte,0xFF
		sec
		rcall	i2c_data		;dummy read with NOACK
		rcall	i2c_stop
; send a strobe pulse
		rcall	i2c_start
		ldi	DataByte,0x40
		sec
		rcall	i2c_data
		brcs	transmit_par_3
		ldi	DataByte,0b01101111	;SLCTIN and STROBE low
		sec
		rcall	i2c_data
		brcs	transmit_par_3
		ldi	DataByte,0b11101111	;SLCTIN low, STROBE high
		sec
		rcall	i2c_data
transmit_par_3:	in	r0,SREG
		rcall	i2c_stop
		out	SREG,r0
		ret


; mass storage handle
tape:		rcall	stop_timer1
		clr	FlowContrChar
		sbrs	Flags,LOAD
		rjmp	serial_write

; LOAD mode
serial_read:	mov	XL,YL		;clear the Load buffer
		ldi	Skipped,5
serial_read_1:	clr	Counter		;count of characters shifted to the
					;DataWord 16-bit variable
; read a character from the serial port, if available
serial_read_2:	sbrs	Flags,TAPE_ACTIVE
		ret
		sbis	USR,7		;UART Receive Complete
		rjmp	serial_read_3
		in	Temp1,UDR
; skip any XON/XOFF characters
		cpi	Temp1,XON
		breq	serial_read_2
		cpi	Temp1,XOFF
		breq	serial_read_2
; store the received character to the Load buffer
		st	x+,Temp1
		cpi	XL,LoadBuffer+LOADBUFSIZE
		brcs	serial_read_3
		subi	XL,LOADBUFSIZE		;wrap the write pointer
; calculate how many characters are there in the Load buffer, and emit the
; XON/XOFF character when necessary (and when the transmitter is free)
serial_read_3:	ldi	Temp1,XON
		mov	r0,XL
		sub	r0,YL
		breq	serial_read_5
		brcc	serial_read_4
		ldi	Temp1,LOADBUFSIZE
		add	r0,Temp1
serial_read_4:	ldi	Temp1,LOADBUFSIZE/2	;limit arbitrary chosen
		cp	r0,Temp1
		brcs	serial_read_6
		ldi	Temp1,XOFF
serial_read_5:	sbis	USR,5			;TX Data Register Empty
		rjmp	serial_read_6
		cp	FlowContrChar,Temp1
		breq	serial_read_6		;character already transmitted
		mov	FlowContrChar,Temp1
		out	UDR,Temp1
; delay ?
serial_read_6:	in	Temp1,TCCR1B
		tst	Temp1			;Timer1 counting ?
		breq	serial_read_6a		;branch if not
		in	Temp1,TCNT1L
		in	Temp1,TCNT1H
		tst	Temp1
		brmi	serial_read_2
		rcall	stop_timer1
; is the calculator ready to accept data ?
serial_read_6a:	sbrs	Flags,TAPE_BUSY
		rjmp	serial_read_2		;previous data not sent yet
; is there any character in the Load buffer ?
		cp	XL,YL
		breq	serial_read_2		;Load buffer empty
; fetch the character from the Load buffer
		ld	Temp1,y+
		cpi	YL,LoadBuffer+LOADBUFSIZE
		brcs	serial_read_7
		subi	YL,LOADBUFSIZE		;wrap the read pointer
; subtract the offset and validate the character
serial_read_7:	subi	Temp1,0x30
		brcs	serial_read_1
		cpi	Temp1,0x40
		brcc	serial_read_1
; shift right the 6-bit character into the DataWord variable
		rol	DataWordHigh
		rol	DataWordHigh
		mov	DataWordLow,DataWordHigh
		mov	DataWordHigh,Temp1
		rol	DataWordHigh
		ldi	Temp1,0x07
		or	DataWordLow,Temp1
		inc	Counter
		cpi	Counter,2
		brcs	serial_read_2
; the DataWord 16-bit variable contains now a valid 12-bit word on bits 14...3
; skip first 5 received data words
		tst	Skipped
		breq	serial_read_8
		dec	Skipped
		rjmp	serial_read_1
; prepare data to send to the calculator in DataWordLow, DataWordHigh and
; WrPortB to save time in the IRQ1 handle routine
serial_read_8:	com	DataWordHigh
		com	DataWordLow
		ldi	WrPortB,(1<<KEY)	;pull-up, initial data to
						;be written to port B
		cbr	Flags,(1<<TAPE_BUSY)	;data ready
		rjmp	serial_read_1

; SAVE mode, sending data to the serial port
serial_write:	clr	SavedCounter
serial_write_1:	sbrs	Flags,TAPE_ACTIVE
		ret
; read the XON/XOFF character, if available
		sbis	USR,7			;UART Receive Complete
		rjmp	serial_write_3
		in	Temp1,UDR
		cpi	Temp1,XON
		breq	serial_write_2
		cpi	Temp1,XOFF
		brne	serial_write_3
serial_write_2:	mov	FlowContrChar,Temp1
; transmission allowed ?
serial_write_3:	mov	Temp1,FlowContrChar
		cpi	Temp1,XOFF
		breq	serial_write_1
; any data received from the calculator ?
		sbrs	Flags,TAPE_BUSY
		rjmp	serial_write_1
; transmit the CR/LF sequence every 32 data words
		mov	Temp1,SavedCounter
		andi	Temp1,0x1F
		brne	serial_write_4
		ldi	Temp1,0x0D
		rcall	transmit
		ldi	Temp1,0x0A
		rcall	transmit
; send the least significant character through the serial port
serial_write_4:	mov	Temp1,DataWordLow
		andi	Temp1,0x3F
		subi	Temp1,0xD0		;add offset 0x30
		rcall	transmit
; send the most significant character through the serial port
		mov	Temp1,DataWordLow
		eor	Temp1,DataWordHigh
		andi	Temp1,0xC0
		eor	Temp1,DataWordHigh
		swap	Temp1
		lsr	Temp1
		lsr	Temp1
		subi	Temp1,0xD0		;add offset 0x30
		rcall	transmit
		inc	SavedCounter
		cbr	Flags,(1<<TAPE_BUSY)	;data sent
		rjmp	serial_write_1

; transmit the Temp1 byte through the serial port
transmit:	sbis	USR,5		;TX Data Register Empty
		rjmp	transmit
		out	UDR,Temp1
		ret

; stop the Timer1 and preset it for a 150ms delay
stop_timer1:	cbr	Flags,(1<<FORCE_BUSY)
		ldi	Temp1,0x00
		out	TCCR1B,Temp1
		ldi	Temp1,high (-1500)
		out	TCNT1H,Temp1
		ldi	Temp1,low (-1500)
		out	TCNT1L,Temp1
		ret


; wait ca. 5us
wait_5us:	ldi	Temp1,256-7	;7 * 0.8us = 5.6us
wait_1:		out	TCNT0,Temp1
wait_2:		in	Temp1,TCNT0
		sbrc	Temp1,7
		rjmp	wait_2
		ret

; wait ca. 50us
wait_50us:	ldi	Temp1,256-62	;62 * 0.8us = 50us
		out	TCNT0,Temp1
		rjmp	wait_2


; *** I2C bus routines ***

; send byte DataByte and read ACK to Carry (on input Carry set required),
; or read byte DataByte and set ACK according to Carry (on input DataByte=0xFF
; required)
i2c_data:	ldi	Counter,9	;bit counter
i2c_data_loop:	rol	DataByte
; copy Carry to SDA
		brcc	i2c_data_1
		cbi	DDRD,SDA	;SDA=1
		rjmp	i2c_data_2
i2c_data_1:	sbi	DDRD,SDA	;SDA=0
i2c_data_2:	rcall	wait_5us
; SCL=1
		cbi	DDRD,SCL	;SCL=1
; read SDA to Carry
		sbis	PIND,SDA
		rjmp	i2c_data_3
		sec
		rjmp	i2c_data_4
i2c_data_3:	clc
i2c_data_4:	rcall	wait_5us
; SCL=0
		sbi	DDRD,SCL	;SCL=0
; next bit
		dec	Counter
		brne	i2c_data_loop
		ret

; send start condition
i2c_start:	rcall	wait_5us
		sbi	DDRD,SCL	;SCL=0
		rcall	wait_5us
		cbi	DDRD,SDA	;SDA=1
		rcall	wait_5us
		cbi	DDRD,SCL	;SCL=1
i2c_start_1:	rcall	wait_5us
		sbi	DDRD,SDA	;SDA=0
		rcall	wait_5us
		sbi	DDRD,SCL	;SCL=0
		ret

; send stop condition
i2c_stop:	rcall	wait_5us
		sbi	DDRD,SCL	;SCL=0
		rcall	wait_5us
		sbi	DDRD,SDA	;SDA=0
		rcall	wait_5us
		cbi	DDRD,SCL	;SCL=1
		rcall	wait_5us
		cbi	DDRD,SDA	;SDA=1
		ret

; I2C bus reset
i2c_reset:	cbi	DDRD,SDA	;SDA=1
		rcall	wait_5us
		cbi	DDRD,SCL	;SCL=1
		rcall	wait_5us
		ldi	Counter,9	;up to 9 clock pulses
i2c_reset_1:	sbic	PIND,SDA	;look for SDA high while SCL is high
		rjmp	i2c_reset_2
		rcall	wait_5us
		sbi	DDRD,SCL	;SCL=0
		rcall	wait_5us
		cbi	DDRD,SCL	;SCL=1
		dec	Counter
		brne	i2c_reset_1
i2c_reset_2:	rcall	i2c_start_1
		rcall	i2c_stop
		ret


; *** Calculator bus routines in the master mode ***

; small delays from 0.7us to 1.5us (rcall execution time inclusive)
delay15:	rjmp	delay13
delay13:	rjmp	delay11
delay11:	rjmp	delay09
delay09:	rjmp	delay07
delay07:	ret

; sends DataByte to the bus (DataByte content isn't modified),
; bits 7-4 of the DataByte will be inverted and put to Data4-Data1 lines,
; bit 3 of the DataByte will be put to the OP line,
; bit 2 of the DataByte will be put to the CE1 line,
; or
; reads Data4-Data1 lines to Temp1, DataByte should contain $F8 (pull-ups
; disabled), and bits 7-4 of Port B should be configured as inputs
bus_nibble:	in	Temp1,PORTB
		eor	Temp1,DataByte
		andi	Temp1,0b11110011	;bits OP and CE1
		eor	Temp1,DataByte
		rcall	delay09
		cbi	PORTD,CK2
		out	PORTB,Temp1
		rcall	delay15
		sbi	PORTD,CK2
		rcall	delay13
		eor	Temp1,DataByte
		ori	Temp1,0xF0
		eor	Temp1,DataByte
		cbi	PORTD,CK1
		out	PORTB,Temp1
		rcall	delay15
		sbi	PORTD,CK1
		rcall	delay15
		cbi	PORTD,CK2
		rcall	delay13
		in	Temp1,PINB
		com	Temp1
		andi	Temp1,0xF0
		sbi	PORTD,CK2
		rcall	delay15
		cbi	PORTD,CK1
		rcall	delay15
		sbi	PORTD,CK1
		ret
