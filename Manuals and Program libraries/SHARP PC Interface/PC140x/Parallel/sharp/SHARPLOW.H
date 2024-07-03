/*********************************************************
*
*       sharplow.h
*
*       Header-Datei fr das Assembler-Interface sharplow.asm,
*       das die elementaren Routinen zum Einlesen von Programmen
*       des Sharp PC1401/2/3 enth„lt.
*
*       01-Nov-1988     Norbert Unterberg
*
**********************************************************/

/*----------> Datenstrukturen */

/*
Warnung: Žnderungen an dieser Struktur mssen immer auch in
         Sharplow.asm bercksichtigt werden! */

typedef struct {
        char Sharpname [8]; /* Dateiname (7 Zeichen) */
        char Password [8];  /* Password, wenn angegeben */
        byte *Buffer;       /* Dateipuffer */
        int Laenge;         /* L„nge der Datei */
        byte Dateityp;      /* Dateityp */
        byte gesichert;     /* Flag, ob Datei gesichert wurde */
    } tHEADER;

/*----------> Konstanten */

#define S_LEER      0x00        /* keine Datei geladen          */
#define S_BASIC     0x70        /* Dateityp Basic               */
#define S_BASIC_P   0x71        /*          Basic mit Password  */
#define S_DATEN     0x74        /*          Datendatei          */
#define S_BINAER    0x76        /*          Bin„rdatei          */
#define S_DUMP      0xff        /* Dump - Lade alles zu Testen  */

/*----------> Funktonsprototypen */

int  InitSharp (int port);      /* port = 0 .. 3 (LPT1 .. LPT4) */
int  HoleSharp (tHEADER *);
int  SendeSharp (tHEADER *);
void Cls (void);


/*----------> externe Variable */

extern int TimeoutReason;
