/*--------------------------------------------------------------------
        sharp.c

        Dieses Programm liest Daten (BASIC-Programme) von dem
        Pocket-Computer Sharp PC 1401/2/3 ein.

        NÑheres, technische Einzelheiten usw. siehe Kommentare in der
        Datei sharplow.asm.

        Norbert Unterberg


        Programmgeschichte
        ------------------

        Datum       Vers    Bemerkung

        15.04.1989  1.05    Problem "deg" <--> "degree" beseitigt durch Um-
                            drehen der ZÑhlrichtung in der Schleife in
                            tokenisiere. Dadurch wird zuerst mit "degree"
                            verglichen, und erst wenn das nicht erkannt wurde,
                            wird "deg" gefunden.

        11.04.1989  1.04    In der Assembler-Datei I/O-Waits eingebaut, da
                            Probleme auf einem schnellen 386iger

        17.03.1989  1.03    MenÅ erscheint nur bei falschen Eingaben

        25.02.1989          Verzeichnis etwas verbessert (es reicht jetzt
                            die Angabe eines Verzeichnisnamens)

        03.02.1989  1.02    8. Menupunkt eingefÅhrt: Anzeige des
                            Dateiverzeichnis

        19.01.1989          Versionsnummer als Konstante eingefÅhrt. Bei
                            jeder énderung des Programms soll die 2. Stelle
                            nach dem Komma um 1 erhîht werden.

        16.12.1988  1.01    Der voreingestellte Druckerport ist der mit
                            der hîchsten Nummer, der gefunden werden kann
                            (Bios-Datenbereich Adressen 40:08 bis 40:0f)

        01.11.1988  1.00    Erste vollstÑndige Version

  --------------------------------------------------------------------*/

#define VERS "1.05"
#define DATE "15.04.1989"

#ifdef __TURBOC__
#pragma warn -pia       /* "Possibly incorrect assignment" unterdrÅcken */
#pragma warn -ucp       /* "Mixing pointers to signed and unsigned char" */
#endif

typedef unsigned char byte;

#include "sharplow.h"

#define MAX_MEM 32000           /* Maximale Grî·e des BASIC-Programms */
#define MAX_ZEILE 80
#define MAX_BASICZEILE 0xfeff

/*-----> globale Variablen */
byte buffer [MAX_MEM];          /* Puffer fÅrs BASIC-Programm */
char eingabe[MAX_ZEILE], *eing; /* Eingabepuffer fÅr Benutzereingaben */

char *fehler[] = {
        "Interner Programmfehler",
        "Timeout bei GetBit",
        "Kein Signal auf der Leitung",
        "Timeout bei der Synchronisation",
        "Unbekanntes ID-Byte im Header",
        "Falsche Header-PrÅfsumme",
        "Falsche Daten-PrÅfsumme",
        "Unerwartetes öbertragungsende",
        "Wo bleibt der Header?"
    };

/*-----> Das sind die Basic-Befehle des Sharp PC-1401 in der Reihenfolge
         ihrer Tokens, angefangen bei 0x80. Neue Tokens (andere
         Sharp-Modelle?) an der entsprechenden Stelle einfÅgen. */
char *tokens[] = {
  "",       "rec",    "pol",    "rot",    "deci",   "hex",    "ten",   "rcp",
  "squ",    "cur",    "hsn",    "hsk",    "htn",    "ahs",    "ahc",   "aht",
  "fact",   "ln",     "log",    "exp",    "sqr",    "sin",    "cos",   "tan",
  "int",    "abs",    "sgn",    "deg",    "dma",    "asn",    "acs",   "atn",
  "rnd",    "and",    "or",     "not",    "asc",    "val",    "len",   "peek",
  "chr$",   "str$",   "mid$",   "left$",  "right$", "inkey$", "pi",    "mem",
  "run",    "new",    "cont",   "pass",   "list",   "llist",  "csave", "cload",
  "",       "",       "",       "",       "",       "",       "",      "",
  "random", "degree", "radian", "grad",   "beep",   "wait",   "goto",  "tron",
  "troff",  "clear",  "using",  "dim",    "call",   "poke",   "",      "",
  "to",     "step",   "then",   "on",     "if",     "for",    "let",   "rem",
  "end",    "next",   "stop",   "read",   "data",   "pause",  "print", "input",
  "gosub",  "aread",  "lprint", "return", "restore"
  };
#define MAX_TOKEN (sizeof (tokens) / sizeof (*tokens))

#include "sharp.h"

/*--------------------------------------------------------------------
  listprogramm

  Das Basicprogramm im Speicher wird in die Datei dateiname gelistet
  (der Bildschirm hat den Namen "CON"!).
  prog ist der Zeiger auf die erste Zeile. Die Angabe von len (in
  Bytes) ist nîtig, damit bei einer fehlenden Ende-Marke im Programm
  kein MÅll ausgegeben wird.
  --------------------------------------------------------------------*/
void listprogramm (dateiname, prog, len)
char *dateiname;
byte *prog;
unsigned int len;
{
    int zeile, space;
    byte b, *end;
    FILE *fd;

    if ((fd = fopen (dateiname, "w")) == NULL)
        return;

    end = prog + len;
    while (*prog != 0xff && prog < end) {
        b = *prog++;
        zeile = b * 256 + *prog++;              /* Zeilennummer */
        fprintf (fd, "%-6u", zeile);            /* linksbÅndig, 6 Stellen */
        space = 0;
        prog++;                                 /* Åberspringe ZeilenlÑnge */
        while ((b = *prog++) != 0x0d && prog < end) {  /* 0D = Zeilenende */

            if ((b & 0x80) && (b < MAX_TOKEN+128)) {
                /* Zeichen ist ein Token */
                b &= 0x7f;
                if (space && (b > 47 || (b >= 33 && b <= 34)))
                    fputc (' ', fd);
                if (b < MAX_TOKEN && *tokens[b])
                    fprintf (fd, tokens [b]);
                else
                    fprintf (fd, "??=%02x", b);
                fputc (' ', fd);
                space = 0;
            }
            else {
                if (b == 0xfb)
                    b = '„';            /* PI-Zeichen */
                else if (b == 0xfc)
                    b = '˚';            /* Wurzel-Zeichen */
                fputc (b, fd);
                if (b == ':')
                    space = 0;
                else
                    space = 1;
            }  /*  if  */
        }   /*  while  */
        fputc ('\n', fd);
    }  /*  while  */
    fclose (fd);
}


/*--------------------------------------------------------------------
  holezeile

  Stammt aus der Zeit, als ich "fgets" noch nicht kannte. Ist
  kompatibel mit "fgets (eingabe, max, fp);"
  --------------------------------------------------------------------*/
void holezeile (FILE *fp, char eingabe[], int max)
{
    int len;

    if (fgets (eingabe, max, fp) == NULL)
        eingabe[0] = '\0';
    /* entferne ÅberflÅssiges Return, wenn vorhanden */
    len = strlen (eingabe)-1;
    if (eingabe[len] == '\n')
        eingabe[len] = '\0';

    /* Das war das alte...
    int c, i;

    i = 0;
    while (i < max && (c = getc (fp)) != '\n' && c != EOF)
        eingabe [i++] = (char) c;
    eingabe [i] = '\0';
    while (c != '\n' && c != EOF)
        c = getc (fp);
    */
}


/*--------------------------------------------------------------------
  janein

  Stellt den User vor eine schwerwiegende Entscheidung: Ja oder Nein
  --------------------------------------------------------------------*/
int janein ()
{
    char s[10];

    do {
        holezeile (stdin, eingabe, MAX_ZEILE);
        if (sscanf (eingabe, "%s", s) == 0)
            *s = '\0';
    } while (!(*s == 'J' || *s == 'j' || *s == 'n' || *s == 'N'));
    return (*s == 'J' || *s == 'j');
}

/*--------------------------------------------------------------------
  convert

  Konvertiert ein Zeichen vom IBM-Zeichensatz in den
  Sharp-Zeichensatz um
  --------------------------------------------------------------------*/
byte convert (byte b)
{
    switch (b) {
        case '˚':       /* Wurzel */
            return (byte) 0xfc;
        case '„':       /* PI */
            return (byte) 0xfb;
        default:
            return (toupper (b));
    }
}


/*--------------------------------------------------------------------
  convert1

  Konvertiert ein Zeichen vom IBM-Zeichensatz in den
  Sharp-Zeichensatz um. Kleinbuchstaben bleiben klein!
  Diese Funktion ist nur fÅr Strings und Kommentare!
  --------------------------------------------------------------------*/
byte convert1 (byte b)
{
    switch (b) {
        case '˚':       /* Wurzel */
            return (byte) 0xfc;
        case '„':       /* PI */
            return (byte) 0xfb;
        default:
            return (b);
    }
}


/*--------------------------------------------------------------------
  compare

  Vergleicht den Text s mit dem BASIC-Token d.
  Liefert 0 (false), wenn d nicht am Anfang von s vorhanden ist.
  Wenn der Befehl d am Anfang von s vollstÑndig enthalten ist,
  wird die Anzahl der Bytes geliefert, die der Befehl einnimmt.
  Leerzeichen in s werden Åberlesen.
  --------------------------------------------------------------------*/
int compare (register byte *s, register byte *d)
{
    int i = 0;

    while (*d) {
        i++;
        while (isspace (*s)) {
            s++;
            i++;
        }
        if (*s == '\0' || tolower (*s) != tolower(*d))
            return (0);
        s++;
        d++;
    }
    return (i);
}


/*--------------------------------------------------------------------
  tokenisiere

  Diese Funktion erhÑlt eine BASIC-Zeile und legt sie im
  Puffer ab. Dabei wird sie nach der Zeilennummer richtig
  einsortiert.
  --------------------------------------------------------------------*/
int tokenisiere (register byte *z, tHEADER *header)
{
    unsigned int n = 0, linenum;     /* Zeilennummer */
    byte p = 0;             /* Pufferzeiger */
    int i;
    int k;
    byte puffer[81];        /* TemporÑrer Puffer fÅr die BASIC-Zeile */
    byte *basic;            /* Zeiger im Basic-Puffer */

    /* hole Zeilennummer */

    while (isspace (*z))
        z++;
    if (!*z)
        return (0);
    while (isdigit (*z))
        n = n*10 + *z++ - '0';
    if (n == 0 || n > MAX_BASICZEILE)    /* ungÅltige Zeilennummer */
        return (1);

    /* tokenisiere den Rest der Zeile */

    while (*z) {

        /* Achtung: Die Schleife mu· rÅckwÑrts zÑhlen,
           da sonst "deg" und "degree" nicht korrekt erkannt
           werden ("deg" < "degree")  */

        for (i = MAX_TOKEN-1; i >= 0; i--)
            if (*tokens[i] && (k = compare (z, tokens [i])))
                break;

        if (i >= 0) {            /* gefunden? */
            puffer [p++] = i | 0x80;
            z += k;

            if (i == 87) {      /* REM gefunden, kopiere den Rest ungesehen */
                if (*z == ' ')
                    z++;        /* Das Leerzeichen nach dem REM ignorieren */
                while (*z) {
                    puffer [p++] = convert1 (*z);
                    z++;
                }  /* end while */
            }  /* end if */

        } else {
            switch (*z) {
                case '"':
                    do
                        puffer [p++] = convert1 (*z++);
                    while (*z && *z != '"');
                    if (!*z)
                        return (2);     /* Stringende fehlt */
                    puffer [p++] = '"';
                    break;
                case ' ':
                    break;
                default:
                    puffer [p++] = convert (*z);
            } /* switch */
            z++;
        } /* if */
    } /* while */
    puffer [p++] = 0x0d;        /* Zeichen fÅr Zeilenende */

    /* fÅge die Zeile im Programm ein */

    basic = header->Buffer;

    while ((linenum = (unsigned int)*basic *256 + *(basic+1)) < n
        && (basic < header->Buffer + header->Laenge))
        basic += *(basic+2) + 3;    /* gehe zur nÑchsten Zeile */

    if (n == linenum)
        return (3);         /* doppelte Zeilennummer */

    memmove (basic + p + 3, basic, header->Laenge - (basic - header->Buffer));
    memcpy (basic + 3, puffer, p);
    *basic       = (byte) (n >> 8);
    *(basic + 1) = (byte) (n & 0xff);
    *(basic + 2) = p;
    header->Laenge += p+3;
    return (0);     /* Melde fehlerfreie AusfÅhrung */
}


/*--------------------------------------------------------------------
  ladeprogramm

  LÑdt ein Basic-Text in den Speicher. Die Datei mu· bereits
  erfolgreich geîffnet worden sein.
  --------------------------------------------------------------------*/
int ladeprogramm (FILE *fd, tHEADER *header)
{
    int f;
    unsigned i;
    char zeile[200];

    header->Buffer = buffer;
    buffer[0] = 0xff;
    header->Laenge = 1;
    i = 0;
    printf ("Zeile: %4u", i);

    do {
        i++;
        printf ("\b\b\b\b%4u", i);
        holezeile (fd, zeile, 199);
        if (f = tokenisiere (zeile, header)) {
            printf ("\nFehler: ");
            switch (f) {
                case 1:
                    puts ("Zeilennummer zu gro·");
                    break;
                case 2:
                    puts ("GÑnsefÅ·chen (Ende der Zeichenkette) erwartet");
                    break;
                case 3:
                    puts ("Doppelte Zeilennummer");
                    break;
            }
        puts (zeile);
        }
    } while (!(f || feof (fd)));
    putchar ('\n');
    return (f);
}


/*--------------------------------------------------------------------
  pause

  Wartet auf ein RETURN vom Benutzer
  --------------------------------------------------------------------*/
void pause ()
{
    printf ("\nReturn drÅcken");
    holezeile (stdin, eingabe, 1);
}


/*--------------------------------------------------------------------
  strlower

  Macht aus allen Zeichen des Strings s Kleinbuchstaben
  --------------------------------------------------------------------*/
char *strlower (char *s)
{
    char *s1 = s;

    while (*s) {
        *s = tolower (*s);
        s++;
    }
    return (s1);
}

/*--------------------------------------------------------------------
  main

  Das ist das Hauptprogramm (wer hÑtte das gedacht...)
  --------------------------------------------------------------------*/
int main ()
{
    unsigned int len;
    int ende, num, f, lpt, lptneu, init, i, wild;
    char c;
    char name [80], name1 [80];
    FILE *fp;
    tHEADER Header;
    struct ffblk findinfo;


    for (lpt=3; lpt>=0; lpt--) {
        init = InitSharp (lpt);
        if (init) break;
    }
    if (!init) {
        puts ("Fehler bei der Initialisierung: Kein Druckerport gefunden!");
        exit (1);
    }
    Header.gesichert = 1;
    Header.Dateityp = S_LEER;
    Header.Buffer = buffer;
    *Header.Sharpname = '\0';

    do {
        Cls ();
        puts ("\nSharp-PC-DatenÅbertragung");
        puts ("Version "VERS" vom "DATE", Norbert Unterberg\n");
        printf ("Interface am Druckerport LPT%u\n", lpt+1);
        if (Header.Dateityp == S_LEER)
            puts ("Keine Datei im Speicher");
        else {
            printf ("Datei ");
            if (*Header.Sharpname)
                printf ("\"%s\", ", Header.Sharpname);
            else
                printf ("OhneNamen, ");
            switch (Header.Dateityp) {
                case S_BASIC:
                    printf ("BASIC-Programm");
                    break;
                case S_BASIC_P:
                    printf ("BASIC-Programm mit Password \"%s\"",
                             Header.Password);
                    break;
                case S_DATEN:
                    printf ("Daten-Datei");
                    break;
                case S_BINAER:
                    printf ("BinÑr-Datei");
                    break;
                default:
                    Header.Dateityp = S_LEER;
                    printf ("Unbekannter Dateityp");
            }
            printf (", %u Bytes\n", Header.Laenge);
        }
        ende = 0;
        putchar ('\n');
        puts ("1. Daten vom Sharp laden (CSAVE/PRINT #)");
        puts ("2. Daten zum Sharp schicken (CLOAD/INPUT #)");
        puts ("3. Daten auf Diskette speichern");
        puts ("4. Daten von Diskette laden");
        puts ("5. BASIC-Programm (ASCII-Text) von Diskette laden");
        puts ("6. Programm LISTen");
        puts ("7. Druckerport Ñndern");
        puts ("8. Verzeichnis anzeigen");
        puts ("\n9. Ende");

        do {
            printf ("\nBefehl: ");
            holezeile (stdin, eingabe, MAX_ZEILE);
            putchar ('\n');
            if (sscanf (eingabe, "%d", &num) != 1)
                num = 0;                    /* nichts */

            switch (num) {
                case 1:
                    /* CSAVE */
                    if (!Header.gesichert) {
                        printf ("Datei nicht gespeichert. Trotzdem laden? (J/N): ");
                        if (!janein ())
                            break;
                    }
                    printf ("Zum Laden bitte RETURN drÅcken...");
                    holezeile (stdin, eingabe, 2);
                    puts ("\nLade Datei...");
                    if (f = HoleSharp (&Header)) {
                        printf ("\nFehler beim Laden: %s\n", fehler[f]);
                        *Header.Sharpname = '\0';
                        break;
                    }
                    Header.gesichert = 0;
                    break;

                case 2:
                    /* CLOAD */

                    if (Header.Dateityp != S_LEER) {
                        printf ("Zum Senden bitte RETURN drÅcken...");
                        holezeile (stdin, eingabe, 2);
                        printf ("Sende Datei\n");
                        SendeSharp (&Header);
                    } else {
                        puts ("Keine Datei im Speicher!");
                    }
                    break;

                case 3:
                    /* Schreibe auf Disk */

                    name[0] = '\0';
                    if (Header.Dateityp == S_LEER) {
                        puts ("Keine Datei im Speicher!");
                        break;
                    }
                    printf ("Datei sichern als");
                    if (*Header.Sharpname) {
                        strcpy (name, Header.Sharpname);
                        switch (Header.Dateityp) {
                            case S_BASIC:
                            case S_BASIC_P:
                                strcat (name, ".BAS");
                                break;
                            case S_BINAER:
                                strcat (name, ".BIN");
                                break;
                            case S_DATEN:
                                strcat (name, ".DAT");
                        }
                        printf (" [%s]", name);
                    }

                    printf (": ");
                    holezeile (stdin, eingabe, MAX_ZEILE);
                    if (sscanf (eingabe, "%s", name1) == 1)
                        strcpy (name, name1);
                    if (*name) {
                        printf ("Schreibe Datei: %s\n", name);
                        if ((fp = fopen (name, "wb")) == NULL)
                            puts ("Fehler beim ôffnen der Datei");
                        else {
                            fwrite (&Header, sizeof (Header), 1, fp);  /* Header */
                            fwrite (Header.Buffer, sizeof (byte), Header.Laenge, fp);
                            if (ferror (fp))
                                puts ("Fehler beim Schreiben der Datei");
                            else {
                                Header.gesichert = 1;
                                puts ("Datei ist gesichert");
                            }
                            fclose (fp);
                        }
                    }
                    break;

                case 4:
                    /* Lade von Disk */

                    if (!Header.gesichert) {
                        printf ("Die Datei ist nicht gesichert. Trotzdem laden? (J/N): ");
                        if (!janein ())
                            break;
                    }

                    printf ("Welche Datei laden: ");
                    holezeile (stdin, eingabe, MAX_ZEILE);
                    if (sscanf (eingabe, "%s", name) == 1) {
                        printf ("Lade Datei: %s\n", name);
                        if ((fp = fopen (name, "rb")) == NULL)
                            puts ("Fehler beim ôffnen der Datei");
                        else {
                            byte t;
                            tHEADER header1;
                            if (fread (&header1, sizeof (header1), 1, fp) != 1
                                || ((t = header1.Dateityp) != S_DATEN
                                    && t != S_BASIC
                                    && t != S_BASIC_P
                                    && t != S_BINAER
                                    && t != S_DUMP))
                                puts ("Falsches Dateiformat!");
                            else {
                                len = fread (buffer,
                                             sizeof (byte),
                                             header1.Laenge, fp);
                                if (ferror (fp))
                                    puts ("Fehler beim Lesen der Datei");
                                else {
                                    Header = header1;
                                    Header.Laenge = len;
                                    Header.Buffer = buffer;
                                    Header.gesichert = 1;
                                    puts ("Datei geladen");
                                }
                            }
                            fclose (fp);
                        }
                    }
                    break;

                case 5:
                    /* BASIC-ASCII nach Token konvertieren */

                    if (!Header.gesichert) {
                        printf ("Die Datei ist nicht gesichert. Trotzdem laden? (J/N): ");
                        if (!janein ())
                            break;
                    }

                    printf ("Welchen BASIC-Text laden: ");
                    holezeile (stdin, eingabe, MAX_ZEILE);
                    if (sscanf (eingabe, "%s", name) == 1) {
                        printf ("Lade Datei: %s\n", name);
                        if ((fp = fopen (name, "r")) == NULL)
                            puts ("Fehler beim ôffnen der Datei");
                        else {
                            /* éktschen */
                            if (ladeprogramm (fp, &Header) == 0) {
                                int i=0;
                                char *n = name;
                                Header.Dateityp = S_BASIC;
                                Header.gesichert = 1;
                                while (isalnum (*n) && i < 7) {
                                    Header.Sharpname[i++] = toupper (*n);
                                    n++;
                                }
                                Header.Sharpname[i] = '\0';
                            } else {
                                Header.Dateityp = S_LEER;
                                Header.Laenge = 0;
                                Header.gesichert = 1;
                            }
                            fclose (fp);
                        }
                    }
                    break;

                case 6:
                    /* List */
                    if (Header.Dateityp != S_BASIC && Header.Dateityp != S_BASIC_P) {
                        puts ("Kein Basic-Programm im Speicher!");
                        break;
                    }
                    printf ("Listing schreiben nach [CON]: ");
                    holezeile (stdin, eingabe, MAX_ZEILE);
                    putchar ('\n');
                    if (!*eingabe)
                        listprogramm ("CON", Header.Buffer, Header.Laenge);
                    else
                        listprogramm (eingabe, Header.Buffer, Header.Laenge);

                    break;

                case 7:
                    /* Druckerport Ñndern */

                    printf ("Neuer Druckerport [LPT%u]: LPT", lpt+1);
                    holezeile (stdin, eingabe, 3);
                    if (sscanf (eingabe, "%u", &lptneu) == 1) {
                        if (lptneu > 4 || lptneu < 1 || !InitSharp (lptneu-1))
                            puts ("Den Druckerport gibt es bei Ihrem Rechner nicht!");
                        else
                            lpt = lptneu - 1;
                    }
                    break;

				case 8: {
                    /* Verzeichnis anzeigen */

                    char tmpnam[15];

                    printf ("Verzeichnis-Maske [*.bas]: ");
                    holezeile (stdin, eingabe, MAX_ZEILE);
                    putchar ('\n');
                    if (!*eingabe)
                        strcpy (eingabe, "*.bas");

                    wild = strchr (eingabe, '*') != NULL ||
                           strchr (eingabe, '?') != NULL;

                    if (!wild &&
                       (c = eingabe[strlen(eingabe)-1]) == '\\' || c == ':') {
                        strcat (eingabe, "*.*");
                        wild = 1;
                    }

                    if (!wild && findfirst (eingabe, &findinfo, FA_DIREC) == 0)
                        strcat (eingabe, "\\*.*");

                    i = 0;
                    if (!findfirst (eingabe,
                                    &findinfo,
                                    FA_RDONLY | FA_ARCH | FA_DIREC))
                        do {
                            if (i == 5) {
                                i = 0;
                                putchar ('\n');
                            }
                            if ((findinfo.ff_attrib & FA_DIREC) != 0) {
                                strcat (strcpy (tmpnam, findinfo.ff_name), "\\");
                                printf ("%-15s", tmpnam);
                            } else
                                printf ("%-15s", strlower (findinfo.ff_name));
                            i++;
                        } while (!findnext (&findinfo));
                    else
                        puts ("Keine Datei gefunden!");
                    putchar ('\n');
                    break;
					}
                case 9:
                    /* Ende */
                    if (!Header.gesichert) {
                        printf ("Die Datei ist nicht gesichert. Trotzdem beenden? (J/N): ");
                        if (janein ())
                            ende = 1;
                    }
                    else
                        ende = 1;
                    break;
				default:
					num = 0;
                    break;
            }
		} while (!(num == 0 || ende));
    } while (!ende);
    return (0);
}
