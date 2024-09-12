PC-1360 Makros
	Zahlenoperationen
		Addition|Bei der Addition werden die Zahlen im BCD Format in den Operationsregistern X (int. Ram 16-23) und Y (24-31) zusammengezählt. X = X + Y|Die Summe wird im BCD Format in X gespeichert.|0|CALL	&1E80|CALL	&1E80|
		Subtraktion|Bei der Subtraktion werden die Zahlen im BCD Format in den Operationsregistern X (int.Ram 16-23) und Y (24-31) abgezogen. X = Y - X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1E84|CALL	&1E84|
		Multiplikation|Bei der Multiplikation werden die Zahlen im BCD Format in den Operationsregistern X (int.Ram 16-23) und Y (24-31) multipliziert. X = Y * X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1E88|CALL	&1E88|
		Division|Bei der Division werden die Zahlen im BCD Format in den Operationsregistern X (int.Ram 16-23) und Y (24-31) geteilt. X = Y / X|Die Summe wird im BCD Format in X gespeichert.|0|CALL	&1E8C|CALL	&1E8C|
		Wurzel|Wurzel aus einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = SQRT(X)|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EF4|CALL	&1EF4|
		Logarithmus LN|Natürlicher Logarithmus LN einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = LN X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EFC|CALL	&1EFC|
		Logarithmus LOG|Dekadischer Logarithmus LOG einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = LOG X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EF8|CALL	&1EF8|
		Exponent|EXP e^X einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = e^X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EC0|CALL	&1EC0|
		Sinus|Sinus einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = SIN X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EC4|CALL	&1EC4|
		Cosinus|Cosinus einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = COS X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EC8|CALL	&1EC8|
		Tangens|Tangens einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = TAN X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1ECC|CALL	&1ECC|
		Arcussinus|Arcussinus einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = ASN X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1ED0|CALL	&1ED0|
		Arcuscosinus|Arcuscosinus einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = ACS X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1ED4|CALL	&1ED4|
		Arcustangens|Arcustangens einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = ATN X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1ED8|CALL	&1ED8|
		DEG|Umwandlung von DMS in Grad einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = DEG X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EDC|CALL	&1EDC|
		DMS|Umwandlung von Gradangaben in DMS einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = DMS X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EE0|CALL	&1EE0|
		Betrag|Betrag einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = ABS X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EE4|CALL	&1EE4|
		Abrunden|Ganzzahliger Wert (abgerundet) einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = INT X|Das Ergebnis wird im BCD Format in X gespeichert.|0|CALL	&1EE8|CALL	&1EE8|
		Vorzeichen|Vorzeichen einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = SGN X|Das Ergebnis (-1, 0, +1) wird im BCD Format in X gespeichert.|0|CALL	&1EEC|CALL	&1EEC|
		Zufallszahl|Zufallszahl aus einer Zahl im BCD Format im Operationsregister X (int.Ram 16-23). X = RND X|Das Ergebnis wird im BCD Format in X gespeichert. X zwischen 0 und 1 erzeugt eine reelle Zufallszahl, X ab 1 erzeugt eine Zufallszahl 1 bis X.|0|CALL	&1EEC|CALL	&1EEC|
	Vergleichsoperationen
		Y <> X|Y<>X: Zahlen im BCD Format in Operationsregister X (int.Ram 16-23) und Y (24-31). Strings als Pointeradresse in den hinteren 4 Bytes des jew. OR: &0D LB HB Länge. Routine in Bank 0!|Wenn Aussage wahr dann steht in OR X eine BCD 1 ansonsten alle Bytes 0.|1|CALL	&6AA3|CALL	&6AA6|CALL	&6AAC|CALL	&6AAF|
		Y = X|Y=X: Zahlen im BCD Format in Operationsregister X (int.Ram 16-23) und Y (24-31). Strings als Pointeradresse in den hinteren 4 Bytes des jew. OR: &0D LB HB Länge. Routine in Bank 0!|Wenn Aussage wahr dann steht in OR X eine BCD 1 ansonsten alle Bytes 0.|1|CALL	&6A87|CALL	&6A8A|CALL	&6A95|CALL	&6A98|
		Y > X|Y>X: Zahlen im BCD Format in Operationsregister X (int.Ram 16-23) und Y (24-31). Strings als Pointeradresse in den hinteren 4 Bytes des jew. OR: &0D LB HB Länge. Routine in Bank 0!|Wenn Aussage wahr dann steht in OR X eine BCD 1 ansonsten alle Bytes 0.|1|CALL	&6A4C|CALL	&6A4F|CALL	&6A55|CALL	&6A58|
		Y < X|Y<X: Zahlen im BCD Format in Operationsregister X (int.Ram 16-23) und Y (24-31). Strings als Pointeradresse in den hinteren 4 Bytes des jew. OR: &0D LB HB Länge. Routine in Bank 0!|Wenn Aussage wahr dann steht in OR X eine BCD 1 ansonsten alle Bytes 0.|1|CALL	&6A37|CALL	&6A3A|CALL	&6A42|CALL	&6A45|
		Y >= X|Y>=X: Zahlen im BCD Format in Operationsregister X (int.Ram 16-23) und Y (24-31). Strings als Pointeradresse in den hinteren 4 Bytes des jew. OR: &0D LB HB Länge. Routine in Bank 0!|Wenn Aussage wahr dann steht in OR X eine BCD 1 ansonsten alle Bytes 0.|1|CALL	&6A72|CALL	&6A75|CALL	&6A7D|CALL	&6A80|
		Y <= X|Y<=X: Zahlen im BCD Format in Operationsregister X (int.Ram 16-23) und Y (24-31). Strings als Pointeradresse in den hinteren 4 Bytes des jew. OR: &0D LB HB Länge. Routine in Bank 0!|Wenn Aussage wahr dann steht in OR X eine BCD 1 ansonsten alle Bytes 0.|1|CALL	&6A5F|CALL	&6A62|CALL	&6A68|CALL	&6A6B|
	Umwandlungen
		STR$ (BCD zu String)|STR$ wandelt eine BCD Zahl in einen String um. Die Zahl im BCD Format ins Operationsregister X (int.Ram 16-23) und &60 an die RAM Adresse (Stringpuffer) &FE79 schreiben.|OR X enthält den Pointer auf den String in den letzten 4 Bytes: &0D LB HB Länge.|0|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F08|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F08|
		CHR$ (BCD zu ASCII)|CHR$ wandelt eine BCD Zahl von 0..255 in ein Zeichen (String) um. Die Zahl im BCD Format ins Operationsregister X (int.Ram 16-23) und &60 an die RAM Adresse (Stringpuffer) &FE79 schreiben.|Carry = 0: OR X enthält den Pointer auf den String in den letzten 4 Bytes: &0D LB HB Länge.\nCarry = 1: Die Zahl war nicht im Bereich 0..255.|0|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F04|LIDP	&FE79\nLIA	&60\nSTD\nCALL	&1F04|
		VAL (String zu BCD)|VAL wandelt einen String in eine BCD Zahl um. Operationsregister X (int.Ram 16-23) soll in den letzen 4 Bytes &0D &79 &FE Länge enthalten und der String kommt an RAM Adresse (Stringpuffer) &FE79.|Carry = 0: OR X enthält die Zahl im BCD Format.\nCarry = 1: Die Umwandlung war nicht möglich.|0|CALL	&1F0C|CALL	&1F0C|
		ASC (ASCII zu BCD)|ASC wandelt ein Zeichen in einen BCD ASCII Code um. Operationsregister X (int.Ram 16-23) soll in den letzen 4 Bytes &0D &79 &FE &01 enthalten und das Zeichen kommt an die RAM Adresse (Stringpuffer) &FE79.|OR X enthält den ASCII Code der Zahl im BCD Format.|0|CALL	&1F00|CALL	&1F00|
		BCD zu HB/LB|BCD->HB/LB wandelt eine BCD Zahl in zwei Bytes um. Operationsregister X (int.Ram 16-23) enthält die Zahl. Version 0 und 1 sind identisch, aber es existieren zwei Routinen: Für Zahlen von 0..65535 Version 1 und -32768..32767 Version 0.|Carry 0: LB in &18 und HB in &19 des internen RAM gespeichert\nCarry 1: X ist nicht im gültigen Bereich|0|CALL	&0912|CALL	&091D|
		HB/LB zu BCD|HB/LB->BCD wandelt zwei Bytes in eine BCD Zahl um. LB muss nach &18 internes RAM und HB nach &19. Version 0 und 1 sind identisch, aber es existieren zwei Routinen: Für Ergebniszahlen von 0..65535 Version 1 und -32768..32767 Version 0.|Operationsregister X (int.Ram 16-23) enthält die Zahl im BCD Format.|0|CALL	&0891|CALL	&088A|
	Suchfunktionen
		Zeilennummernsuche|Version 1: Zeilennummernsuche nach einer Zeile im HB/LB-Format: LB &18 und HB &19 int. RAM. Version 0: Im OR X (16-23) liegt die Zeilennummer im BCD Format, internes RAM &36 OR 00000010 eintragen.\nRoutine in Bank 2!\nDie Bankumschaltung benutzt Akkumulator A!|Carry = 0: Zeile wurde gefunden. Daten im int. RAM:\n	&3A Adresse Zeilennr LB\n	&3B Adresse Zeilennr HB\n	&3C Zeilennummer HB\n	&3D Zeilennummer LB\nCarry = 1: Zeile nicht gefunden.\n	&3C und &3D = 0: Gesamtes Programm ohne\n	Erfolg durchsucht\n	&3C oder &3D <> 0: eine Zeilennummer\n	gefunden, die größer ist|0|LIP	&36\nORIM	2\nLIA	2\nLIDP	&3400\nSTD\nCALL	&5EBE\nLIA	0\nLIDP	&3400\nSTD|LIP	&36\nORIM	2\nLIA	2\nLIDP	&3400\nSTD\nCALL	&5ECB\nLIA	0\nLIDP	&3400\nSTD|
		einfache Variable suchen|Suche nach einer einfachen Variablen mit zweistelligem Namen. Erstes Byte des Namens nach &0A int. RAM und zweites nach &0B. &33 wird auf 0 gesetzt.|Die Adresse des ersten Bytes der Variable wird im int. RAM an &06 LB und &07 HB (Y) gespeichert. A enthält die Länge der Variable. Wenn die Variable nicht existiert sucht der Rechner in einer Endlosschleife.|0|LIP	&33\nANID	0\nCALL	&6E82|LIP	&33\nANID	0\nCALL	&6E85|
		Feldvariable suchen|Suche nach einer Feldvariablen. Erstes Byte des Namens nach &0A int. RAM und zweites nach &0B. &33 wird auf 0 gesetzt. Die Dimensionierung muss in &0C und &0D gespeichert werden.\nEindimensionales Feld: &0C Feldgröße\nZweidimensionales Feld: &0D erste und &0C zweite Dimensionierung|Carry = 0: Die Adresse des ersten Bytes der Variable wird im int. RAM an &06 LB und &07 HB (Y) gespeichert. A enthält die Länge einer Feldeinheit.\nCarry = 1: Die Angaben existieren nicht.|0|CALL	&7135|CALL	&7138|
	Tastatur
		Tastenabfrage|Tastaturabfrage|Carry = 0: keine Taste gedrückt\NCarry = 1: Tastaturcode in A: 0..46, 47 bei mehreren Tasten|0|CALL	&1E9C|CALL	&1E9C|
		ASCII Abfrage|ASCII Tastenabfrage: Wartet auf Tastendruck|Speichert den ASCII Code in B|0|CALL 18442|CALL 18442|
	Display
		Zeilen und ganzes Display|Displayzeilenadressen:\N1 &FD80 - &FD97\N2 &FD98 - &FDAF\N3 &FDB0 - &FDC7\N4 &FDC8 - &FDDF\NRegister K=0, Bit 2 von &FD01 gesetzt, Akku 0..3 für die einzelnen Zeilen, 4 für ganzes Display.|Ausgabe der ASCII Zeichen im Displayspeicher|0|LP	8	# K=0\NANIM	0\NLIDP	&FD01	# Bit 2 setzen\NORID	4\NLIA	1\NLIDP	&3400	# Bankwechsel\NSTD\NLIA	4	# A=4: Ganzer Bildschirm\NCALL	&4004	# Funktionsaufruf: Display\NLIA	0	# Bankwechsel\NLIDP	&3400\NSTD|LP	8	# K=0\NANIM	0\NLIDP	&FD01	# Bit 2 setzen\NORID	4\NLIA	1\NLIDP	&3400	# Bankwechsel\NSTD\NLIA	4	# A=4: Ganzer Bildschirm\NCALL	&4004	# Funktionsaufruf: Display\NLIA	0	# Bankwechsel\NLIDP	&3400\NSTD|||
		Einzelnes Zeichen|Ausgabe eines einzelnen Zeichens an definierter Position: Die Adresse &FD10 enthält die X, &FD11 die Y Koordinate (0 basierend, 24*4 Zeichen)\nBit 0 von &FD01 wird gesetzt\nAkku = Zeichen ASCII\nRoutine auf Bank 1|Das Zeichen erscheint auf dem Bildschirm.|0|LIDP	&FD01\nORID	1\nLIA	1\nLIDP	&3400\nSTD\nLIA	65\nCALL	&615E\nLIA	0\nLIDP	&3400\nSTD|LIDP	&FD01\nORID	1\nLIA	1\nLIDP	&3400\nSTD\nLIA	65\nCALL	&615E\nLIA	0\nLIDP	&3400\nSTD|
		Nach unten scrollen|Weiterscrollen:\nAkku = 4 (Routine auf Bank 1)|Der Bildschirm scrollt eins nach unten, so dass die unterste Zeile leer ist.|0|LIA	1\nLIDP	&3400\nSTD\nLIA	4\nCALL	&4043\nLIA	0\nLIDP	&3400\nSTD|LIA	1\nLIDP	&3400\nSTD\nLIA	4\nCALL	&4043\nLIA	0\nLIDP	&3400\nSTD|
		Display ausschalten|Display ausschalten|Der Bildschirm wird stromlos geschaltet.|0|CALL	&1E98|CALL	&1E98|
		Display anschalten|Display anschalten|Der Bildschirm wird angeschaltet.|0|CALL	&1E94|CALL	&1E94|
	Serielle Schnittstelle
		SIO öffnen|Serielle Schnittstelle öffnen|ER Signal geht auf high. Die Schnittstelle kann angesprochen werden.|0|CALL	&1F2C|CALL	&1F2C|
		SIO schließen|Serielle Schnittstelle schließen|Alle Leitungen gehen auf low.|0|CALL	&1F30|CALL	&1F30|
		CS Status|CS Überwachung|Carry = 0: CS Leitung hat high.\nCarry = 1: Break wurde gedrückt.\nAnsonsten wird die Routine nicht beendet.|0|CALL	&1EA0|CALL	&1EA0|
		CD Status|CD Überwachung|Carry = 0: CD Leitung hat high.\nCarry = 1: Break wurde gedrückt.\nAnsonsten wird die Routine nicht beendet.|0|CALL	&1EA4|CALL	&1EA4|
		SIO Parameter vorbereiten|Schnittstelleneinstellungen lesen: Diese Routine muss vor allen Sende- und Empfangsroutinen ausgeführt worden sein!|Die Einstellungen der Schnittstelle werden im int. RAM abgelegt:\nEOT Code &0D\nBaudrate &0E\nBiteinstellungen &0F|0|CALL	&1EA8|CALL	&1EA8|
		Ein Byte senden|Ein Byte senden: SIO Parameter müssen in den int. RAM eingelesen worden sein!\nA enthält das Byte.|Das Zeichen wird gesendet.|0|CALL	&1F34|CALL	&1F34|
		Ein Byte empfangen|Ein Byte empfangen: SIO Parameter müssen in den int. RAM eingelesen worden sein!|Das empfangene Zeichen landet in B\nCarry = 0: Kein Fehler\nCarry = 1: Bit 5 in &35 int. RAM = 1: Break gedrückt, ansonsten Paritätsfehler\nBei Carry = 1 geht RR auf low.|0|CALL	&1F38|CALL	&1F38|
		Endcode senden|Den Endcode senden: SIO Parameter müssen in den int. RAM eingelesen worden sein!|Carry = 0: Kein Fehler\nCarry = 1: Break wurde gedrückt|0|CALL	&1F3C|CALL	&1F3C|
		Internes Format zu ASCII|Internes Format zu ASCII Sequenz umwandeln: Register X enthält die Zahl im internen Format|Im SIO Puffer (&FB00) steht der String (mit CHR 13 abgeschlossen)|0|CALL	&1F40|CALL	&1F40|
		SIO Puffer senden|SIO Puffer senden: Der Puffer (&FB00-&FBFF) wird mit zu sendenden Daten gefüllt.|Der Puffer (&FB00-&FBFF) wird bis zu einem CHR 13 gesendet. Das CHR 13 wird mitgesendet!\nCarry = 0: Alles gesendet\nCarry = 1: Break wurde gedrückt|0|CALL	&1F40|CALL	&1F40|
	Drucker
		Drucker zurücksetzen|Drucker zurücksetzen: Drucker einschalten!|Drucker bereit.|0|CALL	&1F18|CALL	&1F18|
		Zeile drucken|Eine Zeile drucken: Die Daten kommen in die Operationsregister X (&10-&17), Y (&18-&1F) und Z (&20-&27)|Die 24 Bytes werden gedruckt und eine Zeile weitergeschoben.|0|CALL	&1F1C|CALL	&1F1C|
		Papiervorschub|Zeilenvorschub: Leerzeichen kommen in die Operationsregister X (&10-&17), Y (&18-&1F) und Z (&20-&27)|Die 24 Leerzeichen werden gedruckt und eine Zeile weitergeschoben.|0|LP	&10\nLII	23\nLIA	32\nFILM\nCALL	&1F1C|LP	&10\nLII	23\nLIA	32\nFILM\nCALL	&1F1C|
	Kassette
		Remote an|Kassettenkontrolle aktivieren|Die Kassette wird erst aufgenommen oder abgespielt, wenn der Computer darauf zugreift.|0|CALL	&1F24|CALL	&1F24|
		Remote aus|Kassettenkontrolle deaktivieren|Die Kassette wird aufgenommen oder abgespielt, auch wenn der Computer nicht bereit ist.|0|CALL	&1F28|CALL	&1F28|
		Header speichern|Kassettenheader speichern: Dateiname in Operationsregister Z, ab 2. Byte (&21-&27)\n7 Bytes lang, mit &00 terminiert\n&31 int. RAM = 0 setzen|Der Kopf für einen Speichervorgang wird auf Band geschrieben.|0|LP	&31\nANIM	0\nCALL	&1F28|LP	&31\nANIM	0\nCALL	&1F28|
		Header laden|Kassettenheader laden: Dateiname in Operationsregister Z, ab 2. Byte (&21-&27)\n7 Bytes lang, mit &00 terminiert (außer bei 7 Bytes Länge)\nRoutine in Bank 4!|Der Kopf von einem Speichervorgang wird von Band gelesen. Wenn die angegebene Datei gefunden wurde wird ein * unten rechts angezeigt.|0|LIA	4\nLIDP	&3400\nSTD\nCALL	&68FD\nLIA	0\nLIDP	&3400\nSTD|LIA	4\nLIDP	&3400\nSTD\nCALL	&6902\nLIA	0\nLIDP	&3400\nSTD|
		Ein Zeichen speichern|Ein Byte auf Kassette speichern: Das Byte nach A schreiben.\nRoutine in Bank 4!|Das Byte wird auf Band gespeichert.|0|LIA	4\nLIDP	&3400\nSTD\nCALL	&5BBF\nLIA	0\nLIDP	&3400\nSTD|LIA	4\nLIDP	&3400\nSTD\nCALL	&5BC4\nLIA	0\nLIDP	&3400\nSTD|
	Sonstiges
		ROM Bank umschalten|ROM Bank schalten: Die Bank muss bei Rückkehr ins BASIC wieder auf 0 gesetzt sein!\nIn A steht die Banknummer 0..7.|Die ROM Bank wird gewechselt.|0|LIA	0\nLIDP	&3400\nSTD|LIA	0\nLIDP	&3400\nSTD|
	Systemvariablen
		A|Adresse von Variable A||0|64152|64152|
		B|Adresse von Variable B||0|64144|64144|
		C|Adresse von Variable C||0|64136|64136|
		D|Adresse von Variable D||0|64128|64128|
		E|Adresse von Variable E||0|64120|64120|
		F|Adresse von Variable F||0|64112|64112|
		G|Adresse von Variable G||0|64104|64104|
		H|Adresse von Variable H||0|64096|64096|
		I|Adresse von Variable I||0|64088|64088|
		J|Adresse von Variable J||0|64080|64080|
		K|Adresse von Variable K||0|64072|64072|
		L|Adresse von Variable L||0|64064|64064|
		M|Adresse von Variable M||0|64056|64056|
		N|Adresse von Variable N||0|64048|64048|
		O|Adresse von Variable O||0|64040|64040|
		P|Adresse von Variable P||0|64032|64032|
		Q|Adresse von Variable Q||0|64024|64024|
		R|Adresse von Variable R||0|64016|64016|
		S|Adresse von Variable S||0|64008|64008|
		T|Adresse von Variable T||0|64000|64000|
		U|Adresse von Variable U||0|63992|63992|
		V|Adresse von Variable V||0|63984|63984|
		W|Adresse von Variable W||0|63976|63976|
		X|Adresse von Variable X||0|63968|63968|
		Y|Adresse von Variable Y||0|63960|63960|
		Z|Adresse von Variable Z||0|63952|63952|
	Speicherstellen
		Basicstartadresse|Basicstartadresse Version 1=LB 2=HB||0|&FFD8|&FFD7|
		Basicendadresse|Basicendadresse Version 1=LB 0=HB||0|&FFDA|&FFD9|
		Mergeadresse|Mergeadresse Version 1=LB 0=HB||0|&FFDC|&FFDB|
		Grafikcursor X|Grafikcursor X Version 1=LB 0=HB||0|&FEE9|&FEE8|
		Grafikcursor Y|Grafikcursor Y Version 1=LB 0=HB||0|&FEEB|&FEEA|
		Textcursor X|Textcursor X (0-23)||0|&FD09|&FD09|
		Textcursor Y|Textcursor Y (0-3)||0|&FD0A|&FD0A|
		Displaypointer X|Displaypointer X (0-23)||0|&FD10|&FD10|
		Displaypointer Y|Displaypointer Y (0-3)||0|&FD11|&FD11|
		Endposition der letzten LINE X|Endposition der letzten LINE (X) Version 1=LB 0=HB||0|&FEED|&FEEC|
		Endposition der letzten LINE Y|Endposition der letzten LINE (Y) Version 1=LB 0=HB||0|&FEEF|&FEEE|
		Cursorposition im Eingabepuffer|Cursorposition im Eingabepuffer||0|&FD16|&FD16|
		WAIT Wert|WAIT Wert Version 1=LB 0=HB||0|&287F|&287E|
		Zeichen am Cursor|ASCII Code des Zeichens unter dem Cursor||0|&FF02|&FF02|
		Adresse vom Cursor|Adresse des Cursors im Displayspeicher Version 1=LB 0=HB||0|&FF01|&FF00|
		Variablenbeginn|Beginn der selbstdefinierten Variablen Version 1=LB 0=HB||0|&FFDE|&FFDD|
		BREAK Adresse|BREAK Adresse Version 1=LB 0=HB||0|&FEF5|&FEF4|
		ERROR Adresse|ERROR Adresse Version 1=LB 0=HB||0|&FEF7|&FEF6|
		Selbstabschaltungszähler|Selbstabschaltungszähler Version 1=LB 0=HB||0|&2A7F|&2A7E|
		RAM Karten Startadresse|RAM Karten Startadresse Version 1=LB 0=HB||0|&FFF7|&FFF6|
		RAM Karten Endadresse|RAM Karten Endadresse Version 1=LB 0=HB||0|&FFF9|&FFF8|
		Reservemodus|Reservemodusbereich Version 1=Startbyte 0=Letztes Byte||0|&FFCF|&FF40|
		Reservemoduslänge|Reservemoduslänge||0|&FF3F|&FF3F|
		Eingabezwischenspeicher|Eingabezwischenspeicher Version 1=Startbyte 0=Letztes Byte||0|&FD6F|&FD20|
		Displayspeicher|Displayspeicher im ASCII Format Version 1=Startbyte 0=Letztes Byte||0|&FDDF|&FD80|
		Stringzwischenspeicher|Stringzwischenspeicher Version 1=Startbyte 0=Letztes Byte||0|&FCAF|&FC60|
		Displayspeicher System 1|Displayspeicher System Spalte 1 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|10304|10240|
		Displayspeicher System 2|Displayspeicher System Spalte 2 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|10816|10752|
		Displayspeicher System 3|Displayspeicher System Spalte 3 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|11328|11264|
		Displayspeicher System 4|Displayspeicher System Spalte 4 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|11840|11776|
		Displayspeicher System 5|Displayspeicher System Spalte 5 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|12318|12288|
		Sondersymbole|Das Sondersymbolbyte|Bit 0=SHIFT\nBit 1=DEF\nBit 4=RUN\nBit 5=PRO\nBit 6=JAPAN\nBit 7=SML|0|12348|12348|
		Portadresse seriell|Ansteuerungsadresse für Ausgabe am 15-poligen Interface|Bit 0=Pin 4\nBit 1=Pin 14\nBit 2=Pin 11|0|&3800|&3800|
		Portadresse Drucker|Ansteuerungsadresse für Ausgabe am 11-poligen Interface|Bit 0=Pin 11\nBit 1=Pin 10\nBit 2=Pin 5\nBit 3=Pin 8|0|&3A00|&3A00|
		B-Port Ansteuerung|B-Port Ansteuerung Ausgabe: Bit 4=Pin 9 am Druckeranschluss, Bit 5=Pin 3, Bit 6=Pin 5 und Bit 7=Pin 8 am seriellen Anschluss\nVersion 1=Ausgabe, 0=Eingabe|Eingabe: Bit 3=Pin 8 und Bit 4=Pin 9 am Druckeranschluss, Bit 5=Pin 3, Bit 6=Pin 5 und Bit 7=Pin 8 am seriellen Anschluss|0|INB|LIP	&5D\nORIM	bit\nOUTB|
		F-Port Ansteuerung|F-Port Ansteuerung nur Ausgabe: Bit 1=Pin 2 am seriellen Anschluss||0|LIP	&5E\nORIM	2\nOUTF|LIP	&5E\nORIM	2\nOUTF|
PC-1350 Makros
	Ton
		BEEP kurz|Beep kurz: Geänderte Register: P,Q,A. Speicherstellen: 2,95|Wenn in A das Bit 0 gesetzt ist, 2 kHz ansonsten 4 kHz. Um den Summer auszuschalten: LIP 95, ANIM 239, OUTC|0|CALL	2379|CALL	2379|
		BEEP lang|Beep lang: Geänderte Register: unbekannt. Speicherstellen: unbekannt|Piept mit 4 kHz und muss nicht abgebrochen werden.|0|CALL	49430|CALL	49430|
	Display
		Print|PRINT: Geänderte Register: P,Q,DP,A,B,X,Y,K,L,V. Speicherstellen: 2..39|Gibt die ASCII-Zeichen im Anzeigepuffer wie CURSOR(K,A) aus, wobei A=1,2 die unteren beiden Zeilen sind und A=3,4 die oberen.|0|CALL	53958|CALL	53958|
		LPrint|LPRINT: Geänderte Register: P,Q,DP,A,B,K,L. Speicherstellen: 2,3,8,9,92..95|Gibt die Zeichen im int. RAM von 16..39 auf dem CE-126P aus. Chr(13) bewirkt Zeilenvorschub.|0|CALL	32852|CALL	32852||
	Tastatur
		Getkey|Tastaturabfrage: Geänderte Register: P,Q,DP,I,A,B,X,Y,K,L,V. Speicherstellen: 2..23,92|Wird eine Taste gedrückt, landet der ASCII-Code an Adresse 28503 und im Akku A.|0|CALL	4618|CALL	4618|
		Inkey|Tastaturabfrage: Geänderte Register: P,Q,DP,A,B,X,Y,K,L,V. Speicherstellen: 10..21,23,34,37..39,55..63,92|Wird eine Taste gedrückt, landet der Tastencode an [++Y].|0|CALL	3003|CALL	3003|
	Sonstiges
		Rechner aus|OFF: Geänderte Register: alle. Speicherstellen: alle|Schaltet den Rechner softwareseitig aus. Anschalten über On/Brk.|0|CALL	1240|CALL	1240|
	Systemvariablen
		A|Adresse von Variable A||0|27896|27896|
		B|Adresse von Variable B||0|27888|27888|
		C|Adresse von Variable C||0|27880|27880|
		D|Adresse von Variable D||0|27872|27872|
		E|Adresse von Variable E||0|27864|27864|
		F|Adresse von Variable F||0|27856|27856|
		G|Adresse von Variable G||0|27848|27848|
		H|Adresse von Variable H||0|27840|27840|
		I|Adresse von Variable I||0|27832|27832|
		J|Adresse von Variable J||0|27824|27824|
		K|Adresse von Variable K||0|27816|27816|
		L|Adresse von Variable L||0|27808|27808|
		M|Adresse von Variable M||0|27800|27800|
		N|Adresse von Variable N||0|27792|27792|
		O|Adresse von Variable O||0|27784|27784|
		P|Adresse von Variable P||0|27776|27776|
		Q|Adresse von Variable Q||0|27768|27768|
		R|Adresse von Variable R||0|27760|27760|
		S|Adresse von Variable S||0|27752|27752|
		T|Adresse von Variable T||0|27744|27744|
		U|Adresse von Variable U||0|27736|27736|
		V|Adresse von Variable V||0|27728|27728|
		W|Adresse von Variable W||0|27720|27720|
		X|Adresse von Variable X||0|27712|27712|
		Y|Adresse von Variable Y||0|27704|27704|
		Z|Adresse von Variable Z||0|27696|27696|
	Speicherstellen
		Basicstartadresse|Basicstartadresse Version 1=LB 2=HB||0|28418|28417|
		Basicendadresse|Basicendadresse Version 1=LB 0=HB||0|28420|28419|
		Mergeadresse|Mergeadresse Version 1=LB 0=HB||0|28422|28421|
		WAIT Wert|WAIT Wert Version 1=LB 0=HB||0|28852|28851|
		WAIT Status|WAIT Status: Wert 2=aktiv, 6=aus||0|28435|28435|
		Variablenbeginn|Beginn der selbstdefinierten Variablen Version 1=LB 0=HB||0|28424|28423|
		Reservemodus|Reservemodusbereich Version 1=Startbyte 0=Letztes Byte||0|28671|28527|
		Reservemoduslänge|Reservemoduslänge||0|28526|28526|
		Eingabezwischenspeicher|Eingabezwischenspeicher Version 1=Startbyte 0=Letztes Byte||0|28415|28336|
		Letzte gedrückte Taste|Letzte gedrückte Taste||0|28503|28503|
		Displayspeicher|Displayspeicher im ASCII Format Version 1=Startbyte 0=Letztes Byte||0|27999|27904|
		Passwort|Passwort im ASCII Code Version 1=Startbyte 0=Letztes Byte||0|28432|28426|
		Passwortstatus|Passwortstatus: Bit 5=Passwort aktiv||0|28436|28436|
		Displayspeicher System 1|Displayspeicher System Spalte 1 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|28736|28672|
		Displayspeicher System 2|Displayspeicher System Spalte 2 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|29248|29184|
		Displayspeicher System 3|Displayspeicher System Spalte 3 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|29760|29696|
		Displayspeicher System 4|Displayspeicher System Spalte 4 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|30272|30208|
		Displayspeicher System 5|Displayspeicher System Spalte 5 Version 1=1./3. Zeile-Bereich 0=2./4. Zeile-Bereich|Jeder Bereich ist 60 Bytes lang. Je 30 davon gehören den zwei genannten Zeilen.|0|30784|30720|
		Sondersymbole|Das Sondersymbolbyte|Bit 0=SHIFT\nBit 1=DEF\nBit 4=RUN\nBit 5=PRO\nBit 6=JAPAN\nBit 7=SML|0|30780|30780|
		B-Port Ansteuerung|B-Port Ansteuerung Ein-und Ausgabe: Bit 4=Pin 11, Bit 5=Pin 10, Bit 6=Pin 9, Bit 7=Pin 8 am Druckeranschluss\nVersion 1=Ausgabe, 0=Eingabe||0|INB|LIP	&5D\nORIM	bit\nOUTB|
		F-Port Ansteuerung|F-Port Ansteuerung nur Ausgabe: Bit 1=Pin 5 und Bit 2=Pin 4 am 11-poligen Anschluss||0|LIP	&5E\nORIM	bit\nOUTF|LIP	&5E\nORIM	bit\nOUTF|
PC-1401/02 Makros
	ASM Grundmakros
		LD  Y  <- X|PC1401/02  LD Y,X: CPU Registerinhalt (kein BCD) umladen|Resultat: Y:=X, P=8, Q=6|0|LP	6				# LD Y,X\nLIQ	4\nMVB|CAL	&15EE				# LD Y,X|
		LD  X  <- Y|PC1401/02  LD X,Y: CPU Registerinhalt (kein BCD) umladen|Resultat: X:=Y, P=6, Q=8|0|LP	4				# LD X,Y\nLIQ	6\nMVB|CAL	&15F3				# LD X,Y|
		LD (P) <- X|PC1401/02  LD (P),X: CPU Registerinhalt (kein BCD) umladen|Resultat: (P):=X, P=P+2, Q=6|0|LIQ	4				# LD (P),X\nMVB|CAL	&15EF				# LD (P),X|
		LD (P) <- Y|PC1401/02  LD (P),Y: CPU Registerinhalt (kein BCD) umladen|Resultat: (P):=Y, P=P+2, Q=8|0|LIQ	6				# LD (P),Y\nMVB|CAL	&15F4				# LD (P),Y|
		LD  X  <- BA|PC1401/02  LD X,BA: CPU Registerinhalt (kein BCD) umladen|Resultat: X:=BA, P=6, Q=4|0|LP	4				# LD X,BA\nLIQ	2\nMVB|CAL	&267				# LD X,BA|
		LD  Y  <- BA|PC1401/02  LD Y,BA: CPU Registerinhalt (kein BCD) umladen|Resultat: Y:=BA, P=8, Q=4|0|LP	6				# LD Y,BA\nLIQ	2\nMVB|CAL	&264				# LD Y,BA|
		LD (P) <- BA|PC1401/02  LD (P),BA: CPU Registerinhalt (kein BCD) umladen|Resultat: (P):=BA, P=P+2, Q=4|0|LIQ	2				# LD (P),BA\nMVB|CAL	&268				# LD (P),BA|
		PUSH X|PC-1401/02  PUSH X: CPU Registerinhalt (kein BCD) in den Stack laden|Resultat: geaendert P, Q, A:=R(neu)|0|LDR					# PUSH X\nDECA\nDECA	\nSTP\nLIQ	4\nMVB\n# wenn als Sub: STQ EXB\nSTR|CAL	&1321				# PUSH X|
		POP X|PC-1401/02  POP X: CPU Registerinhalt (kein BCD) aus dem Stack laden|Resultat: geaendert P, Q, A:=R(neu), B bei CAL-Version|0|LDR					# POP X, nicht als Sub nutzen\nLP	4\nSTQ	\nMVB\nINCA\nINCA	\nSTR|CAL	&132C				# POP X|
	BASIC
		Basic Main|PC-1401/02 MAIN: BASIC-Interpreter Stack rücksetzen|Alle offenen GOSUB/RETURN und FOR/NEXT-Anweisungen zurücksetzen|0|# CALL	&D031|CALL	&D031				# BASIC:Main|
	Display
		Display aus|PC-1401/02 Display ausschalten|Der Bildschirm wird stromlos geschaltet.|0|# CAL	&59E|CAL	&59E				# Display off|
		Display an|PC-1401/02 Display anschalten|Der Bildschirm wird angeschaltet.|0|# CAL	&5A2|CAL	&5A2				# Display on|
		Display und warten|PC-1401/02 Display anschalten und warten|Der Bildschirm wird angeschaltet. Das weitere Verhalten entspricht dem Verhalten des PC nach einer PRINT-Anweisung. |0|# CALL	&C53E|CALL	&C53E				# Display PRINT|
		Display aus STR$|PC-1401/02 PRINT Grafische Umwandlung und Ausgabe der ASCII-Zeichen im CPU-RAM &10-&1F|Veraendert: P,Q,DP, A,B,X,Y,L,W,V,RC,RD|0|# CALL	&8015|CALL	&8015				# PRINT Display aus STR$|
	Drucker
		Zeile drucken|PC-1401/02: Eine Zeile drucken: Die Daten kommen in die Operationsregister X (&10-&17), Y (&18-&1F) und Z (&20-&27)|Die 24 Bytes werden gedruckt und eine Zeile weitergeschoben. Veraendert REG: P,Q, A,B,K,L|0|# CALL	&804E|CALL	&804E				# LPRINT|
	Kassette
		Remote start|PC-1401/02: Kassettenrecorder freigeben|Die Kassette wird aufgenommen oder abgespielt. Veraendert: P,DP, A,B Eingesetztes C-Flag wird bei einem Fehler zurückübergeben.|0|# CALL	&803F|CALL	&803F				# TAPE ON|
		Remote stop|PC-1401/02: Kassettenrecorder anhalten|Der Kassettenrecorder wird angehalten. Veraendert: P,DP, A,B Eingesetztes C-Flag wird bei einem Fehler zurückübergeben.|0|# CALL	&8042|CALL	&8042				# TAPE OFF|
	Sonstiges
		PC ausschalten|PC-1401/02 ausschalten|Der PC wird ausgeschaltet.|0|# CAL	?|CAL	&59A                  # PC ausschalten|
		RAM Bank umschalten|PC-1401/02: Zusätzlich eingebaute RAM Bank umschalten - Holger Meyer Pocket Computer Service|Der Schalter Vergleichsart kennzeichnet Zahl = Bank A und String = Bank B.|1|LIDP	&E800			# Schalter RAM Bank A\nSTD|LIDP	&E800			# Schalter RAM Bank A\nSTD|LIDP	&F000			# Schalter RAM Bank B\nSTD|LIDP	&F000			# Schalter RAM Bank B\nSTD|
PC-1403 Makros
	Display
		Display an|PC-1403 Display anschalten, nach PRINT, etc. wird das Display sonst abgeschaltet|Der Bildschirm wird wieder angeschaltet.|0|CAL	1208				# Display on|CAL	1208				# Display on|
	Sonstiges
		PC ausschalten|PC-1403 ausschalten|Der PC wird ausgeschaltet.|0|CAL	1251				# PC ausschalten|CAL	1251				# PC ausschalten|
	Systemvariablen
		A|Adresse von Variable A||0|64472|64472|
		B|Adresse von Variable B||0|64464|64464|
		C|Adresse von Variable C||0|64456|64456|
		D|Adresse von Variable D||0|64448|64448|
		E|Adresse von Variable E||0|64440|64440|
		F|Adresse von Variable F||0|64432|64432|
		G|Adresse von Variable G||0|64424|64424|
		H|Adresse von Variable H||0|64416|64416|
		I|Adresse von Variable I||0|64408|64408|
		J|Adresse von Variable J||0|64400|64400|
		K|Adresse von Variable K||0|64392|64392|
		L|Adresse von Variable L||0|64384|64384|
		M|Adresse von Variable M||0|64376|64376|
		N|Adresse von Variable N||0|64368|64368|
		O|Adresse von Variable O||0|64360|64360|
		P|Adresse von Variable P||0|64352|64352|
		Q|Adresse von Variable Q||0|64344|64344|
		R|Adresse von Variable R||0|64336|64336|
		S|Adresse von Variable S||0|64328|64328|
		T|Adresse von Variable T||0|64320|64320|
		U|Adresse von Variable U||0|64312|64312|
		V|Adresse von Variable V||0|64304|64304|
		W|Adresse von Variable W||0|64296|64296|
		X|Adresse von Variable X||0|64288|64288|
		Y|Adresse von Variable Y||0|64280|64280|
		Z|Adresse von Variable Z||0|64272|64272|
	Speicherstellen
		Basicstartadresse|Basicstartadresse Version 1=LB 2=HB||0|65282|65281|
		Basicendadresse|Basicendadresse Version 1=LB 0=HB||0|65284|65283|
		Mergeadresse|Mergeadresse Version 1=LB 0=HB||0|65286|65285|
		WAIT Wert|WAIT Wert Version 1=LB 0=HB||0|65352|65351|
		WAIT Status|WAIT Status: Wert 2=aktiv, 6=aus||0|65299|65299|
		Variablenbeginn|Beginn der selbstdefinierten Variablen Version 1=LB 0=HB||0|65288|65287|
		Eingabezwischenspeicher|Eingabezwischenspeicher Version 1=Startbyte 0=Letztes Byte||0|65279|65200|
		Displayspeicher|Displayspeicher im ASCII Format Version 1=Startbyte 0=Letztes Byte||0|65143|65120|
		Passwort|Passwort im ASCII Code Version 1=Startbyte 0=Letztes Byte||0|65296|65290|
		Passwortstatus|Passwortstatus: Bit 5=Passwort aktiv||0|65300|65300|
		Displayspeicher erste 12 Zeichen|Displayspeicher erste 12 Zeichen Version 1=Zeichen 1-6, 0=Zeichen 7-12|Jedes Zeichen entspricht 5 Bytes! Daher ist ein 6-Zeichen-Block 30 Bytes lang.|0|12333|12288|
		Displayspeicher zweite 12 Zeichen|Displayspeicher zweite 12 Zeichen Version 1=Zeichen 13-18, 0=Zeichen 19-24|Jedes Zeichen entspricht 5 Bytes! Daher ist ein 6-Zeichen-Block 30 Bytes lang. !Achtung! Diese Zeichen werden rückwärts gezeichnet: Das erste Byte ist daher die letzte Pixelspalte!|0|12377|12392|
		Sondersymbole 1|Das Sondersymbolbyte für µt, /l\, SML, STAT, MATRIX, rechts von PRO und links von CAL|Bit 0=µt\nBit 1=/l\\nBit 2=SML\nBit 3=STAT\nBit 4=MATRIX\nBit 5=rechts von PRO\nBit 6=links von CAL|0|12604|12348|
		Sondersymbole 2|Das Sondersymbolbyte für BUSY, DEF, SHIFT, HYP, PRO, RUN und CAL|Bit 0=BUSY\nBit 1=DEF\nBit 2=SHIFT\nBit 3=HYP\nBit 4=PRO\nBit 5=RUN\nBit 6=CAL|0|12605|12349|
		Sondersymbole 3|Das Sondersymbolbyte für E, M, (), RAD, G, DE und PRINT|Bit 0=E\nBit 1=M\nBit 2=()\nBit 3=RAD\nBit 4=G\nBit 5=DE\nBit 6=PRINT|0|12668|12412|
		Portadresse Ausgabe|Ansteuerungsadresse für Ausgabe am 11-poligen Interface|Bit 0=Pin 11\nBit 1=Pin 10\nBit 2=Pin 5\nBit 3=Pin 8|0|&3A00|&3A00|
		Portadresse Eingabe|Ansteuerungsadresse für Eingabe am 11-poligen Interface|Bit 7=Pin 9|0|&3C00|&3C00|
		B-Port Ansteuerung|B-Port Ansteuerung Ein-und Ausgabe: Bit 7=Pin 8 am Druckeranschluss\nVersion 1=Ausgabe, 0=Eingabe||0|INB|LIP	&5D\nORIM	128\nOUTB|
		F-Port Ansteuerung|F-Port Ansteuerung nur Ausgabe: Bit 2=Pin 4 am 11-poligen Anschluss||0|LIP	&5E\nORIM	4\nOUTF|LIP	&5E\nORIM	4\nOUTF|
PC-1475 Makros
	Systemvariablen
		A|Adresse von Variable A||0|64152|64152|
		B|Adresse von Variable B||0|64144|64144|
		C|Adresse von Variable C||0|64136|64136|
		D|Adresse von Variable D||0|64128|64128|
		E|Adresse von Variable E||0|64120|64120|
		F|Adresse von Variable F||0|64112|64112|
		G|Adresse von Variable G||0|64104|64104|
		H|Adresse von Variable H||0|64096|64096|
		I|Adresse von Variable I||0|64088|64088|
		J|Adresse von Variable J||0|64080|64080|
		K|Adresse von Variable K||0|64072|64072|
		L|Adresse von Variable L||0|64064|64064|
		M|Adresse von Variable M||0|64056|64056|
		N|Adresse von Variable N||0|64048|64048|
		O|Adresse von Variable O||0|64040|64040|
		P|Adresse von Variable P||0|64032|64032|
		Q|Adresse von Variable Q||0|64024|64024|
		R|Adresse von Variable R||0|64016|64016|
		S|Adresse von Variable S||0|64008|64008|
		T|Adresse von Variable T||0|64000|64000|
		U|Adresse von Variable U||0|63992|63992|
		V|Adresse von Variable V||0|63984|63984|
		W|Adresse von Variable W||0|63976|63976|
		X|Adresse von Variable X||0|63968|63968|
		Y|Adresse von Variable Y||0|63960|63960|
		Z|Adresse von Variable Z||0|63952|63952|
	Speicherstellen
		Basicstartadresse|Basicstartadresse Version 1=LB 2=HB||0|&FFD8|&FFD7|
		Basicendadresse|Basicendadresse Version 1=LB 0=HB||0|&FFDA|&FFD9|
		Mergeadresse|Mergeadresse Version 1=LB 0=HB||0|&FFDC|&FFDB|
		WAIT Wert|WAIT Wert Version 1=LB 0=HB||0|&2A7F|&2A7E|
		Zeichen am Cursor|ASCII Code des Zeichens unter dem Cursor||0|&FF02|&FF02|
		Adresse vom Cursor|Adresse des Cursors im Displayspeicher Version 1=LB 0=HB||0|&FF01|&FF00|
		Variablenbeginn|Beginn der selbstdefinierten Variablen Version 1=LB 0=HB||0|&FFDE|&FFDD|
		RAM Karten Startadresse|RAM Karten Startadresse Version 1=LB 0=HB||0|&FFF7|&FFF6|
		RAM Karten Endadresse|RAM Karten Endadresse Version 1=LB 0=HB||0|&FFF9|&FFF8|
		Reservemodus|Reservemodusbereich Version 1=Startbyte 0=Letztes Byte||0|&FFCF|&FF40|
		Reservemoduslänge|Reservemoduslänge||0|&FF3F|&FF3F|
		Eingabezwischenspeicher|Eingabezwischenspeicher Version 1=Startbyte 0=Letztes Byte||0|&FD6F|&FD20|
		Displayspeicher|Displayspeicher im ASCII Format Version 1=Startbyte 0=Letztes Byte||0|&FEAF|&FE80|
		Stringzwischenspeicher|Stringzwischenspeicher Version 1=Startbyte 0=Letztes Byte||0|&FCDF|&FC90|
		Displayspeicher erste Zeile|Displayspeicher erste Zeile Version 1=Zeichen 1-12, 0=Zeichen 13-24|Jedes Zeichen entspricht 5 Bytes! Daher ist ein 12-Zeichen-Block 60 Bytes lang.|0|10752|10240|
		Displayspeicher zweite Zeile|Displayspeicher zweite Zeile Version 1=Zeichen 1-12, 0=Zeichen 13-24|Jedes Zeichen entspricht 5 Bytes! Daher ist ein 12-Zeichen-Block 60 Bytes lang.|0|10816|10304|
		Sondersymbole 1|Das Sondersymbolbyte für BATT, (), HYP, RSV, PRO, RUN und CAL|Bit 0=BATT\NBit 1=()\NBit 2=HYP\NBit 3=RSV\NBit 4=PRO\NBit 5=RUN\NBit 6=CAL|0|10556|10300|||
		Sondersymbole 2|Das Sondersymbolbyte für BUSY, SHIFT, SHIFT und DBL|Bit 0=BUSY\nBit 1=DEF\nBit 2=SHIFT\nBit 3=DBL|0|10557|10301|
		Sondersymbole 3|Das Sondersymbolbyte für E, M, RAD, G, MATRIX, STAT und PRINT|Bit 0=E\nBit 1=M\nBit 2=RAD\nBit 3=G\nBit 4=MATRIX\nBit 5=STAT, Bit 6=PRINT|0|10620|10364|
		Sondersymbole 4|Das Sondersymbolbyte für japan. Symbole, SML und DE|Bit 0=µt\nBit 1=/l\\nBit 2=SML\nBit 3=DE|0|10621|10365|
		Portadresse seriell|Ansteuerungsadresse für Ausgabe am 15-poligen Interface|Bit 0=Pin 4\nBit 1=Pin 14\nBit 2=Pin 11|0|&3800|&3800|
		Portadresse Drucker|Ansteuerungsadresse für Ausgabe am 11-poligen Interface|Bit 0=Pin 11\nBit 1=Pin 10\nBit 2=Pin 5\nBit 3=Pin 8|0|&3A00|&3A00|
		B-Port Ansteuerung|B-Port Ansteuerung Ausgabe: Bit 4=Pin 9 am Druckeranschluss, Bit 5=Pin 3, Bit 6=Pin 5 und Bit 7=Pin 8 am seriellen Anschluss\nVersion 1=Ausgabe, 0=Eingabe|Eingabe: Bit 3=Pin 8 und Bit 4=Pin 9 am Druckeranschluss, Bit 5=Pin 3, Bit 6=Pin 5 und Bit 7=Pin 8 am seriellen Anschluss|0|INB|LIP	&5D\nORIM	bit\nOUTB|
		F-Port Ansteuerung|F-Port Ansteuerung nur Ausgabe: Bit 1=Pin 2 am seriellen Anschluss||0|LIP	&5E\nORIM	2\nOUTF|LIP	&5E\nORIM	2\nOUTF|
