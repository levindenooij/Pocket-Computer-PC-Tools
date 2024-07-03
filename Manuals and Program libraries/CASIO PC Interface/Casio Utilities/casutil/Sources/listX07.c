/*
 *  listX07.c - List Canon X-07 tape files
 *
 *  This program displays the contents of a tape image saved by the Canon X-07 
 *  hand held computer through the sound card
 *
 *  Options:
 *    -b<skip>  read BIN file (Default); skip <skip> garbage bytes
 *    -w        read WAV file
 *
 *  Written by Marcus von Cube
 */
#define DEBUG 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"

/*
 *  Error messages
 */
char *err_msg[] = 
{
    "",                                       /* OK */
    "Premature end of a file",                /* message #1 */
    "Premature end of header segment",        /* message #2 */
    "Header segment expected",                /* message #3 */
    "Missing line terminator",                /* message #4 */
    "Line pointers inconsistent",             /* message #5 */
};

/*
 *  Single byte tokens from 0x80 to 0xFF
 */
char *tokens[] =
{
    "END",      "FOR ",     "NEXT ",    "DATA ",    /* 80 */
    "INPUT",    "DIM ",     "READ ",    "LET ",     /* 84 */
    "GOTO ",    "RUN ",     "IF ",      "RESTORE ", /* 88 */
    "GOSUB ",   "RETURN",   "REM",      "STOP",     /* 8C */
    " ELSE ",   "TR",       "MOTOR ",   "DEFSTR ",  /* 90 */
    "DEFINT ",  "DEFSNG ",  "DEFDBL ",  "LINE ",    /* 94 */
    "ERROR ",   "RESUME ",  "OUT ",     "ON ",      /* 98 */
    "LPRINT",   "DEFFN",    "POKE ",    "PRINT",    /* 9C */
    "CONT",     "LIST ",    "LLIST ",   "CLEAR ",   /* A0 */
    "CIRCLE",   "CONSOLE",  "CLS",      "COLOR ",   /* A4 */
    "EXEC ",    "LOCATE ",  "PSET",     "PRESET",   /* A8 */
    "OFF",      "SLEEP",    "DIR",      "DELETE ",  /* AC */
    "FSET ",    "PAINT ",   "LOAD",     "SAVE",     /* B0 */
    "INIT",     "ERASE ",   "BEEP ",    "CLOAD",    /* B4 */
    "CSAVE",    "NEW",      "TAB(",     " TO ",     /* B8 */
    "FN",       " USING",   "ERL",      " ERROR",   /* BC */
    "STRING$",  "INSTR",    "INKEY$",   "INP",      /* C0 */
    "VARPTR",   "USR",      "SNS",      "ALM$",     /* C4 */
    "DATE$",    "TIME$",    "START$",   "FONT$",    /* C8 */
    "KEY$",     "SCREEN ",  " THEN ",   "NOT ",     /* CC */
    " STEP ",   "+",        "-",        "*",        /* D0 */
    "/",        "^",        " AND ",    " OR ",     /* D4 */
    " XOR ",    " EQU ",    " MOD ",    "\\",       /* D8 */
    ">",        "=",        "<",        "SGN",      /* DC */
    "INT",      "ABS",      "FRE",      "POS",      /* E0 */
    "SQR",      "RND",      "LOG",      "EXP",      /* E4 */
    "COS",      "SIN",      "TAN",      "ATN ",     /* E8 */
    "PEEK",     "CINT",     "CSNG",     "CDBL",     /* EC */
    "FIX",      "LEN",      "HEX$",     "STR$",     /* F0 */
    "VAL",      "ASC",      "CHR$",     "TKEY",     /* F4 */
    "LEFT$",    "RIGHT$",   "MID$",     "CSRLIN",   /* F8 */
    "STICK",    "STRIG",    "POINT",    "'"         /* FC */
};


/*
 *  Escape codes with \
 */
struct _escapes {
    char *text;
    unsigned char token;
} Escapes[] = 
{
    { "\\", 0x5C },  /* backslash */
/*  { "YN", 0x5C },     Yen */
    { "?",  0x7F },  /* reverse ? */
    { "SP", 0x80 },  /* spade */
    { "HT", 0x81 },  /* heart */
    { "DI", 0x82 },  /* diamond */
    { "CL", 0x83 },  /* club */
    { "@",  0x84 },  /* circle */
    { "LD", 0x85 },  /* large dot */
    { ":A", 0x86 },  /* A umlaut */
    { ".A", 0x87 },  /* A circle */
/*  { "AN", 0x87 },     Angstroem */
    { ":a", 0x88 },  /* a umlaut */
    { "`a", 0x89 },  /* a accent grave */
    { "^a", 0x8A },  /* a circonflex */
    { "'a", 0x8B },  /* a accent */
    { ".a", 0x8C },  /* a circle */
    { "_a", 0x8D },  /* a underline */
    { ":I", 0x8E },  /* I umlaut */
    { ":i", 0x8F },  /* i umlaut */
    { "`i", 0x90 },  /* i accent grave */
    { "^i", 0x91 },  /* i circonflex */
    { "'i", 0x92 },  /* i accent */
    { ":U", 0x93 },  /* U umlaut */
    { ":u", 0x94 },  /* u umlaut */
    { "`u", 0x95 },  /* u accent grave */
    { "^u", 0x96 },  /* u circonflex */
    { "'u", 0x97 },  /* u accent */
    { "'E", 0x98 },  /* E accent */
    { ":e", 0x99 },  /* e umlaut */
    { "`e", 0x9A },  /* e accent grave */
    { "^e", 0x9B },  /* e circonflex */
    { "'e", 0x9C },  /* e accent */
    { ":O", 0x9D },  /* O umlaut */
    { ":o", 0x9E },  /* o umlaut */
    { "`o", 0x9F },  /* o accent grave */
    { "RT", 0xA0 },  /* root */
    { "^o", 0xE0 },  /* o circonflex */
    { "'o", 0xE1 },  /* o accent */
    { "_o", 0xE2 },  /* o underline */
    { ":y", 0xE3 },  /* y umlaut */
    { ",C", 0xE4 },  /* C cedille */
    { ",c", 0xE5 },  /* c cedille */
    { "~N", 0xE6 },  /* N tilde */
    { "~n", 0xE7 },  /* n tilde */
    { "GA", 0xE8 },  /* Gamma */
    { "SI", 0xE9 },  /* Sigma */
/*  { "SM", 0xE9 },     Sum */
    { "PI", 0xEA },  /* Pi */
    { "OM", 0xEB },  /* Omega */
    { "al", 0xEC },  /* alpha */
    { "bt", 0xED },  /* beta */
    { "ga", 0xEE },  /* gamma */
    { "dl", 0xEF },  /* delta */
    { "ep", 0xF0 },  /* epsilon */
    { "si", 0xF1 },  /* sigma */
    { "th", 0xF2 },  /* theta */
    { "ka", 0xF3 },  /* kappa */
    { "la", 0xF4 },  /* lambda */
    { "mu", 0xF5 },  /* mu */
    { "rh", 0xF6 },  /* rho */
    { "pi", 0xF7 },  /* pi */
    { "ta", 0xF8 },  /* tau */
    { "ps", 0xF9 },  /* psi */
    { "ch", 0xFA },  /* chi */
    { "om", 0xFB },  /* omega */
    { "nu", 0xFC },  /* nu */
    { "PN", 0xFD },  /* pound */
    { "CN", 0xFE },  /* cent */                     
    { ":-", 0xFF },  /* divide */
    { NULL, 0    }
};

/*
 *  Character set translation tables
 */
typedef struct _translate {
    unsigned char from;
    char *to;
} TRANSLATE;

TRANSLATE ToAscii[] =
{
    { 0x7F, "?"  },
    { 0x84, "0"  },
    { 0x86, "Ae" },
    { 0x87, "A"  },
    { 0x88, "ae" },
    { 0x89, "a"  },
    { 0x8A, "a"  },
    { 0x8B, "a"  },
    { 0x8C, "a"  },
    { 0x8D, "a"  },
    { 0x8E, "I"  },
    { 0x8F, "i"  },
    { 0x90, "i"  },
    { 0x91, "i"  },
    { 0x92, "i"  },
    { 0x93, "Ue" },
    { 0x94, "ue" },
    { 0x95, "u"  },
    { 0x96, "u"  },
    { 0x97, "u"  },
    { 0x98, "E"  },
    { 0x99, "e"  },
    { 0x9A, "e"  },
    { 0x9B, "e"  },
    { 0x9C, "e"  },
    { 0x9D, "Oe" },
    { 0x9E, "oe" },
    { 0x9F, "o"  },
    { 0xA0, "/"  },
    { 0xA2, "|"  },
    { 0xA3, "|"  },
    { 0xA4, "`"  },
    { 0xA5, "+"  },
    { 0xE0, "o"  },
    { 0xE1, "o"  },
    { 0xE2, "o"  },
    { 0xE3, "y"  },
    { 0xE4, "C"  },
    { 0xE5, "c"  },
    { 0xE6, "N"  },
    { 0xE7, "n"  },
    { 0xE8, "G"  },
    { 0xE9, "S"  },
    { 0xEA, "P"  },
    { 0xEB, "O"  },
    { 0xEC, "a"  },
    { 0xED, "ss" },
    { 0xEE, "g"  },
    { 0xEF, "d"  },
    { 0xF0, "e"  },
    { 0xF1, "z"  },
    { 0xF2, "th" },
    { 0xF3, "k"  },
    { 0xF4, "l"  },
    { 0xF5, "u"  },
    { 0xF6, "p"  },
    { 0xF7, "pi" },
    { 0xF8, "t"  },
    { 0xF9, "o"  },
    { 0xFA, "X"  },
    { 0xFB, "w"  },
    { 0xFC, "v"  },
    { 0xFD, "L"  },
    { 0xFE, "c"  },
    { 0xFF, ":"  },
    { 0x00, NULL }
};

TRANSLATE ToDos[] = 
{
    { 0x5C, "\x9D" },
    { 0x7F, "\xA8" },
    { 0x80, "\x06" },
    { 0x81, "\x06" },
    { 0x82, "\x05" },
    { 0x83, "\x04" },
    { 0x84, "\xF8" },
    { 0x85, "\xF9" },
    { 0x86, "\x8E" },
    { 0x87, "\x8F" },
    { 0x88, "\x84" },
    { 0x89, "\x85" },
    { 0x8A, "\x83" },
    { 0x8B, "\xA0" },
    { 0x8C, "\x86" },
    { 0x8D, "\xA6" },
    { 0x8F, "\x8B" },
    { 0x90, "\x8D" },
    { 0x91, "\x8C" },
    { 0x92, "\xA1" },
    { 0x93, "\x9A" },
    { 0x94, "\x81" },
    { 0x95, "\x97" },
    { 0x96, "\x96" },
    { 0x97, "\xA3" },
    { 0x98, "\x90" },
    { 0x99, "\x89" },
    { 0x9A, "\x8A" },
    { 0x9B, "\x88" },
    { 0x9C, "\x82" },
    { 0x9D, "\x99" },
    { 0x9E, "\x94" },
    { 0x9F, "\x95" },
    { 0xA0, "\xFB" },
    { 0xA2, "\xDA" },
    { 0xA3, "\xD9" },
    { 0xE0, "\x93" },
    { 0xE1, "\xA2" },
    { 0xE2, "\xA7" },
    { 0xE3, "\x98" },
    { 0xE4, "\x80" },
    { 0xE5, "\x87" },
    { 0xE6, "\xA5" },
    { 0xE7, "\xA4" },
    { 0xE8, "\xE2" },
    { 0xE9, "\xE4" },
    { 0xEB, "\xEA" },
    { 0xEC, "\xE0" },
    { 0xED, "\xE1" },
    { 0xEF, "\xEB" },
    { 0xF0, "\xEE" },
    { 0xF2, "\xE9" },
    { 0xF5, "\xE6" },
    { 0xF7, "\xE3" },
    { 0xF8, "\xE7" },
    { 0xFD, "\x9C" },
    { 0xFE, "\x9B" },
    { 0xFF, "\xF6" },
    { 0x00, NULL }
};

TRANSLATE ToWindows[] = 
{
    { 0x5C, "\xA5" },
    { 0x7F, "\xBF" },
    { 0x84, "\xB0" },
    { 0x85, "\x95" },
    { 0x86, "\xC4" },
    { 0x87, "\xC5" },
    { 0x88, "\xE4" },
    { 0x89, "\xE0" },
    { 0x8A, "\xE2" },
    { 0x8B, "\xE1" },
    { 0x8C, "\xE5" },
    { 0x8D, "\xAA" },
    { 0x8E, "\xCF" },
    { 0x8F, "\xEF" },
    { 0x90, "\xEC" },
    { 0x91, "\xEE" },
    { 0x92, "\xED" },
    { 0x93, "\xDC" },
    { 0x94, "\xFC" },
    { 0x95, "\xF9" },
    { 0x96, "\xFB" },
    { 0x97, "\xFA" },
    { 0x98, "\xC9" },
    { 0x99, "\xEB" },
    { 0x9A, "\xE8" },
    { 0x9B, "\xEA" },
    { 0x9C, "\xE9" },
    { 0x9D, "\xD6" },
    { 0x9E, "\xF6" },
    { 0x9F, "\xF2" },
    { 0xE0, "\xF4" },
    { 0xE1, "\xF3" },
    { 0xE2, "\xBA" },
    { 0xE3, "\xFF" },
    { 0xE4, "\xC7" },
    { 0xE5, "\xE7" },
    { 0xE6, "\xD1" },
    { 0xE7, "\xF1" },
    { 0xED, "\xDF" },
    { 0xF5, "\xB5" },
    { 0xFD, "\xA3" },
    { 0xFE, "\xA2" },
    { 0xFF, "\xF7" },
    { 0x00, NULL }
};

TRANSLATE ToUtf8[] = 
{
    { 0x5C, "\xC2\xA5" },
    { 0x7F, "\xC2\xBF" },
    { 0x80, "\xE2\x99\xA0" },
    { 0x81, "\xE2\x99\xA5" },
    { 0x82, "\xE2\x99\xA3" },
    { 0x83, "\xE2\x99\xA6" },
    { 0x84, "\xE2\x97\x8B" },
    { 0x85, "\xE2\x97\x8F" },
    { 0x86, "\xC3\x84" },
    { 0x87, "\xC3\x85" },
    { 0x88, "\xC3\xA4" },
    { 0x89, "\xC3\xA0" },
    { 0x8A, "\xC3\xA2" },
    { 0x8B, "\xC3\xA1" },
    { 0x8C, "\xC3\xA5" },
    { 0x8D, "\x61\xCC\xB1" },
    { 0x8E, "\xC3\x8F" },
    { 0x8F, "\xC3\xAF" },
    { 0x90, "\xC3\xAC" },
    { 0x91, "\xC3\xAE" },
    { 0x92, "\xC3\xAD" },
    { 0x93, "\xC3\x9C" },
    { 0x94, "\xC3\xBC" },
    { 0x95, "\xC3\xB9" },
    { 0x96, "\xC3\xBB" },
    { 0x97, "\xC3\xBA" },
    { 0x98, "\xC3\x89" },
    { 0x99, "\xC3\xAB" },
    { 0x9A, "\xC3\xA8" },
    { 0x9B, "\xC3\xAA" },
    { 0x9C, "\xC3\xA9" },
    { 0x9D, "\xC3\x96" },
    { 0x9E, "\xC3\xB6" },
    { 0x9F, "\xC3\xB2" },
    { 0xA0, "\xE2\x88\x9A" },
    { 0xA1, "\xE3\x83\xBB" },
    { 0xA2, "\xE3\x80\x8C" },
    { 0xA3, "\xE3\x80\x8D" },
    { 0xA4, "\xE3\x80\x81" },
    { 0xA5, "\xE3\x80\x82" },
    { 0xA6, "\xE3\x83\xB2" },
    { 0xA7, "\xE3\x82\xA1" },
    { 0xA8, "\xE3\x82\xA3" },
    { 0xA9, "\xE3\x82\xA5" },
    { 0xAA, "\xE3\x82\xA7" },
    { 0xAB, "\xE3\x82\xA9" },
    { 0xAC, "\xE3\x83\xA3" },
    { 0xAD, "\xE3\x83\xA5" },
    { 0xAE, "\xE3\x83\xA7" },
    { 0xAF, "\xE3\x83\x83" },
    { 0xB0, "\xE3\x83\xBC" },
    { 0xB1, "\xE3\x82\xA2" },
    { 0xB2, "\xE3\x82\xA4" },
    { 0xB3, "\xE3\x82\xA6" },
    { 0xB4, "\xE3\x82\xA8" },
    { 0xB5, "\xE3\x82\xAA" },
    { 0xB6, "\xE3\x82\xAB" },
    { 0xB7, "\xE3\x82\xAD" },
    { 0xB8, "\xE3\x82\xAF" },
    { 0xB9, "\xE3\x82\xB1" },
    { 0xBA, "\xE3\x82\xB3" },
    { 0xBB, "\xE3\x82\xB5" },
    { 0xBC, "\xE3\x82\xB7" },
    { 0xBD, "\xE3\x83\x8C" },
    { 0xBE, "\xE3\x83\xA3" },
    { 0xBF, "\xE3\x82\xBD" },
    { 0xC0, "\xE3\x82\xBF" },
    { 0xC1, "\xE3\x83\x81" },
    { 0xC2, "\xE3\x83\x84" },
    { 0xC3, "\xE3\x83\x86" },
    { 0xC4, "\xE3\x83\x88" },
    { 0xC5, "\xE3\x83\x8A" },
    { 0xC6, "\xE3\x83\x8B" },
    { 0xC7, "\xE3\x83\x8C" },
    { 0xC8, "\xE3\x83\x8D" },
    { 0xC9, "\xE3\x83\x8E" },
    { 0xCA, "\xE3\x83\x8F" },
    { 0xCB, "\xE3\x83\x92" },
    { 0xCC, "\xE3\x83\x95" },
    { 0xCD, "\xE3\x83\x98" },
    { 0xCE, "\xE3\x83\x9B" },
    { 0xCF, "\xE3\x83\x9E" },
    { 0xD0, "\xE3\x83\x9F" },
    { 0xD1, "\xE3\x83\xA0" },
    { 0xD2, "\xE3\x83\xA1" },
    { 0xD3, "\xE3\x83\xA2" },
    { 0xD4, "\xE3\x83\xA4" },
    { 0xD5, "\xE3\x83\xA6" },
    { 0xD6, "\xE3\x83\xA8" },
    { 0xD7, "\xE3\x83\xA9" },
    { 0xD8, "\xE3\x83\xAA" },
    { 0xD9, "\xE3\x83\xAB" },
    { 0xDA, "\xE3\x83\xAC" },
    { 0xDB, "\xE3\x83\xAD" },
    { 0xDC, "\xE3\x83\xAF" },
    { 0xDD, "\xE3\x83\xB3" },
    { 0xDE, "\xE3\x82\x9B" },
    { 0xDF, "\xE3\x82\x9C" },
    { 0xE0, "\xC3\xB4" },
    { 0xE1, "\xC3\xB3" },
    { 0xE2, "\x6F\xCC\xB1" },
    { 0xE3, "\xC3\xBF" },
    { 0xE4, "\xC3\x87" },
    { 0xE5, "\xC3\xA7" },
    { 0xE6, "\xC3\x91" },
    { 0xE7, "\xC3\xB1" },
    { 0xE8, "\xCE\x93" },
    { 0xE9, "\xCE\xA3" },
    { 0xEA, "\xCE\xA0" },
    { 0xEB, "\xCE\xA9" },
    { 0xEC, "\xCE\xB1" },
    { 0xED, "\xCE\xB2" },
    { 0xEE, "\xCE\xB3" },
    { 0xEF, "\xCE\xB4" },
    { 0xF0, "\xCE\xB5" },
    { 0xF1, "\xCE\xB6" },
    { 0xF2, "\xCE\xB8" },
    { 0xF3, "\xCE\xBA" },
    { 0xF4, "\xCE\xBB" },
    { 0xF5, "\xCE\xBC" },
    { 0xF6, "\xCF\x81" },
    { 0xF7, "\xCF\x80" },
    { 0xF8, "\xCF\x84" },
    { 0xF9, "\xCF\x95" },
    { 0xFA, "\xCF\x87" },
    { 0xFB, "\xCF\x89" },
    { 0xFC, "\xCE\xBD" },
    { 0xFD, "\xC2\xA3" },
    { 0xFE, "\xC2\xA2" },
    { 0xFF, "\xC3\xB7" },
    { 0x00, NULL }
};

/*
 *  Binary mode
 */
bool BinMode = TRUE;

/*
 *  Escape mode
 */
enum { ESC_NONE, ESC_MODE, ESC_HEX } 
    EscapeMode = ESC_NONE;

/*
 *  Translation mode
 */
enum { TRANS_NONE, TRANS_ASCII, TRANS_DOS, TRANS_UTF8, TRANS_WINDOWS } 
    TransMode = TRANS_NONE;

/*
 *  Header bytes
 */
struct _header {
    char leader[ 10 ];
    char file_name[ 6 ];
} Header;

#define HEADER_SIZE  16

void printHeader( void )
{
    printf( "Program: %.6s\n\n", Header.file_name );
}


/*
 *  Print a character, translate tokens
 */
void canonprint( int c )
{
    static bool insert_space = FALSE;
    static int last = 0;
    static bool quoted = FALSE;
    static bool quoted_until_eol = FALSE;
    static colon_pending = FALSE;
    static remark_pending = FALSE;
    unsigned char *p = NULL;
    char buffer[] = "\\XX\\XX";
    struct _escapes *ep;
    TRANSLATE *tp;

    if ( colon_pending ) {
        /*
         *  check for sequence :REM\FF or :ELSE
         */
        colon_pending = FALSE;
        if ( c == 0x8E ) {
            /*
             *  REM found
             */
            remark_pending = TRUE;
            return;
        }
        else if ( c == 0x90 ) {
            /*
             *  ELSE found, ignore the : and process the ELSE
             */
        }
        else {
            /*
             *  Output the pending : and process next char
             */
            putchar( ':' );
        }
    }
    else if ( remark_pending ) {
        /*
         *  :REM
         */
        remark_pending = FALSE;
        if ( c != 0xff ) {
            /*
             *  Not ' - output the pending :REM and process next char
             */
            printf( ":REM" );
            quoted = quoted_until_eol = TRUE;
        }  
    }
    if ( c == ':' && !quoted ) {
        /*
         *  Can't print : until next char is known
         */
        colon_pending = TRUE;
        return;
    }
    else if ( c == 0x00 ) {
        /*
         *  End of line
         */
        p = "\n";
    }
    else if ( !quoted && c >= 0x80 ) {
        /*
         *  replacable token
         */
        p = tokens[ c - 0x80 ];
        if ( c == 0x83 || c == 0x8E || c == 0xFF ) {
            /*
             *  DATA, REM or '
             */
            quoted = quoted_until_eol = TRUE;
        }
        if ( p == NULL ) {
            /*
             *  Not a valid token, escape it
             */
            sprintf( buffer, "\\%02.2X", (unsigned char) c );
            p = buffer;
        }
    }

    if ( c == ' ' ) {
        /*
         *  Literal space is always printed
         */
        putchar( c );
        insert_space = FALSE;
        last = ' ';
    }
    else {
        if ( p == NULL ) {
            
            if ( quoted && TransMode != TRANS_NONE ) {
                /*
                 *  Translate special characters
                 */
                tp = TransMode == TRANS_ASCII ? ToAscii 
                   : TransMode == TRANS_DOS   ? ToDos
                   : TransMode == TRANS_UTF8  ? ToUtf8
                   : ToWindows;

                for ( ; tp != NULL && tp->to != NULL; ++tp ) {
                    if ( c == tp->from ) {
                        p = tp->to;
                        break;
                    }
                }
                if ( p == NULL && !EscapeMode && c >= 0x7F ) 
                {
                    /*
                     *  Not found: use ASCII table as fallback
                     */
                    for ( tp = ToAscii; tp != NULL && tp->to != NULL; ++tp ) {
                        if ( c == tp->from ) {
                            p = tp->to;
                            break;
                        }
                    }
                    if ( p == NULL ) {
                        p = ".";
                    }
                }
            }
            if ( p == NULL && EscapeMode ) {
                /*
                 *  Create escape sequences like \pi
                 */
                ep = EscapeMode == ESC_MODE ? Escapes : NULL;

                for ( ; ep != NULL && ep->text != NULL; ++ep ) {
                    if ( c == ep->token ) {
                        sprintf( buffer, "\\%s", ep->text );
                        p = buffer;
                        break;
                    }
                }
                if ( p == NULL && ( c < ' ' || c >= 0x80 || c == '\\' ) ) {
                    /*
                     *  Hex escape
                     */
                    sprintf( buffer, "\\%02.2X", c );
                    p = buffer;
                }
            }
            if ( p == NULL ) {
                /*
                 *  Save character as string and point to it
                 */
                buffer[ 0 ] = (char) c;
                buffer[ 1 ] = '\0';
                p = buffer;
            }
        }

        if ( *p == ' ' && last == ' ' ) {
            /*
             *  Collapse spaces in tokens
             */
            ++p;
        }

        /*
         *  Print the Text
         */
        while ( ( c = *p++ ) != '\0' ) {
            /*
             *  Print character, insert or drop spaces as appropriate
             */
            switch ( c ) {
            
            case '"':
                quoted = !quoted || quoted_until_eol;
                insert_space = FALSE;
                break;

            case '\n':
                quoted = quoted_until_eol = FALSE;
                insert_space = FALSE;
                break;
            }

            if ( !quoted && c < ' ' && c != '\n' ) {
                /*
                 *  Unprintable char is escaped
                 */
                printf( "\\%02.2X", (unsigned char) c );
            }
            else if ( quoted ) {
                /*
                 *  Print char as is, no further translation
                 */
                putchar( c );
                insert_space = FALSE;
            }
            else {
                /*
                 *  Check for spaces and printable characters
                 */
                if ( c == ' ' && *p == '\0' ) {
                    /*
                     *  Token ends with a space
                     */
                    insert_space = TRUE;
                }
                else {
                    /*
                     *  Printable character
                     */
                    if ( insert_space && ( c >= '0' && c <= '9' || c >= 'A' ) ) { 
                        putchar( ' ' );
                    }
                    putchar( c );
                    insert_space = FALSE;
                }
            }
            last = c;
        }
    }
}


/*
 *  Handle next character from file
 *
 *  Returns 0 when OK or the error code when invalid data encountered
 */
int list( int c )
{
    static int skipped = 5;
    static int segment_id = 0;
    static int idle_counter = 0;
    static int data_counter = 0;
    static int line_counter = 0;
    static int pointer_counter = 0;
    static int line_number = 0;
    static int last_pointer = 0;
    static int line_pointer = 0x553; /* BASIC area start */
    static int line_length = 0;

    static bool leader_expected = TRUE;
    static bool dataseg_expected = FALSE;

    if ( !BinMode ) {
        /*
         *  Skip first 5 bytes before the leader
         */
        if ( skipped > 0 ) {
            skipped--;
            return 0;
        }

        /*
         *  Handle the leader
         */
        if ( c == KCS_LEAD_IN ) {
            /*
             *  Idle string (leader)
             */
            if ( idle_counter == 0 && !leader_expected ) {
                /*
                 *  "Premature end of header segment" error
                 */
                return 2;
            }
            if ( idle_counter++ >= 12 ) {
                /*
                 *  Leader contains at least 12 idle strings
                 */
                leader_expected = FALSE;
            }
            data_counter = 0;
            return 0;
        }
        else {
            /*
             *  Data byte received
             */
            idle_counter = 0;
            if ( leader_expected ) {
                /*
                 *  "Leader expected" error
                 */
                return 1;
            }
            /*
             *  Strip framing information
             */
            c &= 0xff;
        }
#if DEBUG
        printf( "<%02.2X>", c );
#endif
    }
    else {
        /*
         *  Binary mode
         */
        if ( leader_expected ) {
            /*
             *  New block
             */
            if ( dataseg_expected ) {
                /*
                 *  We cannot identify the data block by contents
                 *  Just assume it has started
                 */
                data_counter = 0;
                leader_expected = FALSE;
#if DEBUG
                printf( "\nData found\n" );
#endif                    
            }
            else {    
                /*
                 *  Check for D3 identifier
                 */
                if ( c == 0xD3 ) {
                    /*
                     *  Valid header byte encountered
                     */
#if DEBUG
                    printf( "\nHeader found\n" );
#endif                    
                    data_counter = 0;
                    leader_expected = FALSE;
                }
                else {
#if DEBUG
                    printf( " skipped\n" );
#endif                    
                    return 0;
                }
            }
        }
    }

    if ( ++data_counter == 1 ) {
        /*
         *  Handle the first segment byte
         */
        segment_id = dataseg_expected ? 'D' : 'H';
        pointer_counter = 2;
#if DEBUG
        printf( "Segment id = '%c'\n", segment_id );
#endif                    
    }

    if ( segment_id == 'D' ) {
        /*
         *  Handle the data segment
         */
        if ( pointer_counter > 0 ) {
            /*
             *  Pointer chain to next line
             */
            if ( --pointer_counter == 1 ) {
                /*
                 *  LSB
                 */
                last_pointer = line_pointer;
                line_pointer = c;
            }
            else {
                /*
                 *  MSB
                 */
                line_pointer += c * 256;
                if ( line_pointer == 0 ) {
                    /*
                     *  End of file
                     */
                    line_length = 0;
                }
                else {
                    line_length = line_pointer - last_pointer - 4;
#if DEBUG
                    printf( "Pointers: %04.4X, %04.4X, %d\n",
                            last_pointer, line_pointer, line_length );
#endif
                    if ( line_length <= 1 ) {
                        /*
                         *  pointers inconsistent
                         */
                        return 5;
                    }
                    line_counter = 2;
                }
            }
            return 0;
        }

        if ( line_counter > 0 ) {
            /*
             *  Line number
             */
            if ( --line_counter == 1 ) {
                /*
                 *  LSB
                 */
                line_number = c;
            }
            else {
                /*
                 *  MSB
                 */
                line_number += c * 256;
                printf( "%d ", line_number );
            }
            return 0;
        }

        if ( line_length > 0 ) {
            /*
             *  Print remaining bytes of the line
             */
            if ( c == 0x00 ) {
                /*
                 *  End of line: prepare for new pointer & line_number
                 */
                if ( line_length != 1 ) {
                    /*
                     *  pointers inconsistent
                     */
                    return 5;
                }
                pointer_counter = 2;
            }
            /*
             *  Output the next char or token
             */
            canonprint(c);
            if ( --line_length < 0 ) {
                /*
                 *  pointers inconsistent
                 */
                return 5;
            }
        }
        return 0;
    }

    /*
     *  Handle the header segment
     */
    if ( c != 0xD3 && data_counter <= 10 ) {
        /*
         *  Header too short, ignore the problem and skip to file_name
         */
        data_counter = 11;
    }

    ((char *) &Header)[ data_counter - 1 ] = c;

    if ( data_counter == HEADER_SIZE ) {
        /*
         *  End of header
         */
        printHeader();
        dataseg_expected = TRUE;
        leader_expected = TRUE;
    }
    return 0;
}


/*
 *  Main program
 */
int main( int argc, char *argv[] )
{
    int c1;
    long position = 0;    /* file position */
    int err_code;
    FILE *infp = NULL;
    KCS_FILE *wfp = NULL;
    int skip = 0;
    bool wavemode = FALSE;
    bool ignore = FALSE;
    int baud = 1200;

    ++argv;
    --argc;

    while ( argc > 1 && **argv == '-' ) {

        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            BinMode = TRUE;
            skip = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-w", 2 ) == 0 ) {
            BinMode = FALSE;
            wavemode = TRUE;
            if ( (*argv)[ 2 ] == '-' ) {
                baud = -baud;
            }
        }
        if ( strncmp( argv[ 1 ], "-i", 2 ) == 0 ) {
            ignore = TRUE;
        }
        if ( strncmp( *argv, "-e", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'C':
                EscapeMode = ESC_MODE; break;
            case 'X':
            case 'H':
            case '\0':
                EscapeMode = ESC_HEX; break;
            }
        }
        if ( strncmp( *argv, "-c", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'A':
                TransMode = TRANS_ASCII; break;
            case 'D':
                TransMode = TRANS_DOS; break;
            case 'U':
                TransMode = TRANS_UTF8; break;
            case 'W':
            case '\0':
                TransMode = TRANS_WINDOWS; break;
            }
        }
        ++argv;
        --argc;
    }
#if DEBUG
    printf( "binmode = %d, wavemode = %d\n", BinMode, wavemode );
#endif

    if ( argc < 1 ) {
        printf( "usage: listX07 <options> infile > outfile\n"
                "         -w reads a WAV file directly\n"
                "         -w- inverts the phase in case of difficulty\n"
                "         -b<skip> reads a binary file, "
                          "<skip> is an optional offset\n" 
                "         -i ignores any errors on the input stream\n"
                "         -e[C|X] use backslash escapes in strings\n"
                "           C: character escapes like '\\pi'\n"
                "           X: generic hexadecimal escapes (default)\n"
                "         -c[A|D|W] use character set translation in strings\n"
                "           A: ASCII codes only (very simple)\n"
                "           D: DOS code page 437\n"
                "           U: Unicode (UTF-8)\n"
                "           W: Windows code page 1252 (default)\n"
                "         -c takes precedence over -e. Both can be mixed.\n"
              );
        return 2;
    }

    if ( wavemode ) {
        /*
         *  Open WAV file
         */
        if ( ( wfp = kcsOpen( *argv, "rb", 1200, 8, 'N', 2 ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    else {
        /*
         *  Open binary file
         */
        if ( ( infp = fopen( *argv, "rb" ) ) == NULL ) {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }

    if ( wavemode ) {
        /*
         *  Process WAV file
         */
        while ( ( c1 = kcsReadByte( wfp ) ) >= 0 ) {
#if DEBUG
            printf( "[%02.2X]", c1 );
#endif
            if ( ignore 
                 && c1 != KCS_LEAD_IN && c1 & ( KCS_FRAMING | KCS_PARITY ) ) 
            {
                continue;
            }
            if ( ( err_code = list( c1 ) ) != 0 ) {
                printf( "\nInvalid data encountered \\%02.2X - %s.\n",
                        c1, err_msg[ err_code ] );
                break;
            }
        }
        if ( c1 < 0 && c1 != KCS_EOF ) {
            fprintf( stderr, "Result: %d\n", c1 );
            if ( errno != 0 ) {
                perror( "read" );
            }
            return 2;
        }
        kcsClose( wfp );
    }
    else {
        /*
         *  Process binary file
         */
        while ( ( c1 = fgetc( infp ) ) != EOF ) {
#if DEBUG
            printf( "[%02.2X]", c1 );
#endif
            if ( skip > 0 ) {
                /*
                 *  Skip garbage
                 */
                --skip;
                ++position;
                continue;
            }
            if ( ( err_code = list( c1 ) ) != 0 ) {
                /*
                 *  Error
                 */
                printf( "\nInvalid data @%04.4lX \\%02.2X - %s.\n",
                         position, c1, err_msg[ err_code ] );
                break;
            }
            ++position;
        }
        fclose( infp );
    }

    return 0;
}
