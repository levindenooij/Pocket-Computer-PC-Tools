/*
 *  basX07.c - Create Canon X-07 tape images from BASIC source
 *
 *  This program creates a binary tape image file for the Canon X-07 computer.
 *
 *  Options:
 *    -b create BIN file to be converted by waveX07 later (Default)
 *    -w create a 1200 baud WAV file
 *    -l allow lowercase 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"
#ifdef __unix__
#include <unistd.h>
#else
#ifdef _WIN32_WCE
#define sleep(s) Sleep((s)*1000)
#else
#include <dos.h>
#endif
#endif

#define LEAD_IN_TIME 6        /* tens of a second */
#define LEAD_IN_TIME_FIRST 20 /* tens of a second */

/*
 *  Error messages
 */
const char *err_msg[] = {
    "",                                       /* OK */
    "No line numbers found",                  /* message #1 */
    "Write error",                            /* message #2 */
    "Line too long",                          /* message #3 */
    "Line number too large",                  /* message #4 */
    "Not implemented"                         /* message #5 */
};

typedef struct _token {
    char *text;
    unsigned char token;
    unsigned char flag;
} TOKEN; 

/*
 *  Character set translation tables
 */
typedef struct _translate {
    char from, to;
} TRANSLATE;

TRANSLATE FromDos[] = 
{
    { 0x9D, 0x5C },
    { 0xA8, 0x7F },
    { 0x06, 0x80 },
    { 0x06, 0x81 },
    { 0x05, 0x82 },
    { 0x04, 0x83 },
    { 0xF8, 0x84 },
    { 0xF9, 0x85 },
    { 0x8E, 0x86 },
    { 0x8F, 0x87 },
    { 0x84, 0x88 },
    { 0x85, 0x89 },
    { 0x83, 0x8A },
    { 0xA0, 0x8B },
    { 0x86, 0x8C },
    { 0xA6, 0x8D },
    { 0x8B, 0x8F },
    { 0x8D, 0x90 },
    { 0x8C, 0x91 },
    { 0xA1, 0x92 },
    { 0x9A, 0x93 },
    { 0x81, 0x94 },
    { 0x97, 0x95 },
    { 0x96, 0x96 },
    { 0xA3, 0x97 },
    { 0x90, 0x98 },
    { 0x89, 0x99 },
    { 0x8A, 0x9A },
    { 0x88, 0x9B },
    { 0x82, 0x9C },
    { 0x99, 0x9D },
    { 0x94, 0x9E },
    { 0x95, 0x9F },
    { 0xFB, 0xA0 },
    { 0xDA, 0xA2 },
    { 0xD9, 0xA3 },
    { 0x93, 0xE0 },
    { 0xA2, 0xE1 },
    { 0xA7, 0xE2 },
    { 0x98, 0xE3 },
    { 0x80, 0xE4 },
    { 0x87, 0xE5 },
    { 0xA5, 0xE6 },
    { 0xA4, 0xE7 },
    { 0xE2, 0xE8 },
    { 0xE4, 0xE9 },
    { 0xEA, 0xEB },
    { 0xE0, 0xEC },
    { 0xE1, 0xED },
    { 0xEB, 0xEF },
    { 0xEE, 0xF0 },
    { 0xE9, 0xF2 },
    { 0xE6, 0xF5 },
    { 0xE3, 0xF7 },
    { 0xE7, 0xF8 },
    { 0x9C, 0xFD },
    { 0x9B, 0xFE },
    { 0xF6, 0xFF },
    { 0x00, 0x00 }
};


TRANSLATE FromWindows[] = 
{
    { 0xA5, 0x5C },
    { 0xBF, 0x7F },
    { 0xB0, 0x84 },
    { 0x95, 0x85 },
    { 0xC4, 0x86 },
    { 0xC5, 0x87 },
    { 0xE4, 0x88 },
    { 0xE0, 0x89 },
    { 0xE2, 0x8A },
    { 0xE1, 0x8B },
    { 0xE5, 0x8C },
    { 0xAA, 0x8D },
    { 0xCF, 0x8E },
    { 0xEF, 0x8F },
    { 0xEC, 0x90 },
    { 0xEE, 0x91 },
    { 0xED, 0x92 },
    { 0xDC, 0x93 },
    { 0xFC, 0x94 },
    { 0xF9, 0x95 },
    { 0xFB, 0x96 },
    { 0xFA, 0x97 },
    { 0xC9, 0x98 },
    { 0xEB, 0x99 },
    { 0xE8, 0x9A },
    { 0xEA, 0x9B },
    { 0xE9, 0x9C },
    { 0xD6, 0x9D },
    { 0xF6, 0x9E },
    { 0xF2, 0x9F },
    { 0xF4, 0xE0 },
    { 0xF3, 0xE1 },
    { 0xFF, 0xE3 },
    { 0xC7, 0xE4 },
    { 0xE7, 0xE5 },
    { 0xD1, 0xE6 },
    { 0xF1, 0xE7 },
    { 0xDF, 0xED },
    { 0xB5, 0xF5 },
    { 0xA3, 0xFD },
    { 0xA2, 0xFE },
    { 0xF7, 0xFF },
    { 0x00, 0x00 }
};


/*
 * UTF-8 encoding
 */
TOKEN FromUtf8[] = 
{
    { "\xC2\xA5"    , 0x5C },
    { "\xC2\xBF"    , 0x7F },
    { "\xE2\x99\xA0", 0x80 },
    { "\xE2\x99\xA5", 0x81 },
    { "\xE2\x99\xA3", 0x82 },
    { "\xE2\x99\xA6", 0x83 },
    { "\xE2\x97\x8B", 0x84 },
    { "\xE2\x97\x8F", 0x85 },
    { "\xC3\x84"    , 0x86 },
    { "\xC3\x85"    , 0x87 },
    { "\xC3\xA4"    , 0x88 },
    { "\xC3\xA0"    , 0x89 },
    { "\xC3\xA2"    , 0x8A },
    { "\xC3\xA1"    , 0x8B },
    { "\xC3\xA5"    , 0x8C },
    { "\x61\xCC\xB1", 0x8D },
    { "\xC3\x8F"    , 0x8E },
    { "\xC3\xAF"    , 0x8F },
    { "\xC3\xAC"    , 0x90 },
    { "\xC3\xAE"    , 0x91 },
    { "\xC3\xAD"    , 0x92 },
    { "\xC3\x9C"    , 0x93 },
    { "\xC3\xBC"    , 0x94 },
    { "\xC3\xB9"    , 0x95 },
    { "\xC3\xBB"    , 0x96 },
    { "\xC3\xBA"    , 0x97 },
    { "\xC3\x89"    , 0x98 },
    { "\xC3\xAB"    , 0x99 },
    { "\xC3\xA8"    , 0x9A },
    { "\xC3\xAA"    , 0x9B },
    { "\xC3\xA9"    , 0x9C },
    { "\xC3\x96"    , 0x9D },
    { "\xC3\xB6"    , 0x9E },
    { "\xC3\xB2"    , 0x9F },
    { "\xE2\x88\x9A", 0xA0 },
    { "\xE3\x83\xBB", 0xA1 },
    { "\xE3\x80\x8C", 0xA2 },
    { "\xE3\x80\x8D", 0xA3 },
    { "\xE3\x80\x81", 0xA4 },
    { "\xE3\x80\x82", 0xA5 },
    { "\xE3\x83\xB2", 0xA6 },
    { "\xE3\x82\xA1", 0xA7 },
    { "\xE3\x82\xA3", 0xA8 },
    { "\xE3\x82\xA5", 0xA9 },
    { "\xE3\x82\xA7", 0xAA },
    { "\xE3\x82\xA9", 0xAB },
    { "\xE3\x83\xA3", 0xAC },
    { "\xE3\x83\xA5", 0xAD },
    { "\xE3\x83\xA7", 0xAE },
    { "\xE3\x83\x83", 0xAF },
    { "\xE3\x83\xBC", 0xB0 },
    { "\xE3\x82\xA2", 0xB1 },
    { "\xE3\x82\xA4", 0xB2 },
    { "\xE3\x82\xA6", 0xB3 },
    { "\xE3\x82\xA8", 0xB4 },
    { "\xE3\x82\xAA", 0xB5 },
    { "\xE3\x82\xAB", 0xB6 },
    { "\xE3\x82\xAD", 0xB7 },
    { "\xE3\x82\xAF", 0xB8 },
    { "\xE3\x82\xB1", 0xB9 },
    { "\xE3\x82\xB3", 0xBA },
    { "\xE3\x82\xB5", 0xBB },
    { "\xE3\x82\xB7", 0xBC },
    { "\xE3\x83\x8C", 0xBD },
    { "\xE3\x83\xA3", 0xBE },
    { "\xE3\x82\xBD", 0xBF },
    { "\xE3\x82\xBF", 0xC0 },
    { "\xE3\x83\x81", 0xC1 },
    { "\xE3\x83\x84", 0xC2 },
    { "\xE3\x83\x86", 0xC3 },
    { "\xE3\x83\x88", 0xC4 },
    { "\xE3\x83\x8A", 0xC5 },
    { "\xE3\x83\x8B", 0xC6 },
    { "\xE3\x83\x8C", 0xC7 },
    { "\xE3\x83\x8D", 0xC8 },
    { "\xE3\x83\x8E", 0xC9 },
    { "\xE3\x83\x8F", 0xCA },
    { "\xE3\x83\x92", 0xCB },
    { "\xE3\x83\x95", 0xCC },
    { "\xE3\x83\x98", 0xCD },
    { "\xE3\x83\x9B", 0xCE },
    { "\xE3\x83\x9E", 0xCF },
    { "\xE3\x83\x9F", 0xD0 },
    { "\xE3\x83\xA0", 0xD1 },
    { "\xE3\x83\xA1", 0xD2 },
    { "\xE3\x83\xA2", 0xD3 },
    { "\xE3\x83\xA4", 0xD4 },
    { "\xE3\x83\xA6", 0xD5 },
    { "\xE3\x83\xA8", 0xD6 },
    { "\xE3\x83\xA9", 0xD7 },
    { "\xE3\x83\xAA", 0xD8 },
    { "\xE3\x83\xAB", 0xD9 },
    { "\xE3\x83\xAC", 0xDA },
    { "\xE3\x83\xAD", 0xDB },
    { "\xE3\x83\xAF", 0xDC },
    { "\xE3\x83\xB3", 0xDD },
    { "\xE3\x82\x9B", 0xDE },
    { "\xE3\x82\x9C", 0xDF },
    { "\xC3\xB4"    , 0xE0 },
    { "\xC3\xB3"    , 0xE1 },
    { "\x6F\xCC\xB1", 0xE2 },
    { "\xC3\xBF"    , 0xE3 },
    { "\xC3\x87"    , 0xE4 },
    { "\xC3\xA7"    , 0xE5 },
    { "\xC3\x91"    , 0xE6 },
    { "\xC3\xB1"    , 0xE7 },
    { "\xCE\x93"    , 0xE8 },
    { "\xCE\xA3"    , 0xE9 },
    { "\xCE\xA0"    , 0xEA },
    { "\xCE\xA9"    , 0xEB },
    { "\xCE\xB1"    , 0xEC },
    { "\xCE\xB2"    , 0xED },
    { "\xCE\xB3"    , 0xEE },
    { "\xCE\xB4"    , 0xEF },
    { "\xCE\xB5"    , 0xF0 },
    { "\xCE\xB6"    , 0xF1 },
    { "\xCE\xB8"    , 0xF2 },
    { "\xCE\xBA"    , 0xF3 },
    { "\xCE\xBB"    , 0xF4 },
    { "\xCE\xBC"    , 0xF5 },
    { "\xCF\x81"    , 0xF6 },
    { "\xCF\x80"    , 0xF7 },
    { "\xCF\x84"    , 0xF8 },
    { "\xCF\x95"    , 0xF9 },
    { "\xCF\x87"    , 0xFA },
    { "\xCF\x89"    , 0xFB },
    { "\xCE\xBD"    , 0xFC },
    { "\xC2\xA3"    , 0xFD },
    { "\xC2\xA2"    , 0xFE },
    { "\xC3\xB7"    , 0xFF },
    { NULL, 0 }
};

/*
 *  Escape codes with \, work everywhere
 */
TOKEN escapes[] = 
{
    { "\\", 0x5C },  /* backslash */
    { "YN", 0x5C },  /* Yen */
    { "?",  0x7F },  /* reverse ? */
    { "SP", 0x80 },  /* spade */
    { "HT", 0x81 },  /* heart */
    { "DI", 0x82 },  /* diamond */
    { "CL", 0x83 },  /* club */
    { "@",  0x84 },  /* circle */
    { "LD", 0x85 },  /* large dot */
    { ":A", 0x86 },  /* A umlaut */
    { ".A", 0x87 },  /* A circle */
    { "AN", 0x87 },  /* Angstroem */
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
    { "SM", 0xE9 },  /* Sum */
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
 *  Keywords
 *  Single byte tokens from 0x80 to 0xFF
 *  Flag 0x01 marks commands that start transparent mode
 *  Flag 0x02 marks ' and ELSE which are to be preceded by a colon
 *  Flag 0x04 marks ' which has an additional REM token 
 */
TOKEN tokens[] =
{
    { "^",       0xD5 },
    { "\\",      0xDB },
    { "XOR",     0xD8 },
    { "VARPTR",  0xC4 },
    { "VAL",     0xF4 },
    { "USR",     0xC5 },
    { "USING",   0xBD },
    { "TR",      0x91 },
    { "TO",      0xBB },
    { "TKEY",    0xF7 },
    { "TIME$",   0xC9 },
    { "THEN",    0xCE },
    { "TAN",     0xEA },
    { "TAB(",    0xBA },
    { "STRING$", 0xC0 },
    { "STRIG",   0xFD },
    { "STR$",    0xF3 },
    { "STOP",    0x8F },
    { "STICK",   0xFC },
    { "STEP",    0xD0 },
    { "START$",  0xCA },
    { "SQR",     0xE4 },
    { "SNS",     0xC6 },
    { "SLEEP",   0xAD },
    { "SIN",     0xE9 },
    { "SGN",     0xDF },
    { "SCREEN",  0xCD },
    { "SAVE",    0xB3 },
    { "RUN",     0x89 },
    { "RND",     0xE5 },
    { "RIGHT$",  0xF9 },
    { "RETURN",  0x8D },
    { "RESUME",  0x99 },
    { "RESTORE", 0x8B },
    { "REM",     0x8E, 0x01 },
    { "READ",    0x86 },
    { "PSET",    0xAA },
    { "PRINT",   0x9F },
    { "PRESET",  0xAB },
    { "POS",     0xE3 },
    { "POKE",    0x9E },
    { "POINT",   0xFE },
    { "PEEK",    0xEC },
    { "PAINT",   0xB1 },
    { "OUT",     0x9A },
    { "OR",      0xD7 },
    { "ON",      0x9B },
    { "OFF",     0xAC },
    { "NOT",     0xCF },
    { "NEXT",    0x82 },
    { "NEW",     0xB9 },
    { "MOTOR",   0x92 },
    { "MOD",     0xDA },
    { "MID$",    0xFA },
    { "LPRINT",  0x9C },
    { "LOG",     0xE6 },
    { "LOCATE",  0xA9 },
    { "LOAD",    0xB2 },
    { "LLIST",   0xA2 },
    { "LIST",    0xA1 },
    { "LINE",    0x97 },
    { "LET",     0x87 },
    { "LEN",     0xF1 },
    { "LEFT$",   0xF8 },
    { "KEY$",    0xCC },
    { "INT",     0xE0 },
    { "INSTR",   0xC1 },
    { "INPUT",   0x84 },
    { "INP",     0xC3 },
    { "INKEY$",  0xC2 },
    { "INIT",    0xB4 },
    { "IF",      0x8A },
    { "HEX$",    0xF2 },
    { "GOTO",    0x88 },
    { "GOSUB",   0x8C },
    { "FSET",    0xB0 },
    { "FRE",     0xE2 },
    { "FOR",     0x81 },
    { "FONT$",   0xCB },
    { "FN",      0xBC },
    { "FIX",     0xF0 },
    { "EXP",     0xE7 },
    { "EXEC",    0xA8 },
    { "ERROR",   0xBF },
    { "ERROR",   0x98 },
    { "ERL",     0xBE },
    { "ERASE",   0xB5 },
    { "EQU",     0xD9 },
    { "END",     0x80 },
    { "ELSE",    0x90, 0x02 },
    { "DIR",     0xAE },
    { "DIM",     0x85 },
    { "DELETE",  0xAF },
    { "DEFSTR",  0x93 },
    { "DEFSNG",  0x95 },
    { "DEFINT",  0x94 },
    { "DEFFN",   0x9D },
    { "DEFDBL",  0x96 },
    { "DATE$",   0xC8 },
    { "DATA",    0x83, 0x01 },
    { "CSRLIN",  0xFB },
    { "CSNG",    0xEE },
    { "CSAVE",   0xB8 },
    { "COS",     0xE8 },
    { "CONT",    0xA0 },
    { "CONSOLE", 0xA5 },
    { "COLOR",   0xA7 },
    { "CLS",     0xA6 },
    { "CLOAD",   0xB7 },
    { "CLEAR",   0xA3 },
    { "CIRCLE",  0xA4 },
    { "CINT",    0xED },
    { "CHR$",    0xF6 },
    { "CDBL",    0xEF },
    { "BEEP",    0xB6 },
    { "ATN",     0xEB },
    { "ASC",     0xF5 },
    { "AND",     0xD6 },
    { "ALM$",    0xC7 },
    { "ABS",     0xE1 },
    { ">",       0xDC },
    { "=",       0xDD },
    { "<",       0xDE },
    { "/",       0xD4 },
    { "-",       0xD2 },
    { "+",       0xD1 },
    { "*",       0xD3 },
    { "'",       0xFF, 0x01 + 0x02 + 0x04 },
    { NULL,      0x00 }
}; 

/*
 *  File handling
 */
KCS_FILE *WaveOut = NULL;
FILE *FileOut = NULL;
FILE *FileIn = NULL;

/*
 *  Output mode, translation mode and some flags
 */
enum { MODE_BIN, MODE_WAVE, MODE_TEXT } 
    OutputMode = MODE_BIN;

enum { TRANS_NONE, TRANS_DOS, TRANS_WINDOWS, TRANS_UTF8 } 
    TransMode = TRANS_NONE;

bool CompressSpaces = FALSE;

/*
 *  Header bytes
 */
struct _header {
    char leader[ 10 ];
    char file_name[ 6 ];
} Header;

#define HEADER_SIZE  16

/*
 *  Buffer for encoded lines
 */
#define BUFFER_SIZE (256 + 5)
unsigned char Buffer[ BUFFER_SIZE ];

/*
 *  Buffer for text lines
 */
#define LINE_SIZE 256
unsigned char Line[ LINE_SIZE ];

/*
 *  Line Pointer to next line
 */
#define BASIC_START 0x553  /* BASIC area start */
unsigned short LinePointer = BASIC_START;

/*
 *  Case insensitive string compare
 */
int compare( char *p1, char *p2, int l ) 
{
    int c;
    while ( l && *p1 != '\0' && *p2 != '\0' ) {
        c = toupper( *p1 ) - toupper( *p2 );
        if ( c != 0 ) {
            return c;
        }
        ++p1;
        ++p2;
        --l;
    }
    c = l == 0 ? 0 : p1[ 1 ] == '\0' ? -1 : 1;
    return c;
}


/*
 *  Case sensitive string compare
 */
int compare_case( char *p1, char *p2, int l ) 
{
    int c;
    while ( l && *p1 != '\0' && *p2 != '\0' ) {
        c = *p1 - *p2;
        if ( c != 0 ) {
            return c;
        }
        ++p1;
        ++p2;
        --l;
    }
    c = l == 0 ? 0 : p1[ 1 ] == '\0' ? -1 : 1;
    return c;
}


/*
 *  Output a byte to the BIN or WAV file
 *  Returns 0 when OK or the error code when invalid data encountered
 */
int output( int c )
{
    static int lead_in_time = LEAD_IN_TIME_FIRST;

    errno = 0;

    switch ( OutputMode ) {
     
    case MODE_BIN:
        /*
         *  Binary mode
         */
        if ( c != KCS_LEAD_IN && c != EOF ) {
            /*
             *  Only ordinary characters are written
             */
            if ( c != fputc( c, FileOut ) ) {
                /*
                 *  I/O error
                 */
                return 2;
            }
        }
        break;

    case MODE_WAVE:
        /*
         *  Wave mode
         */
        switch ( c ) {

        case KCS_LEAD_IN:
            /*
             *  Write a sequence of '1' bits
             */
            if ( 0 != kcsLeadIn( WaveOut, lead_in_time ) ) {
                /*
                 *  I/O error
                 */
                return 2;
            }
            lead_in_time = LEAD_IN_TIME;
            break;

        case EOF:
            /*
             *  Reset lead in time to start value
             */
            lead_in_time = LEAD_IN_TIME_FIRST;
            break;

        default:
            /*
             *  Write byte to file
             */
            if ( 0 != kcsWriteByte( WaveOut, c ) ) {
                /*
                 *  I/O error
                 */
                return 2;
            }
        }    
        break;

    case MODE_TEXT:
        /*
         *  Plain text mode
         */
        if ( c != KCS_LEAD_IN && c != EOF ) {
            /*
             *  Only ordinary characters and EOF are written
             */
            if ( c != fputc( c, FileOut ) ) {
                /*
                 *  I/O error
                 */
                return 2;
            }
        }
        break;
    }
    return 0;
}


/*
 *  Send a segment
 */
int output_segment( void *seg, int l )
{
    unsigned char *p = (unsigned char *) seg;
    int err;

    err = output( KCS_LEAD_IN );

    while ( err == 0 && l-- ) {
        /*
         *  Write each segment byte
         */
        err = output( *p++ );
    }
    return err;
}


/*
 *  Read a line from input file and do some preliminary syntax checks
 *  Returns the line number in binary or 0 (ignore) or EOF (error)
 */
long read_line( char *buffer, int *err )
{ 
    char temp_buffer[ LINE_SIZE ];
    char *temp = temp_buffer;
    char *p, *q, *q2, *q3;
    int line_number, l;
    bool in_string, transparent;
    TOKEN *ep;
    TRANSLATE *tp;
    
    errno = 0;
    p = fgets( temp, LINE_SIZE, FileIn );
    if ( p == NULL ) {
        /*
         *  EOF or error
         */
        if ( errno != 0 ) {
            *err = 2;
        }
        return EOF;
    }
    else {
        p = strchr( temp, '\n' );
        if ( p == NULL ) {
            /*
             *  line is too long
             */
            *err = 3;
            return EOF;
        }
        *p = '\0';
        p = strchr( temp, '\r' );
        if ( p != NULL ) {
            /*
             *  Remove DOS style CR delimiter
             */
            *p = '\0';
        }
    }

    /*
     *  Compute and copy line number
     */
    line_number = 0;
    while ( *temp == ' ' ) {
        ++temp;
    }
    while ( isdigit( *temp ) ) {
        line_number *= 10;
        line_number += ( *temp & 0xf );
        *buffer++ = *temp++;
    }
    if ( line_number == 0 ) {
        /*
         *  Ignore ths line
         */
        return 0;
    }
    if ( *temp == ':' ) {
        /*
         *  Line starts with colon (Sharp style listing)
         */
        ++temp;
    }
    while ( *temp == ' ' ) {
        ++temp;
    }

    /*
     *  Uppercase everything outside strings, REMarks or DATA
     *  Remove spaces
     */
    transparent = FALSE;
    in_string = FALSE;

    q = buffer;

    for ( p = temp; *p != '\0'; *q++ = *p++ ) {

        if ( !in_string && !transparent && 
             ( q[ -1 ] == '\'' 
               || compare( q - 4, "DATA", 4 ) == 0
               || compare( q - 3, "REM" , 3 ) == 0 ) )
        {
            transparent = TRUE;
            continue;
        }
        /*
         *  Remove spaces
         */
        while ( CompressSpaces && !in_string && !transparent 
                && isspace( *p ) ) 
        {
            ++p;
        }

        if ( *p == '"' ) {
            in_string = !in_string;
        }
        if ( TransMode != TRANS_NONE && TransMode != TRANS_UTF8 &&
             ( in_string || transparent ) ) 
        {
            /*
             *  Character set translation
             */
            tp = TransMode == TRANS_DOS ? FromDos : FromWindows;
            for ( ; tp->from != 0x00; ++tp ) {
                if ( *p == tp->from ) {
                    *p = tp->to;
                    break;
                }
            }
        }

        if ( in_string || transparent ) {
            continue;
        }

        /*
         *  Make uppercase
         */
        *p = toupper( *p );
    }
    *q = '\0';

    printf( "%5d %s\n", line_number, buffer );

    /*
     *  Handle escapes and Unicode
     */
    for ( q = p = buffer; *p != '\0'; ++q, ++p ) {
        /*
         *  Copy char, may be overwritten later
         */
        *q = *p;

        if ( TransMode == TRANS_UTF8 ) {
            /*
             *  Look for UTF-8 sequence
             */
            for ( ep = FromUtf8; ep->text != NULL; ++ep ) {
                l = strlen( ep->text );
                if ( 0 == compare_case( p, ep->text, l ) ) {
                    break;
                }
            }
            if ( ep->text != NULL ) {
                /*
                 *  Replace string by internal code
                 */
                p += l - 1;
                *q = ep->token;
            }
        }

        if ( *p == '\\' ) {
            /*
             *  look for abreviation
             */
            ++p;
            
            /*
             *  Case sensitive search
             */
            for ( ep = escapes; ep->text != NULL; ++ep ) {
                l = strlen( ep->text );
                if ( 0 == compare_case( p, ep->text, l ) ) {
                    break;
                }
            }
            if ( ep->text == NULL ) {
                /*
                 *  Not found - case insensitive search
                 */
                for ( ep = escapes; ep->text != NULL; ++ep ) {
                    l = strlen( ep->text );
                    if ( 0 == compare( p, ep->text, l ) ) {
                        break;
                    }
                }
            }
            if ( ep->text != NULL ) {
                /*
                 *  Replace string by token value
                 */
                p += l - 1;
                *q = ep->token;
            }
            else {
                /*
                 *  hex value
                 */
                if ( isxdigit( *p ) ) {
                    sscanf( p, "%2x", &l );
                }
                else {
                    l = 0;
                }
                if ( l != 0 ) {
                    ++p;
                    *q = l;
                }
                else {
                    /*
                     *  unknown code, copy string after backslash
                     */
                    *q = *p;
                }
            }
            continue;
        }
    }
    *q = '\0';

    return line_number;
}


/*
 *  Encode (tokenize) a program line
 */
int encode( long line_number, char *line, char *buffer )
{
    unsigned int i, l, tl;
    char *lp = buffer;
    TOKEN *tp;
    bool transparent, in_string;

    /*
     *  Skip line pointer
     */
    buffer += 2;

    /*
     *  insert line number
     */
    *buffer++ = (char) ( line_number & 0xff );
    *buffer++ = (char) ( line_number >> 8   );
    l = 4;

    /*
     *  Skip line number in input line
     */
    while ( isdigit( *line ) ) {
        ++line;
    }

    /*
     *  Encode the rest
     */
    transparent = FALSE;
    in_string = FALSE;

    while ( *line != '\0' && l < BUFFER_SIZE - 3 ) {

        if ( *line == '"' ) {
            in_string = !in_string;
        }
        if ( !in_string && !transparent ) {

            if ( !isdigit( *line ) ) {
                /*
                 *  Look if a token is matched
                 */
                for ( tp = tokens; tp->text != NULL; ++tp ) {
                    tl = strlen( tp->text );
                    if ( 0 == compare( line, tp->text, tl ) ) {
                        break;
                    }
                }
                if ( tp->text != NULL ) {
                    /*
                     *  Replace string by token value
                     */
                    if ( tp->flag & 0x01 ) {
                        /*
                         *  DATA or REM or '
                         */
                        transparent = TRUE;
                    }
                    if ( tp->flag & 0x02 ) {
                        /*
                         *  ELSE or '
                         */
                        *buffer++ = ':';
                        ++l;
                    }
                    if ( tp->flag & 0x04 ) {
                        /*
                         *  '
                         */
                        *buffer++ = 0x8E;   /* REM */
                        ++l;
                    }
                    line += tl;
                    *buffer++ = tp->token;
                    ++l;
                    continue;
                }
            }              
        }
        /*
         *  Simple copy 
         */
        *buffer++ = *line++;
        ++l;
    }
    /*
     *  End of line delimiter
     */
    *buffer = 0x00;
    ++l;

    /*
     *  Update pointer chain
     */
    LinePointer += l;
    lp[ 0 ] = (char) ( LinePointer & 0xFF );
    lp[ 1 ] = (char) ( LinePointer >> 8   );
    return l;
}


/*
 *  Main program
 */
int main( int argc, char *argv[] )
{
    int err = 0;
    int i, l, linecount;
    long line_number;
    char *p, *buff, c;
    int delay;

    ++argv;
    --argc;

    while ( argc > 0 && **argv == '-' ) {
        /*
         *  Options
         */
        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            OutputMode = MODE_BIN;
        }
        if ( strncmp( *argv, "-w", 2 ) == 0 ) {
            OutputMode = MODE_WAVE;
        }
        if ( strncmp( *argv, "-t", 2 ) == 0 ) {
            OutputMode = MODE_TEXT;
            delay = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-s", 2 ) == 0 ) {
            CompressSpaces = TRUE;
        }
        if ( strncmp( *argv, "-c", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
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

    if ( argc < 1 ) {
        printf( "usage: basX07 <options> infile outfile\n"
                "         -b create a binary file\n"
                "         -w create a WAV file\n"
                "         -t create plain text for download via serial"
                             " or USB interface\n"
                "            <delay> is an optional delay in seconds"
                             " after each line\n"
                "         -c set character translation mode:\n"
                "           D DOS charset - code page 850\n"
                "           W Windows charset - code page 1252 (default)\n"
                "           U Unicode (UTF-8) charset\n"
                "           This simplifies the use of national characters\n"
                "         -s remove space characters to save memory\n"
              );
        return 2;
    }

    /*
     *  Open input file
     */
    if ( ( FileIn = fopen( *argv, "rt" ) ) == NULL ) {
        fprintf( stderr, "Cannot open input file\n" );
        perror( *argv );
        return 2;
    }
    ++argv;
    --argc;

    /*
     *  Output file
     */
    if ( argc < 1 ) {
        fprintf( stderr, "Missing output file name\n" );
        return 2;
    }

    switch ( TransMode ) {

    case TRANS_DOS:
        puts( "Using DOS character set CP437" );
        break;

    case TRANS_WINDOWS:
        puts( "Using Windows character set CP1252" );
        break;
    }

    switch ( OutputMode ) {
        
    case MODE_BIN:
        /*
         *  Open output file in binary mode
         */
        FileOut = fopen( *argv, "wb" );
        if ( NULL == FileOut ) {
            fprintf( stderr, "Cannot open binary output file %s\n", *argv );
            perror( *argv );
            return 2;
        }
        printf( "Creating binary file %s\n", *argv );
        break;

    case MODE_WAVE:
        /*
         *  Open the output WAV file
         */
        WaveOut = kcsOpen( *argv, "wb", 1200, 8, 'N', 3 );
        if ( NULL == WaveOut ) {
            /*
             *  I/O error
             */
            fprintf( stderr, "Cannot open wave output file %s\n", *argv );
            if ( errno != 0 ) {
                perror( *argv );
            }
            return 2;
        }
        printf( "Creating wave file %s\n", *argv );
        break;

    case MODE_TEXT:
        /*
         *  Open output file for plain text output
         *  If a delay is specified, binary mode is used.
         */
        FileOut = fopen( *argv, delay == 0 ? "w" : "wb" );
        if ( NULL == FileOut ) {
            fprintf( stderr, "Cannot open text output file %s\n", *argv );
            perror( *argv );
            return 2;
        }
        printf( "Creating text file %s for serial or USB interface\n", *argv );
        break;
    }


    /*
     *  Create file header
     */
    memset( Header.leader, 0xD3, sizeof( Header.leader ) );

    /*
     *  Filename = basename of output file (6 chars)
     */
    memset( Header.file_name, 0, 6 );
    p = strrchr( *argv, '/' );
    if ( p != NULL ) {
        ++p;
    }
    else {
        p = strrchr( *argv, '\\' );
        if ( p != NULL ) {
            ++p;
        }
        else {
            p = strrchr( *argv, ':' );
            if ( p != NULL ) {
                ++p;
            }
            else {
                p = *argv;
            }
        }
    }
     
    i = 0;
    while ( i < 6 && *p != '\0' && *p != '.' ) {
        Header.file_name[ i ] = *p++;
        ++i;
    }

    /*
     *  Process file
     */
    linecount = 0;
    
    switch ( OutputMode ) {
        
    case MODE_BIN:
    case MODE_WAVE:
        /*
         *  Dump header to WAV file
         */
        err = output_segment( &Header, HEADER_SIZE );
        if ( err == 0 ) {
            err = output( KCS_LEAD_IN );
        }    
        line_number = 0;
        LinePointer = BASIC_START;

        /*
         *  Process the input file
         */
        while ( err == 0 ) {
            /*
             *  read the text input file line by line
             */
            l = line_number;
            line_number = read_line( Line, &err );
            if ( line_number == 0 ) {
                /*
                 *  Line has no line number - ignore
                 */
                continue;
            }
            if ( line_number == EOF || line_number <= l ) {
                /*
                 *  EOF or error or lines out of order
                 */
                if ( err != 0 ) {
                    break;
                }
                /*
                 *  Terminate file with 11 zero bytes
                 */
                for ( i = 0; i < 11 && err == 0; ++i ) {
                    err = output( 0 );
                }
                break;
            }
            else {
                ++linecount;
                /*
                 *  Encode the line
                 */
                l = encode( line_number, Line, Buffer );

                /*
                 *  Output the line
                 */
                for ( i = 0; i < l && err == 0; ++i ) {
                    err = output( Buffer[ i ] );
                }
            }
        }
        break;

    case MODE_TEXT:
        /*
         *  Send lines to text file as is
         */
        line_number = 0;

        /*
         *  Process the input file
         */
        while ( err == 0 ) {
            /*
             *  read the text input file line by line
             */
            l = line_number;
            line_number = read_line( Line, &err );
            if ( line_number == 0 ) {
                /*
                 *  Line has no line number - ignore
                 */
                continue;
            }
            if ( line_number == EOF || line_number <= l ) {
                /*
                 *  EOF or error or lines out of order
                 */
                break;
            }
            else {
                ++linecount;
            }

            /*
             *  Terminate line with CR or LF and send it to file
             *  LF is used for file output (no delay), CR for device output.
             */
            l = strlen( Line );
            Line[ l++ ] = delay == 0 ? '\n' : '\r';
            p = Line;
            while ( err == 0 && l-- ) {
                /*
                 *  Write each byte of the line
                 */
                err = output( *p++ & 0xFF );
            }
            if ( delay > 0 ) {
                sleep( delay );
            }
        }
        break;
    }

    if ( err == 0 ) {
        /*
         *  Send a special termination to the file if neccessary
         */
        err = output( EOF );
    }

    /*
     *  Check for errors and close files
     */
    if ( err == 0 && linecount == 0 ) {
        /*
         *  No valid lines found
         */
        err = 1;
    }

    if ( err == 2 ) {
        perror( *argv );
    }
    else if ( err != 0 ) {
        printf( "Error encountered: %s\n", err_msg[ err ] );
    }

    if ( WaveOut != NULL ) {
        kcsClose( WaveOut );
    }
    if ( FileOut != NULL ) {
        fclose( FileOut );
    }
    fclose( FileIn );

    return err != 0 ? 2 : 0;
}
