; SIO Driver

; Usage: 
; - Send a char 4800 baud: CALL 32900,c  (c=char)
; - Send a char 4800 baud: CALL 32905  (char is at 62599)
; - Receive a char 4800 baud: CALL 32910  (char is at 62599)
; - Send BASIC memory as binary data: CALL 32915
; - Receive BASIC memory as binary data: CALL 32920
; - Send VAR memory as binary data: CALL 32925
; - Receive VAR memory as binary data: CALL 32930

; OUTCOMMENT THE FOLLOWING LINES WHEN USING PC-1360!

; FOR 1403H

.ORG	32900
; OS Adressen
.equ	DATEN	32899
.equ	BASSTART	65281	; low byte, high byte is at +1
.equ	BASENDE	65283	; low byte, high byte is at +1
.equ	VARSTART	65287	; low byte, high byte is at +1
.equ	VARENDE_H	0xFB
.equ	VARENDE_L	0x0F
.equ	STACKPTR	88
; HW Adressen und Masken
.equ	GATEADR	0x3A00
.equ	GATEVAL	0x04
.equ	INMASK	0x80	; INB
.equ	OUTMASK	0x04	; OUTF


; REMOVE FOLLOWING COMMENTS FOR SWITCH TO PC-1360:

; FOR 1360:

;.ORG	32900
; OS Adressen
;.equ	DATEN	32899
;.equ	BASSTART	0xFFD7	; low byte, high byte is at FFD8
;.equ	BASENDE	0xFFD9	; low byte, high byte is at FFDA
;.equ	VARSTART	0xFFDD
;.equ	VARENDE_H	0xF9
;.equ	VARENDE_L	0xCF
;.equ	STACKPTR	86
; HW Adressen und Masken
;.equ	GATEADR	0x3800
;.equ	GATEVAL	0x04
;.equ	INMASK	0x20	; INB
;.equ	OUTMASK	0x02	; OUTF


	CALL	SEND	; +0
	JRP	EXIT
	CALL	SENDSTD	; +5
	JRP	EXIT
	CALL	RECEIVE	; +10
	JRP	EXIT
	CALL	SENDBAS	; +15
	JRP	EXIT
	CALL	RECEIVEBAS	; +20
	JRP	EXIT
	CALL	SENDVAR	; +25
	JRP	EXIT
	CALL	RECEIVEVAR	; +30

EXIT:
	RC
	LIA	STACKPTR
	STR
	RTN


SEND:
	ix
	ixl
	CALL	sio_send	; Call procedure sio_send

	RTN

SENDSTD:
	LIDP	DATEN
	LDD
	CALL	sio_send	; Call procedure sio_send

	RTN

RECEIVE:
	CALL	sio_get	; Call procedure sio_get
	LIDP	DATEN	; Store result in daten
	STD

	RTN

SREG:
.DW	0,0,0,0

SENDBAS:
	LP	0
	LIDP	SREG
	LII	7
	EXWD

	LP	5
	LIDP	BASSTART+1
	LDD
	EXAM
	LP	4
	LIDP	BASSTART
	LDD
	EXAM

	LP	7
	LIDP	BASENDE+1
	LDD
	EXAM
	LP	6
	LIDP	BASENDE
	LDD
	EXAM

  SENDLOOP:
	IXL
	CALL	sio_send
	LP	5
	LDM
	EXAB
	LP	4
	LDM

	LP	5
	LDM
	LP	7
	CPMA
	JRNZM	SENDLOOP
	LP	4
	LDM
	LP	6
	CPMA
	JRNZM	SENDLOOP

	LP	0
	LIDP	SREG
	LII	7
	MVWD

	RTN

SENDVAR:
	LP	0
	LIDP	SREG
	LII	7
	EXWD

	LP	5
	LIDP	VARSTART+1
	LDD
	EXAM
	LDM
	CALL	sio_send
	LP	4
	LIDP	VARSTART
	LDD
	EXAM
	LDM
	CALL	sio_send

	LP	7
	LIA	VARENDE_H
	EXAM
	LP	6
	LIA	VARENDE_L
	EXAM

	DX
	JRM	SENDLOOP


RECEIVEBAS:
	LP	0
	LIDP	SREG
	LII	7
	EXWD

	LP	7
	LIDP	BASSTART+1
	LDD
	EXAM
	LP	6
	LIDP	BASSTART
	LDD
	EXAM

  RECVLOOP:
	CALL	sio_get
	IYS
	CPIA	255
	JRNZM	RECVLOOP
	
	LP	7
	LIDP	BASENDE+1
	LDM
	STD
	LP	6
	LIDP	BASENDE
	LDM
	STD

	LP	0
	LIDP	SREG
	LII	7
	MVWD

	RTN


RECEIVEVAR:
	LP	0
	LIDP	SREG
	LII	7
	EXWD

	CALL	sio_get
	LIDP	VARSTART+1
	STD
	LP	7
	EXAM
	CALL	sio_get
	LIDP	VARSTART
	STD
	LP	6
	EXAM
	DY

	LP	5
	LIA	VARENDE_H
	EXAM
	LP	4
	LIA	VARENDE_L
	EXAM

  VRLOOP:
	CALL	sio_get
	IYS

	LP	5
	LDM
	LP	7
	CPMA
	JRNZM	VRLOOP
	LP	4
	LDM
	LP	6
	CPMA
	JRNZM	VRLOOP

	LP	0
	LIDP	SREG
	LII	7
	MVWD

	RTN


sio_send:	; Procedure
	LIP	0x5E
	ANIM	0
	ADIM	OUTMASK	; 4
	OUTF		; 3
	LII	8	; 4
	WAIT	21	; 27
	ss1:
	ANIM	0	; 4
	SR		; 2
	JRCP	3	;4/7
	ADIM	OUTMASK	; 4
	OUTF		; 3
	DECI		; 4
	WAIT	21	; 27
	JRNZM	ss1	; 7
	ANIM	0
	OUTF

	RTN


sio_get:	; Procedure
	LIA	GATEVAL	; Load constant 
	LIDP	GATEADR	; Store result in cts
	STD

	
	INB			; 2
	ANIA	INMASK  ; 4
	JRZM	4	;4/7
	LIB		0	; 4
	LP		3	; 2
	LIA		1	; 4
	LII		8	; 4
	WAIT	54+10-6-24
	sg1:
	PUSH		; 3
	INB			; 2
	ANIA	INMASK  ; 4
	POP			; 2
	JRNZP	2	;4/7
	ORMA		; 4
	RC			; 2
	SL			; 2
	WAIT	56-6-34-6
	ANID	0	; 6
	DECI		; 4
	JRNZM	sg1	; 7

	RA		; Load 0
	LIDP	GATEADR	; Store result in cts
	STD

	EXAB		; Load variable b
	RTN		; Return

