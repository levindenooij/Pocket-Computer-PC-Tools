Page 72, 132
Title SHARP-PC-öbertragung - Low Level Routinen
SubTtl Vereinbarungsteil
Name SHARP

COMMENT $

        sharplow.asm

        Low-Level-Routinen zum Laden und Speichern von
        BASIC-Programmen des Sharp PC 1401/1402/1403 auf
        einem IBM-PC-kompatiblen Rechner.

        Literatur:
          c't Heft 5/88, Seite 116ff;
          siehe auch c't Heft 9/88, "ErgÑnzungen und Berichtigungen"

        FÅr das Timing wird der Kanal 2 des Timers (Beep-Timer)
        mi·braucht. Der Timer wird mit 1.193.182 Hz getaktet.

        PIO-Port B (Adresse 61h) Bit 0: 1 = Timer freigegeben
                                        0 = Timer angehalten (Gate)
                                 Bit 1: 1 = Lautsprecher ein
                                        0 = Lautsprecher aus
        PIO-Port C (62h, Port B, 61h bei AT):
                                 Bit 5: 1 = Timer abgelaufen (Ausgang des
                                        0 = Timer lÑuft       Timers)

        Diese Routinen sind als Unterroutinen eines
        C-Programms gedacht.

        Warnung: Das C-Programm mu· mit dem
                 Small Memory Model compiliert werden!
                 Andernfalls gibt es ein Chaos mit den Zeigern
                 und den SEGMENT-Registern...

        Die öbertragung der Daten erfolgt Åber den parallelen Port
        des Rechners. Der Eingang zum Rechner (bei CSAVE) geschieht
        Åber den STROBE-Anschlu· (Pin 1). Dazu mu· das Strobe-Signal
        5V-Pegel haben (open collector), das bedeutet eine 0 in Bit 0
        des Steuerports. Geschrieben (bei CLOAD) wird Åber Data0, das
        ist Bit 0 des Datenports.

        Autor:          Norbert Unterberg
        Assembler:      MASM 5.1 oder TASM 1.0.
                        Das Programm lÑ·t sich wegen der erweiterten
                        PROCedure-Deklarationen nicht mit MASM 5.0
                        Åbersetzen!
        Betriebssystem: MS-DOS 3.3
        Datum:          16.Okt.1988


        11.04.1989      Problem mit einem 386-AT:
                        Einbau des Makros IOWait, um den Bausteinen nach
                        dem Zugriff etwas Zeit zu geben, sich zu erholen
                        ("recovery time").

        19.Jan.1989     Kleine Modifikation in der syncronisiere-Routine
                        ("Einschwingen" der Leitung nicht nîtig)

        05.Jan.1989     Konsequente Gro·-/Kleinschreibung der Bezeichner

        27.Dez.1988     éndern des Nibbletauschs in HoledByte

        16.Dez.1988     EinfÅhrung der Variable Periode_Senden

        04.Dez.1988     Fertigstellung mit Senderoutinen und Cls()

        24.Nov.1988     Umstellung auf die Datenstruktur tHeader

        01.Nov.1988     vorlÑufige Fertigstellung dieser Datei


$


;-----Allgemeine Konstanten-----
TRUE            EQU 1           ; C-Konvention
FALSE           EQU 0
nMess           EQU 20          ; auszuzÑhlende Perioden bei Sync
Periode_4000    EQU 298         ; = eine Periode bei 4000 Hz (1193182/4000)
Vers            EQU "1.01"      ; Versionsnummer

S_LEER          EQU 000h        ; keine Datei geladen
S_BASIC         EQU 070h        ; Dateityp Basic
S_BASIC_P       EQU 071h        ;          Basic mit Password
S_DATEN         EQU 074h        ;          Datendatei
S_BINAER        EQU 076h        ;          BinÑrdatei
S_DUMP          EQU 0ffh        ; Dump - Lade alles zu Testen

;----- Fehler, die beim Laden auftreten kînnen
tWaitBit        EQU 1           ; Timeout bei GetBit (öbertragung abgebrochen)
tNoSignal       EQU 2           ;    "    kein Signal auf der Leitung
tSyncError      EQU 3           ;    "    wÑhrend Syncronisation
f_Header_id     EQU 4           ; falsche Header-ID-Bytes
f_Header_chk    EQU 5           ; fehlerhafte Header-PrÅfsumme
f_Data_chk      EQU 6           ; fehlerhafte Daten-PrÅfsumme
tEndOfData      EQU 7           ; öbertragungsende
f_NoHeader      EQU 8           ; Header kommt nicht

;----- Konstanten fÅr den Timer
TimerBit        EQU 5           ; Bit 5 von PIO: Timerausgang (0 = Aktiv)
TimerMask       EQU 1 shl TimerBit
DataMask        EQU 00000001b   ; Bit 0 vom Steuerport: Strobe, CSAVE vom Sharp
DataBit         EQU 00000001b   ; Bit 0 vom Datenport: Ausgang, CLOAD vom Sharp

TimerValue      EQU 0ffffh

TimerNum        EQU 2 shl 6     ; Timer 2
TimerMode1      EQU 1 shl 1     ; Monoflop
TimerMode3      EQU 3 shl 1     ; Frequenzteiler mit symm. Ausgang
TimerL          EQU 1 shl 4     ; low byte lesen
TimerH          EQU 1 shl 5     ; high byte lesen
TimerLH         EQU TimerL + TimerH
TimerInit1      EQU TimerNum + TimerMode1 + TimerLH
TimerInit3      EQU TimerNum + TimerMode3 + TimerLH
TimerRead       EQU TimerInit1
TimerStop       EQU TimerNum + TimerMode1

;-----Port-Adressen-------------
PIO_B           EQU 61h         ; Schreiben: Timer-Steuerung
Timer_Read_AT   EQU 61h         ; Lesen: Timer-Ausgang beim AT
Timer_Read_PC   EQU 62h         ; Lesen: Timer-Ausgang beim PC
Timer_2         EQU 42h         ; Daten Timer 2
Timer_Control   EQU 43h         ; Steuerwort Timer

;-----Datenstrukturen-----------

; Warnung: Diese Struktur ist das direkte Abbild von sHeader
;          in SHARP.C!!!

sHeader         Struc
Sharpname       DB 8 dup (?)
Password        DB 8 dup (?)
Buffer          DW 0
Laenge          DW 0
Dateityp        DB ?
sHeader         ENDS


                .Model Small, C ; Small Model, Sprache C

Public  InitSharp, HoleSharp, TimeoutReason, SendeSharp, Cls


IOWait          MACRO
;; Sorge fÅr genÅgend Zeit, damit sich der Portbaustein
;; zwischen zwei aufeinanderfolgenden Zugriffen erholen
;; kann.
                jmp SHORT $+2
                ENDM


ReadTimer       MACRO
;; Liest den Timer 2 aus.

                mov al, TimerStop       ; ZÑhlerstand festhalten
                out Timer_Control, al
                IOWait
                in al, Timer_2          ; Low Byte lesen
                mov ah, al
                IOWait
                in al, Timer_2          ; High Byte lesen
                IOWait
                xchg ah, al
                ENDM


LoadTimer       MACRO Mode, Value
;; LÑdt den Timer mit dem angegebenen Wert

                mov al, Mode
                out Timer_Control, al
                mov ax, Value
                IOWait
                out Timer_2, al
                mov al, ah
                IOWait
                out Timer_2, al
                IOWait
                ENDM


StartTimer      MACRO
;; Gibt das Gate des Timers frei

                in al, PIO_B
                IOWait
                or al, 1                ; Bit 0: Gate des Timers 2
                out PIO_B, al
                IOWait
                ENDM


StopTimer       MACRO
;; Verriegelt das Gate des Timers

                in al, PIO_B
                IOWait
                and al, not 1          ; Bit 0: Gate des Timers 2
                out PIO_B, al
                IOWait
                ENDM


SubTtl Datenbereich
;******************************************************

                .Data

LPT_Base        DW ?            ; Basisadresse der parallelen Schnittstelle
Timer_Read      DW ?            ; Adresse des PIO-Ports mit dem Timer-Ausgang
BiosSegment     DW 40h          ; SEGMENTadresse des Bios
Periode         DW ?            ; eine mittlere Periode des Eingangssignals
Periode_Senden  DW Periode_4000 ; wird nach erfolgreichem Sync durch
                                ; gemessen Wert ersetzt
Periode_7       DW ?            ; mittlere Zeit fÅr 7 Perioden
TimeoutReason   DW 0            ; Grund fÅr TimeOut
Checksum        DB 0            ; PrÅfsumme
Anzahl          DW 0            ; Anzahl gelesener Datenbytes
Stopbits        DW 3            ; Anzahl der Stopbits beim Schreiben
PrevByte        DB ?            ; vorheriges Byte in HoleBasic


Bios_Data       SEGMENT AT 40h
                ORG 08h
ParallelPort    DW 4 dup (?)    ; Array der Drucker-Portadressen

                ORG 6ch
BiosTime        DW ?            ; ZÑhler der Timer-Ticks

Bios_Data       ENDS


Bios_Code       SEGMENT AT 0f000h   ; Bios-ROM
                ORG 0fffeh
Rechnertyp      DB ?            ; zur Unterscheidung PC <--> AT
Bios_Code       ENDS


SubTtl Code-Bereich - Leseroutinen
;******************************************************
                .Code

;---------------------------------------
Cls             PROC
;---------------------------------------
; lîscht den Bildschirm, setzt den Cursor auf die
; linke obere Ecke

                mov ah, 6               ; Scrolle hoch
                mov cx, 0               ; links oben
                mov dx, 24 * 256 + 79   ; rechts unten
                mov bh, 7               ; Attribut
                mov al, 0               ; Anzahl 0 = Clear
                int 10h

                mov ah, 15
                int 10h                 ; hole Daten (Seite)

                mov ah, 2
                mov dx, 0
                int 10h

                ret
Cls             ENDP



;---------------------------------------
InitSharp       PROC USES es, Port:Word
;---------------------------------------
; int initsharp (int port)
;
; Initialisiert die Schnittstelle
; Port:         gewÅnschte Schnittstelle (0 = LPT1, 1 = LPT2 etc.)
; Ergebnis:     TRUE = Erfolgreich
;               FALSE = Schnittstelle nicht vorhanden

                mov TimeoutReason, 0

                in al, PIO_B            ; ZÑhler und Lautsprecher aus
                IOWait
                and al, 11111100b
                out PIO_B, al

                mov ax, FALSE
                cmp Port, 4
                jae init_end

                mov es, BiosSegment
                ASSUME es:Bios_Data

                mov bx, Port
                add bx, bx
                mov dx, es:ParallelPort[bx]
                cmp dx, 0
                jz init_end
                mov LPT_Base, dx

                mov ax, Bios_Code
                mov es, ax
                ASSUME es:Bios_Code

                cmp es:Rechnertyp, 0fch     ; ist es ein AT?
                mov ax, Timer_Read_AT
                je init1
                mov ax, Timer_Read_PC
init1:
                mov Timer_Read, ax

                mov ax, TRUE
init_end:
                ret
InitSharp       ENDP


;---------------------------------------
syncronisiere   PROC USES ES
;---------------------------------------
; int syncronisiere ()
;
; Wartet auf das Sync-Signal und ermittelt die
; Ladegeschwindigkeit

;----- 1. Warte, bis sich etwas auf der Leitung bewegt,
;         max. etwa 5 Sekunden.

                mov TimeoutReason, 0

                mov es, BiosSegment
                ASSUME es:Bios_Data

                mov bx, es:BiosTime
                add bx, 91              ; etwa 5 Sekunden

                mov dx, LPT_Base
                add dx, 2
                in al, dx               ; hole Leitungsstatus
                IOWait
                and al, not DataMask    ; Strobe mu· 0 sein!
                out dx, al
                IOWait

                mov ah, al              ; aktuellen Status merken

Syncron_1:
                in al, dx               ; dx = Druckerport!
                IOWait
                xor ah, al
                and ah, DataMask        ; VerÑnderungen auf der Leitung?
                mov ah, al
                jnz Syncron_2           ; ja: nÑchster Schritt

                cmp bx, es:BiosTime     ; Time Out?
                jne Syncron_1           ; nein: ok, warte weiter!

                mov ax, tNoSignal       ; ja: Fehler
                jmp Syncron_End

;----- 2. Warte auf einen weiteren Pegelwechsel, da der Sharp noch den
;         Kassettenmotor einschaltet

Syncron_2:
                mov bx, es:BiosTime
                add bx, 18              ; gib ihm etwa 1 Sekunde Zeit

                in al, dx               ; dx = Druckerport!
                xor ah, al
                and ah, DataMask        ; VerÑnderungen auf der Leitung?
                mov ah, al
                jnz Syncron_3           ; ja: nÑchster Schritt

                cmp bx, es:BiosTime     ; Time Out?
                jne Syncron_2           ; nein: ok, warte weiter!

                mov ax, tNoSignal       ; ja: Fehler
                jmp Syncron_End

Syncron_3:
                cmp bx, es:BiosTime
                jne Syncron_3

;----- 3. Messe einige Perioden aus (es sollten eigentlich 4000Hz sein!)

                cli                     ; nicht stîren, bitte!
                StopTimer
                LoadTimer TimerInit1, TimerValue    ; ZÑhler auf 0 ( = 2^16)
                mov cx, 7*nMess         ; zÑhle cx Perioden aus

                mov ah, 0               ; gesucht: 0
                call WaitForBit
                jc Sync_Timeout
                StartTimer
Syncron_4:
                xor ah, DataMask
                call WaitForBit
                jc Sync_Timeout

                loop Syncron_4

                ; Zeit = (-Timer) * 1.5/10

                ReadTimer
                sti                     ; hier wieder erlaubt
                cmp ax, 0               ; Timeout?
                jz Sync_Timeout

                not ax                  ; ax = 0ffffh - ax, also die
                                        ; abgelaufene Zeit fÅr TimerValue = ffff

                push ax                 ; rette ZÑhlerwert

                mov bx, 3
                mul bx
                mov bx, 2*nMess         ; 3/20 = 1.5/10
                div bx                  ; ax = Timer/10 (Zeit fÅr 7 Halbwellen)
                mov Periode_7, ax
                cwd
                mov bx, 7
                div bx
                mov Periode, ax         ; Zeit fÅr eine Halbwelle

                pop ax
                mov bx, 7*nMess/2
                cwd
                div bx
                mov Periode_Senden, ax

                mov ax, 0               ; kein Fehler!
                jmp short Syncron_End
Sync_Timeout:
                mov ax, tSyncError
Syncron_End:
                ret
syncronisiere   ENDP


;---------------------------------------
WaitForBit      PROC USES ES
;---------------------------------------
; Wartet, bis die Leitung den Pegel hat, den
; das Bit 7 von AH angibt.
; Auch diese Routine hat ein Time-Out (Cy).
; DX enthÑlt den Daten-Port!

                mov es, BiosSegment
                ASSUME es:Bios_Data

                mov bx, 0               ; Zeitschleife, geht nicht anders (cli)
WaitForLoop:
                in al, dx
                xor al, ah
                and al, DataMask        ; ist es das gewÅnschte Bit?
                jz WaitFor_End          ; "AND" lîscht das Carry!

                dec bx
                jnz WaitForLoop
                mov TimeoutReason, tWaitBit
                stc
WaitFor_End:
                ret
WaitForBit      ENDP


;---------------------------------------
Skip            PROC
;---------------------------------------
; öberliest die nÑchsten CX Pegelwechsel auf der Leitung.
; AH enthÑlt den aktuellen Pegel (sowohl vorher als auch hinterher!)

                xor ah, DataMask
                call WaitForBit
                jc Skip_End
                loop Skip
Skip_End:
                ret
Skip            ENDP


;---------------------------------------
Sync            PROC USES CX
;---------------------------------------
; Wartet das nÑchste 0-Bit (Startbit) ab.

Sync_Loop:
                StopTimer
                LoadTimer TimerInit1, Periode       ; Zeit fÅr 1 Periode

                mov ah, DataMask
                call WaitForBit         ; warte auf 1
                StartTimer

                mov ah, 0               ; und stoppe die Zeit, bis wieder 0
                call WaitForBit
                jc Sync2                ; Time-Out

                push dx
                mov dx, Timer_Read
                in al, dx               ; Timer abgelaufen (ja = 0-Bit)
                pop dx
                test al, TimerMask
                jz Sync_Loop            ; nein: 1-Bit, weiter warten
                                        ; 1-Bit: warte weitere 3 Perioden!
                mov cx, 6               ; warte 6 Pegelwechsel ab
                call Skip
Sync2:
                jnc Sync_End
                cmp TimeoutReason, tWaitBit   ; wenn hier der Fehler auftritt
                stc                           ; trotzdem Fehler
                jne Sync_End
                mov TimeoutReason, tEndOfData ; dann öbertragungsende
Sync_End:
                ret
Sync            ENDP


;---------------------------------------
GetBit          PROC USES CX
;---------------------------------------
; Holt ein Bit. Das Bit wird im Carry geliefert.
; Die Routine startet ihre ZÑhlung bei einem 0-1-öbergang
; und endet nach dem 1-0-öbergang. Es werden alle 4 bzw 8
; Perioden ausgezÑhlt!

                StopTimer               ; Stoppuhr sperren
                LoadTimer TimerInit1, Periode_7
                mov ah, 0
                call WaitForBit         ; wirklich 0?
                mov ah, DataMask        ; warte auf 1
                call WaitForBit
                StartTimer              ; jetzt Uhr starten

                mov cx, 7               ; warte 7 Signalwechsel ab
                call Skip
                jc GetBit_End

                push dx
                mov dx, Timer_Read
                in al, dx               ; wenn Zeit abgelaufen, dann 0-Bit
                pop dx
                mov cl, TimerBit+1
                shr al, cl              ; schiebe Timer-Ausgang in Carry
                cmc                     ; hier: Cy = 1-Bit, nC = 0-Bit
                jnc GetBit_End          ; bei 0: fertig
                                        ; bei 1: warte noch 4 weitere Perioden
                mov cx, 8
                call Skip
                stc                     ; Cy = 1-Bit
GetBit_End:
                ret
GetBit          ENDP


;---------------------------------------
HoledByte       PROC
;---------------------------------------
; char HoledByte (void)
;
; Holt ein Datenbyte. Das Byte wird in AL geliefert.

                call holebyte           ; im Datenblock sind Low- und High-
                mov cl, 4
                ror al, cl              ; Rotation um 4 Bits = Nibble-Tausch
                ret
HoledByte       ENDP


;---------------------------------------
holehbyte       PROC
;---------------------------------------
; char holehbyte (void)
;
; Holt ein Headerbyte. Das Byte wird in AL geliefert.

                call holebyte
                ret
holehbyte       ENDP


;---------------------------------------
holebyte        PROC
;---------------------------------------
; Holt ein Byte. Das Byte wird in AL geliefert.

                mov TimeoutReason, 0
                mov dx, LPT_Base
                add dx, 2
                in al, dx
                IOWait
                and al, not DataMask    ; Strobe mu· 0 sein
                out dx, al

                call Sync               ; warte auf Startbit
                jc GetByte_End
                mov cl, 8               ; cl zÑhlt Bits
GetByteLoop:
                push ax
                call GetBit
                pop ax

                rcr al, 1               ; al empfÑngt

                cmp TimeoutReason, 0    ; bei Timeout abbrechen
                jnz GetByte_End

                dec cl
                jz GetByte_End          ; alle Bits?
                cmp cl, 4
                jnz GetByteLoop

                push ax                 ; nach 4 Daten-Bits
                call Sync               ; warte auf 2. Startbit
                pop ax
                jmp GetByteLoop
GetByte_End:
                ret
holebyte        ENDP


;---------------------------------------
Check           PROC USES cx
;---------------------------------------
; Berechnet die PrÅfsumme in Checksum. Das aktuelle
; Byte ist in AL

                mov ch, al              ; merken
                mov ah, al
                and ah, 0fh             ; ah = low nibble
                mov cl, 4
                shr al, cl              ; al = high nibble
                add Checksum, al        ; zuerst das high nibble
                adc Checksum, ah        ; dann mit öbertrag das low nibble
                mov al, ch              ; Orginal wiederherstellen
                ret
Check           ENDP


;---------------------------------------
HoleName        PROC
;---------------------------------------
; Holt einen Namen (entweder Dateiname oder Password)
; nach SI
; Liefert in AX die Fehlernummer (0 = kein Fehler)

                mov BYTE PTR [si+8], 0  ; Sicherheitshalber fÅrs String-Ende
                mov Checksum, 0         ; neue PrÅfsummenberechnung

                mov bx, 7               ; ein Name hat max. 7 Zeichen
HoleName1:
                dec bx
                push bx
                call holehbyte          ; Namen einladen
                call Check              ; und PrÅfsumme berechnen
                pop bx
                mov [si+bx], al         ; Zeichen ablegen

                mov ax, TimeoutReason   ; ist ein Fehler aufgetreten?
                cmp ax, 0
                jnz HoleName_End        ; ja: sofort beenden!

                cmp bx, 0
                jnz HoleName1

                call HoledByte          ; hole Ende-Marke
                call Check
                cmp al, 5fh             ; Kennung Header-Ende
                je HoleName2

                mov ax, f_Header_id
                jmp short HoleName_End
HoleName2:
                call HoledByte          ; hole PrÅfsumme
                cmp al, Checksum        ; und vergleiche
                mov ax, f_Header_chk
                jne HoleName_End

                mov ax, 0               ; wer bis hierhin kommt: kein Fehler!
HoleName_End:
                ret
HoleName        ENDP


;---------------------------------------
HoleHeader      PROC
;---------------------------------------
; LÑdt den Dateinamen.
; DI zeigt auf die Header-Struktur

                mov [di].Sharpname[7], 0
                mov [di].Password[7], 0
                mov Checksum, 0

                call HoledByte
                cmp TimeoutReason, 0    ; Fehler?
                jz HoleHeader0          ; nein
                mov ax, f_NoHeader      ; Header kommt nicht
                jmp SHORT HoleHeader_End

HoleHeader0:
                mov [di].Dateityp, al

                cmp al, 70h             ; Header-ID (Kennung fÅr CSAVE)
                je HoleHeader1
                cmp al, 74h             ; Kennung fÅr PRINT #
                je HoleHeader1
                cmp al, 71h             ; Kennung fÅr CSAVE , "password"
                je HoleHeader1
                cmp al, 76h             ; BinÑrdatei
                je HoleHeader1

                mov ax, f_Header_id
                jmp short HoleHeader_End

HoleHeader1:
                lea si, [di].Sharpname
                call HoleName           ; Lade den Dateinamen ein
                cmp ax, 0
                jne HoleHeader_End

                cmp [di].Dateityp, 71h  ; war es geschÅtztes BASIC?
                mov ax, 0
                jne HoleHeader_End      ; nein: ok

                lea si, [di].Password   ; ja: hole das Password
                call HoleName

HoleHeader_End:
                ret
HoleHeader      ENDP


;---------------------------------------
HoleBasic       PROC USES ES DI
;---------------------------------------
; LÑdt den gesamten Datenblock als BASIC-Programm ein.
; DI zeigt auf die Header-Struktur.
; Als Ergebnis wird in AX die Fehlernummer zurÅckgegeben.

                call HoleAlles          ; ohne Test alles laden!
                cmp ax, 0
                jnz HoleBasic_End       ; Ende, wenn Fehler

                push ds                 ; Lade ES fÅr LODSB/STOSB
                pop es
                cld                     ; aufwÑrts
                mov si, [di].Buffer     ; SI = Quelladresse
                mov di, si              ; DI = Zieladresse
                mov bx, Anzahl          ; BX = verbleibende Bytes
                mov Anzahl, 0
                cmp bx, 3               ; ist die LÑnge gÅltig?
                mov ax, f_Data_chk
                jbe HoleBasic_End       ; nein: Ende !!! Fehler ausgeben!
HoleBasic1:
                mov Checksum, 0
                mov cx, 120             ; weil alle 120 Bytes PrÅfsumme
HoleBasic2:
                lodsb                   ; holen,
                call Check              ; prÅfen
                stosb                   ; und sichern
                inc Anzahl
                dec bx

                cmp bx, 3               ; noch 3 Bytes Åber (FF FF chk)?
                jbe HoleBasic3          ; ja: Ende!

                loop HoleBasic2         ; und weiter

                lodsb                   ; hole PrÅfsumme
                dec bx
                cmp al, Checksum        ; und teste
                je HoleBasic1           ; ok: weiter

        ; PrÅfsumme stimmt nicht

                mov ax, f_Data_chk      ; PrÅfsumme stimmt nicht!
                jmp short HoleBasic_End

HoleBasic3:
                lodsb                   ; letztes Basic-Byte (FF)
                call Check
                stosb
                inc Anzahl

                lodsb                   ; Åberspringe letztes FF
                lodsb                   ; und hole PrÅfsumme
                cmp al, Checksum
                mov ax, 0               ; Fehler 0: ok
                je HoleBasic_End        ; ok, Ende
                mov ax, f_Data_chk

HoleBasic_End:
                ret
HoleBasic       ENDP


;---------------------------------------
HoleAlles       PROC USES DI ES
;---------------------------------------
; LÑdt alle Daten, die ankommen, in den Buffer

                mov Anzahl, 0
                cld
                mov ax, @data
                mov es, ax              ; wird fÅr STOSB/W gebraucht
                ASSUME es:@data
                mov di, [di].Buffer     ; di --> Buffer
HoleAlles1:
                call HoledByte
                stosb
                inc Anzahl

                mov ax, TimeoutReason
                cmp ax, 0
                jz HoleAlles1
                cmp ax, tEndOfData
                jne HoleAlles_End
                mov ax, 0
HoleAlles_End:
                dec Anzahl              ; weil letztes Byte ungÅltig!
                ret
HoleAlles       ENDP


;---------------------------------------
HoleSharp       PROC USES SI DI, Header:PTR
;---------------------------------------
; int holesharp (sHeader *Header);

;----- 1. Syncronisiere

                call syncronisiere
                cmp ax, 0
                jne HoleSharp_End

;----- 2. Lade den Header

                cli                     ; ab hier bitte ungestîrt!
                mov di, Header
                cmp [di].Dateityp, S_DUMP
                jne HoleSharp1

                call HoleAlles
                jmp short HoleSharp2

HoleSharp1:
                call HoleHeader
                cmp ax, 0
                jne HoleSharp_End

;----- 3. Lade den Datenblock

                cmp [di].Dateityp, S_DATEN
                je HoleSharp11
                cmp [di].Dateityp, S_BINAER
                je HoleSharp11

                call HoleBasic          ; BASIC-Programm holen
                jmp short HoleSharp2

HoleSharp11:
                call HoleAlles          ; Daten holen

HoleSharp2:
                mov bx, Anzahl
                mov [di].Laenge, bx

                ;cmp ax, 0
                ;jne HoleSharp_End

HoleSharp_End:
                sti                     ; Interrupts sind wieder erlaubt.
                ret
HoleSharp       ENDP



SubTtl Code-Bereich - Schreibroutinen
;-----------------------------------------------------------------
;----(Ende der Leseroutinen, hier beginnen die Schreibroutinen)---
;-----------------------------------------------------------------

; Allgemeines zu den Senderoutinen:
;
;     Da die Laufzeit zwischen zwei aufeinanderfolgenden Aufrufen
;   von SendeBit unbestimmt und nicht konstant ist, wird am Ende
;   dieser Routine der Timer neu gestartet. Bei einem erneuten
;   Aufruf mu· sie also als erstes darauf warten, da· der Timer
;   abgelaufen ist. Der Timer wird in SendeSync initialisiert und
;   gestartet, und er lÑuft wÑhrend des gesamten Sendebetriebes
;   durch. Er darf also nicht unterbrochen oder gestoppt werden,
;   da sonst das Timing durcheinander geraten kînnte.


;---------------------------------------
SendeBit        PROC USES AX CX
;---------------------------------------
; Sendet das Bit, das im Carry steht.

                mov cx, 8 shl 8 + 2     ; 0-Bit: 4 Perioden
                jnc SendeBit0
                mov cx, 16 shl 8 + 1    ; 1-Bit: 8 Perioden
SendeBit0:
                mov bl, cl
SendeBit1:
                mov cl, bl
                mov dx, Timer_Read
SendeBit2:
                in al, dx
                IOWait
                mov ah, al              ; merke Status
                and ah, TimerMask
SendeBit3:
                in al, dx
                and al, TimerMask
                xor al, ah              ; hat sich was geÑndert?
                jz SendeBit3            ; warte, bis vorheriges Bit fertig ist

                dec cl
                jne SendeBit2

                mov dx, LPT_Base
                in al, dx
                IOWait
                xor al, DataBit         ; Bit ausgeben
                out dx, al

                dec ch                  ; fertig?
                jnz SendeBit1
SendeBit_End:
                ret
SendeBit        ENDP


;---------------------------------------
SendedByte      PROC USES CX
;---------------------------------------
; Sendet das Byte in AL als Datenbyte

                mov cl, 4
                mov ah, al              ; Nibble vertauscht!
                shl al, cl              ; Low nach High,
                shr ah, cl              ; High nach Low,
                or al, ah               ; und wieder zusammenbringen
                call SendeByte          ; und Senden
                ret
SendedByte      ENDP


;---------------------------------------
SendehByte      PROC USES CX
;---------------------------------------
; Sendet das Byte in AL als Headerbyte

                call SendeByte
                ret
SendehByte      ENDP


;---------------------------------------
SendeByte       PROC
;---------------------------------------
; Sendet das Byte in AL

                mov cl, 8           ; 8 Datenbits senden
SendeByte1:
                clc
                call SendeBit       ; Sende Startbit
SendeByte2:
                ror al, 1
                call SendeBit

                dec cl
                cmp cl, 4           ; 1. Nibble um?
                jne SendeByte3      ; nein: weiter

                stc
                call SendeBit       ; ein Stopbit
                jmp SendeByte1      ; neues Startbit und weiter
SendeByte3:
                cmp cl, 0           ; fertig?
                jnz SendeByte2      ; nein: nÑchstes Bit

                mov cx, Stopbits
SendeByte4:
                stc
                call SendeBit       ; Stopbits
                loop SendeByte4
                ret                 ; und Ende
SendeByte       ENDP


;---------------------------------------
SendeSync       PROC
;---------------------------------------
; Sendet einige Sekunden lang einen 4000 Hz-Ton
; (einige 1-Bits zur Synchronisation am Beginn
; der öbertragung)

                LoadTimer TimerInit3, Periode_Senden

                mov cx, 1000        ; 1000 1-Bits = 2 Sekunden Header
                mov dx, LPT_Base
                mov al, 0
                out dx, al          ; Bringe die Leitung auf defierten Pegel
                IOWait

                StartTimer          ; los gehts
SendeSync1:
                stc
                call SendeBit
                loop SendeSync1
                ret
SendeSync       ENDP


;---------------------------------------
SendeName       PROC
;---------------------------------------
; Sendet den Namen, auf den SI zeigt. Dabei wird auch die
; PrÅfsumme gesendet.

                mov Checksum, 0
                mov cx, 7
SendeName1:
                cmp BYTE PTR [si], 0    ; Suche das String-Ende
                je SendeName2
                inc si
                loop SendeName1         ; kein Ende gefunden: ok
                jmp short SendeName3
SendeName2:
                mov BYTE PTR [si], 0    ; fÅlle bis zum Ende mit 0 auf
                inc si
                loop SendeName2
SendeName3:
                mov cx, 7
                std                     ; Richtung abwÑrts
                dec si
SendeName4:
                lodsb
                call Check
                call SendehByte
                loop SendeName4

                mov al, 5fh             ; Kennzeichen fÅr Namenende
                call Check
                call SendedByte

                mov al, Checksum        ; und PrÅfsumme senden
                call SendedByte

                ret
SendeName       ENDP


;---------------------------------------
SendeHeader     PROC USES SI
;---------------------------------------
; Sendet einen Header nach den Informationen in der
; Datenstruktur sHeader, auf die DI zeigt.

                mov al, [di].Dateityp
                call SendedByte

                lea si, [di].Sharpname
                call SendeName

                cmp [di].Dateityp, 71h      ; BASIC, geschÅtzt
                jne SendeHeader_End

                lea si, [di].Password
                call SendeName
SendeHeader_End:
                ret
SendeHeader     ENDP


;---------------------------------------
SendeBasic      PROC USES SI
;---------------------------------------
; Sende den Datenbereich, auf dessen Header-
; Struktur DI zeigt, als BASIC-Programm.

                cld                     ; zÑhle aufwÑrts
                mov bx, [di].Laenge     ; Anzahl der Bytes
                mov si, [di].Buffer     ; Adresse des Puffers
SendeBasic1:
                mov Checksum, 0
                mov cx, 120             ; fÅr die PrÅfsumme
SendeBasic2:
                push bx
                lodsb                   ; Hole das Byte
                call Check
                call SendedByte         ; sende Byte
                pop bx
                dec bx
                cmp bx, 1
                jbe SendeBasic3
                loop SendeBasic2

                mov al, Checksum
                push bx
                call SendedByte
                pop bx
                jmp SendeBasic1

SendeBasic3:
                lodsb                   ; letztes Basic-Byte
                call Check
                call SendedByte

                mov al, 0FFh
                call SendedByte
                mov al, Checksum
                call SendedByte

                ret
SendeBasic      ENDP


;---------------------------------------
SendeAlles      PROC
;---------------------------------------
; Sendet den ganzen Datenblock, ohne auf PrÅfsummen etc.
; zu achten.

                mov si, [di].Buffer
                mov cx, [di].Laenge
                cld
                jcxz SendeAlles_End
SendeAlles1:
                lodsb
                call SendedByte
                loop SendeAlles1
SendeAlles_End:
                ret
SendeAlles      ENDP


;---------------------------------------
SendeSharp      PROC USES DI, Header:PTR
;---------------------------------------
; int sendesharp (sHeader *Header);

;----- 1. Sende den Sync-Vorspann

                cli

                call SendeSync

;----- 2. Sende den Header

                mov Stopbits, 5

                mov di, Header
                cmp [di].Dateityp, S_DUMP
                je SendeSharp2

                call SendeHeader

;----- 3. Sende den Datenblock

                cmp [di].Dateityp, S_DATEN
                je SendeSharp2
                cmp [di].Dateityp, S_BINAER
                je SendeSharp2

                mov Stopbits, 2
                call SendeBasic
                jmp short SendeSharp3

SendeSharp2:
                mov Stopbits, 3
                call SendeAlles
SendeSharp3:
                mov ax, 0
SendeSharp_End:
                sti
                ret
SendeSharp      ENDP

;---------------------------------------
                End
