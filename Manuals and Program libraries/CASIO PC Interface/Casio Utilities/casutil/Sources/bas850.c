/*
 *  bas850.c - Create Casio tape images from BASIC source
 *
 *  This program creates a binary tape image file for the Casio
 *  Casio calculators PB-700, FX-750P, FP-200, FX-850P or PB-1000.
 *
 *  Options:
 *    -b create BIN file to be converted by wave850 later (Default)
 *    -a create ASCII file (Piotr's format)
 *    -s, -w create a 300  baud WAV file (slow: PB-700, FX-750P, FP-200)
 *    -f create a 1200 baud WAV file (fast: FX-850P, PB-1000)
 *    -2  FP-200 specifics, use together with -A
 *    -tA SAVE,A output
 *    -tB same as A but creates larger blocks for FP-200 and FX-850P
 *    -t2 tokenize for FP-200
 *    -t7 tokenize for PB-700 family 
 *    -t8 tokenize for FX-850 family
 *    -u make all data uppercase (for FX-750P)
 *    -l allow lowercase (for FP-200, FX-850P, PB-1000)
 *    -p force PB-700 escape syntax (for FP-200, PB-1000)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"

#define LEAD_IN_TIME 20       /* tens of a second */
#define LEAD_IN_TIME_FIRST 20 /* tens of a second */
#define LEAD_IN_TIME_DATA  20 /* tens of a second */
#define LEAD_IN_TIME_FP200 40 /* tens of a second */
#define LEAD_IN_ASCII 80      /* Number of groups of six '1' bits */

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
    unsigned char prefix;
} TOKEN; 

/*
 *  Escape codes with \, work everywhere
 */
TOKEN Escapes_FX[] = 
{
    { "\\", 0x5C },  /* backslash */
    { "YN", 0x5C },  /* Yen */
    { "AN", 0x80 },  /* Angstrom */
    { "IN", 0x81 },  /* integral */
    { "RT", 0x82 },  /* root */
    { "/",  0x83 },  /* over */
    { "SM", 0x84 },  /* Sigma, sum */
    { "OM", 0x85 },  /* Omega */
    { "]",  0x86 },  /* grey block */
    { "#",  0x87 },  /* block */
    { "AL", 0x88 },  /* alpha */
    { "BT", 0x89 },  /* beta */
    { "GA", 0x8A },  /* gamma */
    { "EP", 0x8B },  /* epsilon */
    { "TH", 0x8C },  /* theta */
    { "PH", 0x8C },  /* used to be phi */
    { "MU", 0x8D },  /* mu */
    { "SI", 0x8E },  /* sigma */
    { "PS", 0x8F },  /* psi */
    { "S0", 0x90 },  /* small 0 */
    { "S1", 0x91 },  /* small 0 */
    { "S2", 0x92 },  /* small 0 */
    { "S3", 0x93 },  /* small 0 */
    { "S4", 0x94 },  /* small 0 */
    { "S5", 0x95 },  /* small 0 */
    { "S6", 0x96 },  /* small 0 */
    { "S7", 0x97 },  /* small 0 */
    { "S8", 0x98 },  /* small 0 */
    { "S9", 0x99 },  /* small 0 */
    { "S+", 0x9A },  /* small + */
    { "S-", 0x9B },  /* small - */
    { "SN", 0x9C },  /* small N */
    { "SX", 0x9D },  /* small X */
    { "-1", 0x9E },  /* small -1 */
    { ":",  0x9F },  /* divide */
    { ".",  0xA5 },  /* dot */
    { "DG", 0xDF },  /* degree */
    { ">=", 0xE0 },  /* >= */
    { "<=", 0xE1 },  /* <= */
    { "<>", 0xE2 },  /* <> */
    { "^",  0xE3 },  /* up arrow */
    { "<-", 0xE4 },  /* left arrow */
    { "V",  0xE5 },  /* down arrow */
    { "->", 0xE6 },  /* right arrow */
    { "PI", 0xE7 },  /* PI */
    { "SP", 0xE8 },  /* spade */
    { "HT", 0xE9 },  /* heart */
    { "DI", 0xEA },  /* diamond */
    { "CL", 0xEB },  /* club */
    { "SQ", 0xEC },  /* sQuare */
    { "@",  0xED },  /* circle */
    { "TR", 0xEF },  /* triangle */
    { "*",  0xF0 },  /* multiply */
    { "PN", 0xF7 },  /* pound */
    { "CN", 0xF8 },  /* cent */
    { "+-", 0xF9 },  /* +/- */
    { "-+", 0xFA },  /* -/+ */
    { NULL, 0    }
};

/*
 *  Escape codes with \, work everywhere
 *  Subset for PB-700/PB-1000/FP-200
 */
TOKEN Escapes_PB[] = 
{
    { "YN", 0x5C },  /* Yen */
    { "_1", 0x80 },  /* graphics */
    { "_2", 0x81 },  /* graphics */
    { "_3", 0x82 },  /* graphics */
    { "_4", 0x83 },  /* graphics */
    { "_5", 0x84 },  /* graphics */
    { "_6", 0x85 },  /* graphics */
    { "_7", 0x86 },  /* graphics */
    { "_8", 0x87 },  /* graphics */
    { "#",  0x87 },  /* block */
    { "|1", 0x88 },  /* graphics */
    { "|2", 0x89 },  /* graphics */
    { "|3", 0x8A },  /* graphics */
    { "|4", 0x8B },  /* graphics */
    { "|5", 0x8C },  /* graphics */
    { "|6", 0x8D },  /* graphics */
    { "|7", 0x8E },  /* graphics */
    { "^",  0x90 },  /* up arrow */
    { "V",  0x91 },  /* down arrow */
    { "<-", 0x92 },  /* left arrow */
    { "->", 0x93 },  /* right arrow */
    { ".",  0xA5 },  /* dot */
    { "DG", 0xDF },  /* degree */
    { "TR", 0xE4 },  /* triangle */
    { "SP", 0xE8 },  /* spade */
    { "HT", 0xE9 },  /* heart */
    { "DI", 0xEA },  /* diamond */
    { "CL", 0xEB },  /* club */
    { "LD", 0xEC },  /* large dot */
    { "@",  0xED },  /* circle */
    { "/",  0x83 },  /* over */
    { "\\", 0xEF },  /* backslash */
    { "]",  0xFE },  /* grey block */
    { NULL, 0    }
};

/*
 *  Unicode (UTF-8) code table for FX series
 */
struct _utf8 {
    unsigned char token;
    char *text;
} Utf8_FX[] = 
{
    { 0x5C, "\xC2\xA5" },
    { 0x80, "\xC3\x85" },
    { 0x81, "\xE2\x88\xAB" },
    { 0x82, "\xE2\x88\x9A" },
    { 0x83, "\xC2\xB4" },
    { 0x84, "\xCE\xA3" },
    { 0x85, "\xCE\xA9" },
    { 0x86, "\xE2\x96\x91" },
    { 0x87, "\xE2\x96\xA0" },
    { 0x88, "\xCE\xB1" },
    { 0x89, "\xCE\xB2" },
    { 0x8A, "\xCE\xB3" },
    { 0x8B, "\xCE\xB5" },
    { 0x8C, "\xCE\xB8" },
    { 0x8D, "\xCE\xBC" },
    { 0x8E, "\xCF\x83" },
    { 0x8F, "\xCF\x95" },
    { 0x90, "\xE2\x81\xB0" },
    { 0x91, "\xC2\xB9" },
    { 0x92, "\xC2\xB2" },
    { 0x93, "\xC2\xB3" },
    { 0x94, "\xE2\x81\xB4" },
    { 0x95, "\xE2\x81\xB5" },
    { 0x96, "\xE2\x81\xB6" },
    { 0x97, "\xE2\x81\xB7" },
    { 0x98, "\xE2\x81\xB8" },
    { 0x99, "\xE2\x81\xB9" },
    { 0x9A, "\xE2\x81\xBA" },
    { 0x9B, "\xE2\x81\xBB" },
    { 0x9C, "\xE2\x81\xBF" },
    { 0x9D, "\x78" },
    { 0x9E, "\xE2\x81\xBB\xC2\xB9" },
    { 0x9F, "\xC3\xB7" },
    { 0xA0, "\xC2\xA0" },
    { 0xA1, "\xE3\x80\x82" },
    { 0xA2, "\xE3\x80\x8C" },
    { 0xA3, "\xE3\x80\x8D" },
    { 0xA4, "\xE3\x80\x81" },
    { 0xA5, "\xE3\x83\xBB" },
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
    { 0xE0, "\xE2\x89\xA5" },
    { 0xE1, "\xE2\x89\xA4" },
    { 0xE2, "\xE2\x89\xA0" },
    { 0xE3, "\xE2\x86\x91" },
    { 0xE4, "\xE2\x86\x90" },
    { 0xE5, "\xE2\x86\x93" },
    { 0xE6, "\xE2\x86\x92" },
    { 0xE7, "\xCF\x80" },
    { 0xE8, "\xE2\x99\xA0" },
    { 0xE9, "\xE2\x99\xA5" },
    { 0xEA, "\xE2\x99\xA6" },
    { 0xEB, "\xE2\x99\xA3" },
    { 0xEC, "\xE2\x97\xBB" },
    { 0xED, "\xE2\x97\x8B" },
    { 0xEE, "\xE2\x88\x86" },
    { 0xEF, "\xE2\x9F\x8B" },
    { 0xF0, "\xC3\x97" },
    { 0xF1, "\xE5\x86\x86" },
    { 0xF2, "\xE5\xB9\xB4" },
    { 0xF3, "\xE2\xBD\x89" },
    { 0xF4, "\xE2\xBD\x87" },
    { 0xF5, "\xE5\x8D\x83" },
    { 0xF6, "\xE3\x84\xAA" },
    { 0xF7, "\xC2\xA3" },
    { 0xF8, "\xC2\xA2" },
    { 0xF9, "\xC2\xB1" },
    { 0xFA, "\xE2\x88\x93" },
    { 0xFB, "\xE2\x82\x92" },
    { 0, NULL }
};

/*
 *  Unicode (UTF-8) code table for PB series
 */
struct _utf8 Utf8_PB[] = 
{
    { 0x5C, "\xC2\xA5" },
    { 0x80, "\xE2\x96\x81" },
    { 0x81, "\xE2\x96\x82" },
    { 0x82, "\xE2\x96\x83" },
    { 0x83, "\xE2\x96\x84" },
    { 0x84, "\xE2\x96\x85" },
    { 0x85, "\xE2\x96\x86" },
    { 0x86, "\xE2\x96\x87" },
    { 0x87, "\xE2\x96\x88" },
    { 0x88, "\xE2\x96\x8F" },
    { 0x89, "\xE2\x96\x8E" },
    { 0x8A, "\xE2\x96\x8D" },
    { 0x8B, "\xE2\x96\x8C" },
    { 0x8C, "\xE2\x96\x8B" },
    { 0x8D, "\xE2\x96\x8A" },
    { 0x8E, "\xE2\x96\x89" },
    { 0x8F, "\xE2\x94\xBC" },
    { 0x90, "\xE2\x94\xB4" },
    { 0x91, "\xE2\x94\xAC" },
    { 0x92, "\xE2\x94\xA4" },
    { 0x93, "\xE2\x94\x9C" },
    { 0x94, "\xE2\x8E\xB9" },
    { 0x95, "\xE2\x94\x80" },
    { 0x96, "\xE2\x94\x82" },
    { 0x97, "\xE2\x8E\xBA" },
    { 0x98, "\xE2\x94\x8C" },
    { 0x99, "\xE2\x94\x90" },
    { 0x9A, "\xE2\x94\x94" },
    { 0x9B, "\xE2\x94\x98" },
    { 0x9C, "\xE2\x95\xAD" },
    { 0x9D, "\xE2\x95\xAE" },
    { 0x9E, "\xE2\x95\xB0" },
    { 0x9F, "\xE2\x95\xAF" },
    { 0xA0, "\xC2\xA0" },
    { 0xA1, "\xE3\x80\x82" },
    { 0xA2, "\xE3\x80\x8C" },
    { 0xA3, "\xE3\x80\x8D" },
    { 0xA4, "\xE3\x80\x81" },
    { 0xA5, "\xE3\x83\xBB" },
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
    { 0xE0, "\xE2\x95\x90" },
    { 0xE1, "\xE2\x95\x9E" },
    { 0xE2, "\xE2\x95\xAA" },
    { 0xE3, "\xE2\x95\xA1" },
    { 0xE4, "\xE2\x97\xA2" },
    { 0xE5, "\xE2\x97\xA3" },
    { 0xE6, "\xE2\x97\xA5" },
    { 0xE7, "\xE2\x97\xA4" },
    { 0xE8, "\xE2\x99\xA0" },
    { 0xE9, "\xE2\x99\xA5" },
    { 0xEA, "\xE2\x99\xA6" },
    { 0xEB, "\xE2\x99\xA3" },
    { 0xEC, "\xE2\x97\x8F" },
    { 0xED, "\xE2\x97\x8B" },
    { 0xEE, "\xE2\x95\xB1" },
    { 0xEF, "\xE2\x95\xB2" },
    { 0xF0, "\xC3\x97" },
    { 0xF1, "\xE5\x86\x86" },
    { 0xF2, "\xE5\xB9\xB4" },
    { 0xF3, "\xE2\xBD\x89" },
    { 0xF4, "\xE2\xBD\x87" },
    { 0xF5, "\xE6\x99\x82" },
    { 0xF6, "\xE5\x88\x86" },
    { 0xF7, "\xE7\xA7\x92" },
    { 0xF8, "\xE3\x80\x92" },
    { 0xF9, "\xE5\xB8\x82" },
    { 0xFA, "\xE5\x8C\xBA" },
    { 0xFB, "\xE7\x94\xBA" },
    { 0xFC, "\xE6\x9D\x91" },
    { 0xFD, "\xE4\xBA\xBA" },
    { 0xFE, "\xE2\x96\x91" },
    { 0, NULL }
};

/*
 *  PB-700 / FX-750 keywords
 *  Single byte tokens from 0x80 to 0xEC
 *  Flag 0x40 marks commands that start transparent mode
 */
TOKEN Tokens700[] =
{
    { "WAIT",    0xD6 },
    { "VERIFY",  0xC4 },
    { "VAL(",    0x92 },
    { "USING",   0xB4 },
    { "TRON",    0xBE },
    { "TROFF",   0xBF },
    { "TO",      0xB3 },
    { "THEN",    0xB2 },
    { "TAN",     0x82 },
    { "TAB(",    0xB5 },
    { "SYSTEM",  0xD0 },
    { "SUMY2",   0xDF },
    { "SUMY",    0xDB },
    { "SUMXY",   0xDD },
    { "SUMX2",   0xDE },
    { "SUMX",    0xDC },
    { "STR$(",   0x9B },
    { "STOP",    0xA6 },
    { "STEP",    0xB1 },
    { "STAT",    0xD2 },
    { "SRAM",    0xD4 },
    { "SQR",     0x89 },
    { "SIN",     0x80 },
    { "SGN",     0x8D },
    { "SDYN",    0xE5 },
    { "SDY",     0xE3 },
    { "SDXN",    0xE4 },
    { "SDX",     0xE2 },
    { "SAVE",    0xC6 },
    { "RUN",     0xCC },
    { "ROUND(",  0x8F },
    { "ROUND",   0xD3 },
    { "RND",     0x94 },
    { "RIGHT$(", 0x9D },
    { "RETURN",  0xA2 },
    { "RESTORE", 0xA9 },
    { "REM",     0xB8, 0x40 },
    { "READ",    0xA8 },
    { "PUT",     0xC2 },
    { "PROG",    0xC1 },
    { "PRINT",   0xAE },
    { "POKE",    0xEC },
    { "POINT(",  0x8E },
    { "PI",      0x93 },
    { "PEEK",    0xEB },
    { "PASS",    0xC8 },
    { "ON",      0xD7 },
    { "OFF",     0xD8 },
    { "NEXT",    0xA4 },
    { "NEW",     0xC9 },
    { "MOD",     0x9F },
    { "MID$(",   0x9E },
    { "MEANY",   0xE1 },
    { "MEANX",   0xE0 },
    { "LRB",     0xE7 },
    { "LRAM",    0xD5 },
    { "LRA",     0xE6 },
    { "LPRINT",  0xAF },
    { "LOG",     0x87 },
    { "LOCATE",  0xAD },
    { "LOAD",    0xC7 },
    { "LLIST",   0xCB },
    { "LIST",    0xCA },
    { "LGT",     0x88 },
    { "LET",     0xB9 },
    { "LEN(",    0x91 },
    { "LEFT$(",  0x9C },
    { "INT",     0x8C },
    { "INPUT",   0xA7 },
    { "INKEY$",  0x99 },
    { "IF",      0xA5 },
    { "HYP",     0x96 },
    { "HEX$(",   0x97 },
    { "GOTO",    0xA0 },
    { "GOSUB",   0xA1 },
    { "GET",     0xC3 },
    { "FRAC",    0x8B },
    { "FOR",     0xA3 },
    { "EXP",     0x83 },
    { "ERASE",   0xBD },
    { "EOY",     0xEA },
    { "EOX",     0xE9 },
    { "END",     0xAA },
    { "ELSE",    0xB0 },
    { "EDIT",    0xCE },
    { "DRAWC(",  0xAC },
    { "DRAW(",   0xAB },
    { "DMS$(",   0x98 },
    { "DIM",     0xBC },
    { "DELETE",  0xCD },
    { "DEG(",    0x95 },
    { "DATA",    0xB7, 0x40 },
    { "COS",     0x81 },
    { "COR",     0xE8 },
    { "CONT",    0xCF },
    { "CNT",     0xDA },
    { "CLS",     0xD1 },
    { "CLEAR",   0xC0 },
    { "CHR$(",   0x9A },
    { "CHAIN",   0xC5 },
    { "BEEP",    0xBB },
    { "AUTO",    0xD9 },
    { "ATN",     0x86 },
    { "ASN",     0x84 },
    { "ASC(",    0x90 },
    { "ANGLE",   0xBA },
    { "ALL",     0xB6 },
    { "ACS",     0x85 },
    { "ABS",     0x8A },
    { NULL,      0x00 }
}; 
                     
/*
 *  FX-850 keywords - works for PB-1000 with option -lF set
 *  Double byte tokens from 0x40 to 0xCF, prefix 4 to 7
 *  Flag 0x80 marks commands that are followed by linenumbers
 *  Flag 0x40 marks commands that start transparent mode
 *  Flag 0x20 marks ELSE (to be preceded by statement delimiter)
 */
TOKEN Tokens850[] =
{
    { "XOR",       0xC6, 7 },
    { "WRITE#",    0x4E, 4 },
    { "VERIFY",    0x60, 4 },
    { "VAR",       0xB2, 4 },
    { "VALF",      0x92, 5 },
    { "VAL",       0x96, 5 },
    { "VAC",       0x7A, 4 },
    { "USING",     0xC2, 7 },
    { "TRON",      0x5D, 4 },
    { "TROFF",     0x5F, 4 },
    { "TO",        0xC1, 7 },
    { "TIME$",     0xAC, 6 },
    { "THEN",      0x47, 7 + 0x80 },
    { "TAN",       0x6D, 5 },
    { "TAB",       0xB6, 7 },
    { "SYSTEM",    0x52, 4 },
    { "SUMY2",     0x55, 5 },
    { "SUMY",      0x53, 5 },
    { "SUMXY",     0x56, 5 },
    { "SUMX2",     0x54, 5 },
    { "SUMX",      0x52, 5 },
    { "STR$",      0xA1, 6 },
    { "STOP",      0xAE, 4 },
    { "STEP",      0xC0, 7 },
    { "STAT",      0xAD, 4 },
    { "SQR",       0x7A, 5 },
    { "SIN",       0x6B, 5 },
    { "SGN",       0x7C, 5 },
    { "SET",       0xAC, 4 },
    { "SDYN",      0x5C, 5 },
    { "SDY",       0x5A, 5 },
    { "SDXN",      0x5B, 5 },
    { "SDX",       0x59, 5 },
    { "SAVE",      0x6C, 4 },
    { "RUN",       0x6D, 4 },
    { "RSET",      0xAA, 4 },
    { "ROUND",     0x90, 5 },
    { "RND",       0x91, 5 },
    { "RND",       0x80, 5 },
    { "RIGHT$",    0x9D, 6 },
    { "REV",       0xB9, 7 },
    { "RETURN",    0x4B, 4 + 0x80 },
    { "RESUME",    0x4C, 4 + 0x80 },
    { "RESTORE",   0x4D, 4 + 0x80 },
    { "REM",       0xA9, 4 + 0x40 },
    { "REC",       0xA7, 5 },
    { "READ",      0xA8, 4 },
    { "RAN#",      0x93, 5 },
    { "PUT",       0xA5, 4 },
    { "PRINT",     0xA3, 4 },
    { "POL",       0xA8, 5 },
    { "POKE",      0x63, 4 },
    { "POINT",     0x8F, 5 },
    { "PI",        0x60, 5 },
    { "PEEK",      0x86, 5 },
    { "PBLOAD",    0xB3, 4 },
    { "PBGET",     0xB4, 4 },
    { "PASS",      0x53, 4 },
    { "OUT",       0x99, 4 },
    { "OR",        0xC5, 7 },
    { "OPEN",      0x97, 4 },
    { "ON",        0x9A, 4 },
    { "OFF",       0xBF, 7 },
    { "NPR",       0xAA, 5 },
    { "NOT",       0xC3, 7 },
    { "NORM",      0xBA, 7 },
    { "NEXT",      0x82, 4 },
    { "NEW",       0x6B, 4 },
    { "NCR",       0xAB, 5 },
    { "MON",       0x61, 4 },
    { "MODE",      0xB0, 4 },
    { "MOD",       0xC7, 7 },
    { "MID$",      0x9C, 6 },
    { "MID",       0x9A, 6 },
    { "MERGE",     0x5A, 4 },
    { "MEANY",     0x58, 5 },
    { "MEANX",     0x57, 5 },
    { "LSET",      0x93, 4 },
    { "LRB",       0x5E, 5 },
    { "LRA",       0x5D, 5 },
    { "LPRINT",    0xA4, 4 },
    { "LOG",       0x78, 5 },
    { "LOF",       0x89, 5 },
    { "LOCATE",    0x91, 4 },
    { "LOAD",      0x59, 4 },
    { "LN",        0x77, 5 },
    { "LLIST",     0x58, 4 + 0x80 },
    { "LIST",      0x57, 4 + 0x80 },
    { "LINE",      0x90, 4 },
    { "LET",       0x8F, 4 },
    { "LEN",       0x95, 5 },
    { "LEFT$",     0x9E, 6 },
    { "KEY",       0xA9, 6 },
    { "INT",       0x7D, 5 },
    { "INPUT",     0x9B, 6 },
    { "INKEY$",    0xA8, 6 },
    { "IF",        0x8D, 4 },
    { "HYPTAN",    0x73, 5 },
    { "HYPSIN",    0x71, 5 },
    { "HYPCOS",    0x72, 5 },
    { "HYPATN",    0x76, 5 },
    { "HYPASN",    0x74, 5 },
    { "HYPACS",    0x75, 5 },
    { "HYP",       0xAC, 5 },
    { "HEX$",      0xA3, 6 },
    { "GOTO",      0x49, 4 + 0x80 },
    { "GOSUB",     0x4A, 4 + 0x80 },
    { "GET",       0x8C, 4 },
    { "FRE",       0x8D, 5 },
    { "FRAC",      0x7F, 5 },
    { "FORMAT",    0x8B, 4 },
    { "FOR",       0x81, 4 },
    { "FIX",       0x7E, 5 },
    { "FIELD",     0x8A, 4 },
    { "FACT",      0x67, 5 },
    { "EXP",       0x79, 5 },
    { "ERROR",     0x86, 4 },
    { "ERR",       0x50, 5 },
    { "ERL",       0x4F, 5 },
    { "ERASE",     0x85, 4 },
    { "EOY",       0x6A, 5 },
    { "EOX",       0x69, 5 },
    { "EOF",       0x8A, 5 },
    { "END",       0x87, 4 },
    { "ELSE",      0x48, 7 + 0x80 + 0x20 },
    { "EDIT",      0x6F, 4 },
    { "DRAWC",     0xA2, 4 },
    { "DRAW",      0x7D, 4 },
    { "DMS$",      0x97, 6 },
    { "DIM",       0x7C, 4 },
    { "DELETE",    0x55, 4 },
    { "DEG",       0x9C, 5 },
    { "DEFSEG",    0x78, 4 },
    { "DEFM",      0x77, 4 },
    { "DEF",       0x76, 4 },
    { "DATE$",     0xAB, 6 },
    { "DATA",      0x80, 4 + 0x40 },
    { "CUR",       0x63, 5 },
    { "CSR",       0xB8, 7 },
    { "COS",       0x6C, 5 },
    { "COR",       0x5F, 5 },
    { "CNT",       0x51, 5 },
    { "CLS",       0x71, 4 },
    { "CLOSE",     0x72, 4 },
    { "CLEAR",     0x6A, 4 },
    { "CHR$",      0xA0, 6 },
    { "CHAIN",     0x69, 4 },
    { "CALL",      0x62, 4 },
    { "CALCJMP",   0x9F, 4 },
    { "CALC$",     0xAD, 6 },
    { "BSAVE",     0x56, 4 },
    { "BLOAD",     0xA0, 4 },
    { "BEEP",      0x70, 4 },
    { "ATN",       0x70, 5 },
    { "ASN",       0x6E, 5 },
    { "ASC",       0x94, 5 },
    { "AS",        0xBC, 7 },
    { "APPEND",    0xBD, 7 },
    { "ANGLE",     0x6E, 4 },
    { "AND",       0xC4, 7 },
    { "ALL",       0xBB, 7 },
    { "ACS",       0x6F, 5 },
    { "ABS",       0x7B, 5 },
    { NULL ,       0x00, 0 }
};

/*
 *  FB-200 keywords
 *  Double byte tokens from 0x25 to 0xB0, prefix 4
 *  Flags as above
 */
TOKEN Tokens200[] =
{
    { "XOR",       0x71, 4 },
    { "VERIFY",    0x64, 4 },
    { "VAL",       0xA4, 4 },
    { "USING",     0x6A, 4 },
    { "TRON",      0x37, 4 },
    { "TROFF",     0x38, 4 },
    { "TO",        0x6E, 4 },
    { "TIME$",     0xAF, 4 },
    { "THEN",      0x27, 4 + 0x80 },
    { "TAN",       0x8E, 4 },
    { "TAB",       0x84, 4 },
    { "SYSTEM",    0x57, 4 },
    { "SUMY2",     0x78, 4 },
    { "SUMY",      0x76, 4 },
    { "SUMXY",     0x79, 4 },
    { "SUMX2",     0x77, 4 },
    { "SUMX",      0x75, 4 },
    { "SUMRC",     0x8A, 4 },
    { "SUMIT",     0x8B, 4 },
    { "STR$",      0xA8, 4 },
    { "STOP",      0x43, 4 },
    { "STEP",      0x6D, 4 },
    { "STAT",      0x31, 4 },
    { "SQR",       0x95, 4 },
    { "SIN",       0x8C, 4 },
    { "SGN",       0x97, 4 },
    { "SDYN",      0x7F, 4 },
    { "SDY",       0x7D, 4 },
    { "SDXN",      0x7E, 4 },
    { "SDX",       0x7C, 4 },
    { "SAVE",      0x4E, 4 },
    { "RUN",       0x2A, 4 },
    { "RSET",      0x51, 4 },
    { "ROUND",     0x85, 4 },
    { "RND",       0x9B, 4 },
    { "RIGHT$",    0xAC, 4 },
    { "RETURN",    0x35, 4 },
    { "RESTORE",   0x29, 4 + 0x80 },
    { "RESET",     0x66, 4 },
    { "RENUM",     0x2E, 4 },
    { "REM",       0x3F, 4 + 0x40 },
    { "READ",      0x41, 4 },
    { "RC",        0x88, 4 },
    { "RANDOMIZE", 0x5E, 4 },
    { "QUAD",      0x4B, 4 },
    { "PUT",       0x53, 4 },
    { "PROG",      0x2F, 4 },
    { "PRINT",     0x5A, 4 },
    { "POKE",      0x4C, 4 },
    { "POINT",     0x86, 4 },
    { "PEEK",      0x9E, 4 },
    { "PASS",      0x5C, 4 },
    { "OUTPUT",    0x6C, 4 },
    { "OR",        0x6F, 4 },
    { "OPTION",    0x36, 4 },
    { "OPEN",      0x55, 4 },
    { "ON",        0x34, 4 },
    { "NOT",       0x72, 4 },
    { "NEXT",      0x45, 4 },
    { "NEW",       0x30, 4 },
    { "MOUNT",     0x59, 4 },
    { "MOD",       0x73, 4 },
    { "MKS$",      0xA9, 4 },
    { "MKD$",      0xAA, 4 },
    { "MID$",      0xAB, 4 },
    { "MEANY",     0x7B, 4 },
    { "MEANX",     0x7A, 4 },
    { "LSET",      0x50, 4 },
    { "LRB",       0x81, 4 },
    { "LRA",       0x80, 4 },
    { "LPRINT",    0x5B, 4 },
    { "LOG",       0x92, 4 },
    { "LOF",       0xA0, 4 },
    { "LOCATE",    0x47, 4 },
    { "LOC",       0x9F, 4 },
    { "LOAD",      0x4F, 4 },
    { "LLIST",     0x2D, 4 },
    { "LIST",      0x2C, 4 },
    { "LGT",       0x93, 4 },
    { "LET",       0x5F, 4 },
    { "LEN",       0xA2, 4 },
    { "LEFT$",     0xAD, 4 },
    { "KILL",      0x63, 4 },
    { "KEY",       0x5D, 4 },
    { "IT",        0x89, 4 },
    { "INT",       0x98, 4 },
    { "INPUT",     0x46, 4 },
    { "INKEY$",    0xAE, 4 },
    { "INIT",      0x49, 4 },
    { "IF",        0x33, 4 },
    { "GOTO",      0x25, 4 + 0x80 },
    { "GOSUB",     0x26, 4 + 0x80 },
    { "GET",       0x54, 4 },
    { "FRE",       0x83, 4 },
    { "FRAC",      0x9A, 4 },
    { "FORMAT",    0x62, 4 },
    { "FOR",       0x44, 4 },
    { "FN",        0x82, 4 },
    { "FL",        0x87, 4 },
    { "FIX",       0x99, 4 },
    { "FILES",     0x61, 4 },
    { "FILE",      0x67, 4 },
    { "FIELD",     0x52, 4 },
    { "EXP",       0x94, 4 },
    { "EOF",       0xA1, 4 },
    { "END",       0x42, 4 },
    { "ELSE",      0x28, 4 + 0x80 + 0x20 },
    { "EDIT",      0x2B, 4 },
    { "DRAW",      0x4A, 4 },
    { "DIM",       0x32, 4 },
    { "DEFSTR",    0x3B, 4 },
    { "DEFSNG",    0x39, 4 },
    { "DEFDBL",    0x3A, 4 },
    { "DEF",       0x3C, 4 },
    { "DATE$",     0xB0, 4 },
    { "DATA",      0x40, 4 + 0x40 },
    { "CVS",       0xA6, 4 },
    { "CVD",       0xA5, 4 },
    { "CSNG",      0x9C, 4 },
    { "COS",       0x8D, 4 },
    { "CNT",       0x74, 4 },
    { "CLS",       0x48, 4 },
    { "CLOSE",     0x56, 4 },
    { "CLEAR",     0x58, 4 },
    { "CHR$",      0xA7, 4 },
    { "CDBL",      0x9D, 4 },
    { "CALL",      0x4D, 4 },
    { "BASE",      0x6B, 4 },
    { "ATN",       0x91, 4 },
    { "ASN",       0x8F, 4 },
    { "ASC",       0xA3, 4 },
    { "AS",        0x69, 4 },
    { "AREA",      0x65, 4 },
    { "ANGLE",     0x60, 4 },
    { "AND",       0x70, 4 },
    { "ALL",       0x68, 4 },
    { "ACS",       0x90, 4 },
    { "ABS",       0x96, 4 },
    { NULL ,       0x00, 0 }
};

/*
 *  Old style - new style translation table
 */
struct _translate {
    char *from;
    char *to;
} translation_table[] = 
{
    { "VAC",     "CLEAR"     },
    { "DEFM",    "DIMZ("     },
    { "CSR",     "TAB("      },
    { "INKEY$",  "INKEY$"    },
    { "KEY$",    "INKEY$"    },
    { "KEY",     "INKEY$"    },
    { "RND",     "ROUND"     },
    { "MODE",    "ANGLE-4+"  },
    { "RETURN",  "RETURN"    },
    { "RET",     "RETURN"    },
    { "GSB",     "GOSUB"     },
    { "PRT",     "PRINT"     },
    { "INPUT",   "INPUT"     },
    { "INP",     "INPUT"     },
    { "DMS$",    "DMS$"      },
    { "DMS",     "PRINTDMS$" },
    { "HSN",     "HYPSIN"    },
    { "HCS",     "HYPCOS"    },
    { "HTN",     "HYPTAN"    },
    { "AHS",     "HYPASN"    },
    { "AHC",     "HYPACS"    },
    { "AHT",     "HYPATN"    },
    { "SAC",     "STATCLEAR" },
    { "SX",      "SUMX"      },
    { "SY",      "SUMY"      },
    { "SX2",     "SUMX2"     },
    { "SY2",     "SUMY2"     },
    { "SXY",     "SUMXY"     },
    { NULL,      NULL        }
};

/*
 *  Logarithm translation tables
 */
struct _translate translation_log_PB[] = 
{
    { "LOG",     "LGT"       },
    { "LN",      "LOG"       },
    { NULL,      NULL        }
};

struct _translate translation_log_FX[] = 
{
    { "LOG",     "LN"        },
    { "LGT",     "LOG"       },
    { NULL,      NULL        }
};

/*
 *  File handling
 */
KCS_FILE *WaveOut = NULL;
FILE *FileOut = NULL;
FILE *FileIn = NULL;

/*
 *  Output mode and some flags
 */
enum { MODE_BIN, MODE_ASCII, MODE_WAVE, MODE_TEXT } OutputMode = MODE_BIN;
bool Uppercase = FALSE;
bool Lowercase = FALSE;
bool Translate = FALSE;
bool JisBasic = FALSE;
bool PbEscapes = FALSE;
bool Fp200 = FALSE;
bool AsciiLargeBlocks = FALSE;
int LeadInTime;
int DataLines;
enum { LOG_NONE, LOG_FX, LOG_PB } LogarithmMode = LOG_NONE;
enum { UTF8_NONE, UTF8_FX, UTF8_PB } Utf8Mode = UTF8_NONE; 


/*
 *  Header bytes
 */
struct _header {
    char segment_id;
    unsigned char file_type;
    char file_name[ 8 ];
    char file_ext[ 3 ];
    unsigned char password[ 8 ];
    unsigned char parameters[ 12 ];
} Header;

#define HEADER_SIZE  33
#define TYPE_PROGRAM 0xD0
#define TYPE_ALL     0xC1
#define TYPE_ASCII   0x30
#define TYPE_DATA    0x24
#define TYPE_PROG850 0x10
#define TYPE_ALL850  0x90
#define TYPE_TEXT    0

#ifdef __MSDOS__
#define DATA_SIZE 45000
#else
#define DATA_SIZE 65536
#endif
/*
 *  Data segment, large enough for a 64KB program (45KB in DOS)
 */
static struct _data {
    char segment_id;
    unsigned char file_type;
    unsigned char eof_marker;
    unsigned char len_lsb, len_msb;
    char data[ DATA_SIZE ];
} *Data;


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
 *  Output a byte to the BIN, ASC or WAV file
 *  Returns 0 when OK or the error code when invalid data encountered
 */
int output( int c )
{
    static int lead_in_time = LEAD_IN_TIME_FIRST;
    static int column_count = 0;
    int i;
    int parity;
    int bit;
    int byte;
    int mask;

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

    case MODE_ASCII:
        /*
         *  ASCII mode
         */
        switch ( c ) {

        case KCS_LEAD_IN:
            /*
             *  Write a fixed number of '1' bits
             */
            if ( column_count != 0 ) { 
                fputc( '\n', FileOut );
                column_count = 0;
            }
            for ( i = 0; i < LEAD_IN_ASCII && errno == 0; ++i ) { 
                fputc( 0x3F + 0x30, FileOut );
                if ( ++column_count == 64 ) {
                    fputc( '\n', FileOut );
                    column_count = 0;
                }
            }
            break;

        case EOF:
            /*
             *  Add a line feed, if neccessary
             */
            if ( column_count != 0 ) { 
                fputc( '\n', FileOut );
                column_count = 0;
            }
            break;

        default:
            /*
             *  Encode byte to 12 bits with even parity and split in two bytes
             */
            byte = c;

            /*
             *  Startbit is zero, start with next bit in output character
             */
            c = 0;
            mask = 2;
            parity = 0;
            
            /*
             *  The other 11 bits to send
             */
            for ( i = 1; i < 12; ++i ) {
                if ( i <= 8 ) {
                    /*
                     *  data bits, LSB first
                     */
                    bit = byte & 1;
                    byte >>= 1;
                    if ( bit ) {
                        c |= mask;
                        parity ^= 1;
                    }
                }
                else {
                    /*
                     *  Parity and stop bits
                     */
                    if ( parity || i >= 10 ) {
                        c |= mask;
                    }
                }
                mask <<= 1;
                if ( mask == 0x40 ) {
                    /*
                     *  Output this group of bits and start over
                     */
                    c += 0x30;
                    if ( c != fputc( c, FileOut ) ) {
                        /*
                         *  I/O error
                         */
                        break;
                    }
                    c = 0;
                    mask = 1;
                    /*
                     *  Add a line feed, if neccessary
                     */
                    if ( ++column_count == 64 ) { 
                        fputc( '\n', FileOut );
                        column_count = 0;
                    }
                }
            }
        }
            
        if ( errno != 0 ) {
            /*
             *  I/O error
             */
            return 2;
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
            lead_in_time = LeadInTime; 
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
        if ( c != KCS_LEAD_IN ) {
            /*
             *  Only ordinary characters and EOF are written
             */
            if ( c == EOF ) {
                c = 0x1A;
            }
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
 *  Send a segment with checksum 
 */
int output_segment( void *seg, int l )
{
    unsigned char *p = (unsigned char *) seg;
    int check = 0;
    int err;

    err = output( KCS_LEAD_IN );

    while ( err == 0 && l-- ) {
        /*
         *  Write each segment byte
         */
        err = output( *p );
        check += *p++;
    }
    if ( err == 0 ) {
        check = 256 - ( check & 0xff );
        err = output( check & 0xff );
        if ( err == 0 && Fp200 ) {
            err = output( check & 0xff );
        }
    }
    if ( !JisBasic ) {
        if ( err == 0 ) {
            err = output( 0xF1 );
        }
        if ( err == 0 ) {
            err = output( 0x00 );
        }
    }
    return err;
}


/*
 *  Add byte to ASCII file, flush if necessary
 */
int output_ASCII( int c, bool flush, bool eof )
{
    static int l = 0;
    int err = 0;

    Data->data[ l++ ] = (char) c;
    if ( flush || l == 256 ) {
        /*
         *  Segment is full
         */
        Data->len_lsb = (unsigned char) l;
        Data->len_msb = (unsigned char) ( l >> 8 );
        Data->eof_marker = eof ? 0xff : 0;
        err = output_segment( Data, l + 5 );
        l = 0;
    }
    return err;
}


/*
 *  Find the start of an expression
 */
char *find_expression( char *beg, char *p )
{
    int plevel;
    bool in_string = FALSE;

    if ( p <= beg ) {
        return beg;
    }
    if ( *p == ')' ) {
        /*
         *  (...)
         */
        plevel = 1;
        while ( p != beg ) {
            --p;
            if ( *p == '"' ) {
                in_string = !in_string;
                continue;
            }
            if ( in_string ) {
                continue;
            }
            if ( *p == '(' ) {
                if ( --plevel == 0 ) {
                    return p;
                }
                continue;
            }
            if ( *p == ')' ) {
                ++plevel;
                continue;
            }
            if ( *p == ':' ) {
                return p;
            }
        }
        return beg;
    }

    /*
     *  Variable or constant
     */
    while ( isalnum( *p ) && p != beg ) {
        --p;
    }
    return p == beg ? beg : p + 1;
}


/*
 *  Read a line from input file and do some preliminary syntax checks
 *  Returns the line number in binary or 0 (ignore) or EOF (error)
 */
long read_line( char *buffer, int *err )
{ 
    char temp_buffer[ 256 ];
    char *temp = temp_buffer;
    char *p, *q, *q2, *q3;
    int line_number = 0, l;
    bool in_string, transparent;
    struct _translate *tp;
    TOKEN *ep;
    bool paren = FALSE;
    
    errno = 0;
    p = fgets( temp, 256, FileIn );
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
            if ( feof( FileIn ) ) {
                /*
                 *  Last line lacks line end
                 */
                p = temp + strlen( temp );
            }
            else {
                /*
                 *  line is too long
                 */
                *err = 3;
                return EOF;
            }
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
     *  Most of this is irrelevant for data files
     */
    if ( Data->file_type != TYPE_DATA ) {
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

        /*
         *  Keyword translation needs a second pass
         *  Thus, store intermediate result in temp buffer if neccessary
         */
        q = Translate ? temp : buffer;

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
            while ( !in_string && !transparent && isspace( *p ) ) {
                if ( JisBasic && isalpha( q[ -1 ] ) && 
                     ( 0 == compare( p, " TO", 3 ) 
                    || 0 == compare( p, " THEN", 5 )
                    || 0 == compare( p, " ELSE", 5 ) )
                   )
                {
                    /*
                     *  Preserve space between alphabetic character
                     *  and some keywords
                     */
                    break;
                }
                ++p;
            }

            if ( Uppercase ) {
                /*
                 *  The FX-750P has no lowercase characters!
                 */
                *p = toupper( *p );
            }
            if ( *p == '"' ) {
                in_string = !in_string;
            }
            if ( in_string || transparent || Lowercase ) {
                continue;
            }

            /*
             *  Make uppercase
             */
            *p = toupper( *p );
        }
        *q = '\0';

        if ( Translate ) {
            /*
             *  Translate old style keywords
             */
            /* printf( "%s\n", temp ); */

            transparent = FALSE;
            in_string = FALSE;

            for ( p = temp, q = buffer; *p != '\0'; q++, p++ ) {
                /*
                 *  Copy char, may be overwritten later
                 */
                *q = *p;

                if ( *p == '"' ) {
                    in_string = !in_string;
                }
                if ( in_string || transparent ) {
                    continue;
                }
                if ( *p == '\'' 
                     || compare( p - 4, "DATA", 4 ) == 0
                     || compare( p - 3, "REM" , 3 ) == 0 )
                {
                    transparent = TRUE;
                    continue;
                }

                for ( tp = translation_table; tp->from != NULL; ++tp ) {

                    l = strlen( tp->from );
                    if ( 0 == compare( tp->from, p, l ) ) {
                        /*
                         *  replace keyword
                         */
                        p += l - 1;
                        l = strlen( tp->to );
                        memcpy( q, tp->to, l );
                        q += l - 1;

                        /*
                         *  Check if a closing parenthesis is needed
                         */
                        paren = *q == '(';
                        if ( paren && p[ 1 ] == '(' ) {
                            --q;
                            paren = FALSE;
                        }
                        break;
                    }
                }
                if ( paren && ( *p == ':' || *p == ',' || *p == ';' ) ) {
                    /*
                     *  Insert missing parenthesis before delimiter
                     */
                    paren = FALSE;
                    *q++ = ')';
                    *q = *p;
                }

                if ( *p == '!' && p != temp ) {
                    /*
                     *  Find start of expression and insert FACT before it
                     */
                    q2 = find_expression( buffer, --q );
                    /* printf( "find_expression: >%.*s<\n", q - q2 + 1, q2 ); */
                    q3 = q;
                    do {
                        q3[ 4 ] = *q3;
                    } while ( q3-- != q2 );
                    memcpy( q2, "FACT", 4 );
                    q += 4;
                }
            }
            if ( paren ) {
                /*
                 *  Insert missing parenthesis at end of line
                 */
                *q++ = ')';
            }
            *q = '\0';
        }

        if ( LogarithmMode ) {
            /*
             *  Handle LOG/LN LGT/LOG conversion
             */
            strcpy( temp, buffer );

            for ( p = temp, q = buffer; *p != '\0'; *q++ = *p++ ) {

                tp = LogarithmMode == LOG_PB ? translation_log_PB
                                             : translation_log_FX;
                for ( ; tp->from != NULL; ++tp ) {

                    l = strlen( tp->from );
                    if ( 0 == compare( tp->from, p, l ) ) {
                        /*
                         *  replace keyword
                         */
                        p += l - 1;
                        l = strlen( tp->to );
                        memcpy( q, tp->to, l );
                        q += l - 1;
                        break;
                    }
                }
            }
            *q = '\0';
        }

        if ( FileOut != stdout ) {
            printf( "%5d %s\n", line_number, buffer );
        }
    }
    else {
        /*
         *  Just use and show the untranslated buffer for data files
         */
        strcpy( buffer, temp );
        if ( FileOut != stdout ) {
            puts( buffer );
        }
    }

    /*
     *  Handle escapes for both BASIC and data files
     */
    for ( q = p = buffer; *p != '\0'; ++q, ++p ) {
        /*
         *  Copy char, may be overwritten later
         */
        *q = *p;

        if ( Utf8Mode ) {
            /*
             *  Look for UTF-8 sequence
             */
            struct _utf8 *up = Utf8Mode == UTF8_FX ? Utf8_FX : Utf8_PB;
            for ( ; up->text != NULL; ++up ) {
                l = strlen( up->text );
                if ( 0 == compare_case( p, up->text, l ) ) {
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
            ep = PbEscapes ? Escapes_PB : Escapes_FX;
            for ( ; ep->text != NULL; ++ep ) {
                l = strlen( ep->text );
                if ( 0 == compare( p, ep->text, l ) ) {
                    break;
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
                sscanf( p, "%2x", &l );
                if ( l != 0xF0 && ( l >= 0x10 || *p == '0' ) ) {
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
 *  Encode (tokenize) a program line for PB-700/FX-750P
 */
int encode700( long line_number, char *line, char *buffer )
{
    unsigned int i, l, tl;
    long ln = line_number;
    TOKEN *tp;
    bool transparent, in_string;

    /*
     *  Convert line number to BCD
     */
    l = 0;
    for ( i = 0; i < 16; i += 4 ) {
        l |= (int)( ln % 10 ) << i;
        ln /= 10;
    }
    *buffer++ = l & 0xff;
    *buffer++ = l >> 8;
    l = 2;

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

    while ( *line != '\0' ) {

        if ( *line == '"' ) {
            in_string = !in_string;
        }
        if ( !in_string && !transparent ) {

            if ( *line == ':' ) {
                /*
                 *  Statement delimiter
                 */
                *buffer++ = 0xFE;
                ++l;
                ++line;
                continue;
            }
            if ( *line == '\'' ) {
                /*
                 *  Replace ' by :REM
                 */
                if ( l > 2 && (unsigned char) buffer[ -1 ] != 0xFE ) {
                    *buffer++ = 0xFE;
                    ++l;
                }
                *buffer++ = 0xB8;
                ++l;
                ++line;
                transparent = TRUE;
                continue;
            }
            if ( *line == '&' && toupper( line[ 1 ] ) == 'H' ) {
                /*
                 *  Allow &H HEX number (convert to decimal)
                 */
                line += 2;
                ln = 0;
                while ( isxdigit( *line ) ) {
                    ln <<= 4;
                    ln |= *line++ & 0xF;
                }
                sprintf( buffer, "%ld", ln );
                i = strlen( buffer );
                buffer += i;
                l += i;
                continue;
            }
            if ( isalpha( *line ) ) {
                /*
                 *  Look if a token is matched
                 */
                for ( tp = Tokens700; tp->text != NULL; ++tp ) {
                    tl = strlen( tp->text );
                    if ( 0 == compare( line, tp->text, tl ) ) {
                        break;
                    }
                }
                if ( tp->text != NULL ) {
                    /*
                     *  Replace string by token value
                     */
                    if ( tp->prefix & 0x40 ) {
                        /*
                         *  DATA or REM
                         */
                        transparent = TRUE;
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
    *buffer = 0xFF;
    return l + 1;
}


/*
 *  Encode (tokenize) a program line for FX-850P/PB-1000/FP-200
 */
int encode850_200( long line_number, char *line, char *buffer )
{
    unsigned int i, l, tl;
    long ln = line_number;
    TOKEN *tp;
    bool transparent, in_string, binary;

    /*
     *  Length byte and line number in binary
     */
    ++buffer;
    *buffer++ = ln & 0xff;
    *buffer++ = ln >> 8;
    l = 3;

    /*
     *  Skip line number in input line
     */
    while ( isdigit( *line ) ) {
        ++line;
    }
    if ( *line != ' ' ) {
        /*
         *  Add a mandatory space after line number
         */
        *buffer++ = ' ';
        ++l; 
    }

    /*
     *  Encode the rest
     */
    transparent = FALSE;
    in_string = FALSE;
    binary = FALSE;

    while ( *line != '\0' ) {

        if ( *line == '"' ) {
            in_string = !in_string;
        }
        if ( !in_string && !transparent ) {

            if ( binary && 
                 !isdigit( *line ) && *line != ',' && *line != ' ' )
            {
                /*
                 *  Something other then list of line numbers
                 */
                binary = FALSE;
            }
            if ( *line == ':' ) {
                /*
                 *  Statement delimiter
                 */
                *buffer++ = 0x01;
                ++l;
                ++line;
                continue;
            }
            if ( *line == '\'' ) {
                /*
                 *  End of line comment
                 */
                *buffer++ = 0x02;
                ++l;
                ++line;
                transparent = TRUE;
                continue;
            }
            if ( *line == '&' && toupper( line[ 1 ] ) == 'H' ) {
                /*
                 *  Avoid accidental encoding of HEX numbers
                 */
                *buffer++ = '&';
                *buffer++ = 'H';
                l += 2;
                line += 2;
                while ( isxdigit( *line ) ) {
                    *buffer++ = *line++;
                    ++l;
                }
                continue;
            }
            if ( binary && isdigit( *line ) ) {
                /*
                 *  Store line numbers in binary
                 */
                ln = 0;
                while ( isdigit( *line ) ) {
                    ln = 10 * ln + ( *line++ & 0x0F );
                }
                *buffer++ = 0x03;
                *buffer++ = (unsigned char) ( ln & 0xFF );
                *buffer++ = (unsigned char) ( ln >> 8 );
                l += 3;
                continue;
            }
            if ( isalpha( *line ) ) {
                /*
                 *  Look if a token is matched
                 */
                tp = Fp200 ? Tokens200 : Tokens850;
                for ( ; tp->text != NULL; ++tp ) {
                    tl = strlen( tp->text );
                    if ( 0 == compare( line, tp->text, tl ) ) {
                        break;
                    }
                }
                if ( tp->text != NULL ) {
                    /*
                     *  Replace string by prefix and token value
                     */
                    if ( tp->prefix & 0x20 ) {
                        /*
                         *  Delimiter before ELSE
                         */
                        *buffer++ = 0x01;
                        ++l;
                    }
                    if ( tp->prefix & 0x40 ) {
                        /*
                         *  DATA or REM
                         */
                        transparent = TRUE;
                    }
                    if ( tp->prefix & 0x80 ) {
                        /*
                         *  Encode line numbers
                         */
                        binary = TRUE;
                    }
                    line += tl;
                    *buffer++ = tp->prefix & 0x0F;
                    *buffer++ = tp->token;
                    l += 2;
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
     *  Length and end of line delimiter
     */
    *buffer = 0x00;
    buffer -= l;
    *buffer = (unsigned char) l;
    return l + 1;
}


/*
 *  Main program
 */
int main( int argc, char *argv[] )
{
    int err = 0;
    int baudrate = 300;   /* default is slow */
    int i, l, linecount;
    long line_number, file_number = 0;
    char *p, *buff, c;
    char ext[ 4 ];

    ++argv;
    --argc;

    Header.file_type = TYPE_ASCII;

    while ( argc > 0 && **argv == '-' ) {
        /*
         *  Options
         */
        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            OutputMode = MODE_BIN;
        }
        if ( strncmp( *argv, "-a", 2 ) == 0 ) {
            OutputMode = MODE_ASCII;
        }
        if ( strncmp( *argv, "-s", 2 ) == 0 
          || strncmp( *argv, "-w", 2 ) == 0 ) 
        {
            OutputMode = MODE_WAVE;
            baudrate = 300;
        }
        if ( strncmp( *argv, "-f", 2 ) == 0 ) {
            OutputMode = MODE_WAVE;
            baudrate = 1200;
        }
        if ( strncmp( *argv, "-2", 2 ) == 0 ) {
            Fp200 = TRUE;
            AsciiLargeBlocks = TRUE;
        }
        if ( strncmp( *argv, "-t", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'B':
                AsciiLargeBlocks = TRUE;
            case 'A':
                Header.file_type = TYPE_ASCII;
                break;
            case '7':
                Header.file_type = TYPE_PROGRAM;
                PbEscapes = TRUE;
                break;
            case '8':
                Header.file_type = TYPE_PROG850;
                break;
            case '2':
                Header.file_type = TYPE_PROG850;
                Fp200 = TRUE;
                break;
            case 'T':
            case '\0':
                Header.file_type = TYPE_TEXT;
                OutputMode = MODE_TEXT;
                break;
            }
        }
        if ( strncmp( *argv, "-d", 2 ) == 0 ) {
            Header.file_type = TYPE_DATA;
            LeadInTime = atoi( *argv + 2 );
            if ( LeadInTime <= 0 ) {
                LeadInTime = 0;
            }
            p = strchr( *argv + 2, ',' );
            if ( p != NULL ) {
                DataLines = atoi( p + 1 );
                if ( DataLines == 0 && p[ 1 ] == 'B' ) {
                    AsciiLargeBlocks = TRUE;
                }
            }
        }
        if ( strncmp( *argv, "-u", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'F':
                Utf8Mode = UTF8_FX; 
                break;
            case 'P':
                Utf8Mode = UTF8_PB; 
                break;
            case '\0':
                Uppercase = TRUE;
                Lowercase = FALSE;
                break;
            }
        }
        if ( strncmp( *argv, "-l", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'F':
                LogarithmMode = LOG_FX; 
                break;
            case 'P':
                LogarithmMode = LOG_PB; 
                break;
            case '\0':
                Uppercase = FALSE;
                Lowercase = TRUE;
                break;
            }
        }
        if ( strncmp( *argv, "-e", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'F':
                PbEscapes = FALSE; 
                break;
            case 'P':
                PbEscapes = TRUE; 
                break;
            }
        }
        if ( strncmp( *argv, "-o", 2 ) == 0 ) {
            Translate = TRUE;
        }
        ++argv;
        --argc;
    }
    if ( Lowercase || baudrate != 300 || OutputMode == MODE_TEXT
         || AsciiLargeBlocks ) 
    {
        JisBasic = TRUE;
    }
    if ( Fp200 ) {
        baudrate = 300;
    }
    if ( LeadInTime == 0 ) {
        LeadInTime = Fp200 || AsciiLargeBlocks     ? LEAD_IN_TIME_FP200 
                   : Header.file_type == TYPE_DATA ? LEAD_IN_TIME_DATA
                                                   : LEAD_IN_TIME;
    }

    if ( argc < 1 ) {
        printf(
         "usage: bas850 <options> infile outfile\n"
         "         -a create an ASCII encoded file for Piotr's interface\n"
         "         -b create a binary file\n"
         "         -w or -s create a 300 baud WAV file "
                   "(slow: PB-700, FX-750P)\n"
         "         -f create a 1200 baud WAV file "
                      "(fast: FX-850P, PB-1000)\n"
         "         -2 create FP-200 compatible file\n"
         "         -t[T|A|B|2|7|8] select type of output\n"
         "           T: plain text for download via serial or USB interface\n"
         "           A: SAVE,A output; load with LOAD,A on PB-700/FX-750P/FX-850P\n"
         "           B: same as -A but creates large blocks: FP-200/FX-850P only\n"
         "           2: internal format for FP-200\n"
         "           7: internal format for PB-700 family\n" 
         "           8: internal format for FX-850P/PB-1000 family\n"
         "         -d<delay>,<count> process data file instead of BASIC code\n"
         "           <delay> adjusts the time between data lines\n"
         "           Start with 20 (default) and increase in case of difficulty\n"
         "           <count> inserts a new file header each <count> data lines\n"
         "           This must equal the number of items read by a PB-700 GET statement\n"
         "         -e[F|P] backslash escape syntax used\n"
         "           F: FX-850P/VX/Z extended character set\n"
         "           P: PB-700/PB-1000/FP-200 graphics character set\n"
         "         -l[F|P] handling of LOG/LN versus LGT/LOG\n"
         "           F: convert to FX-850P/VX/Z logarithm syntax LOG and LN\n"
         "              use with -t8 to translate PB-1000 programs\n" 
         "           P: convert to PB-700/PB-1000/FP-200 logarithm syntax LGT and LOG\n"
         "         -l allow lowercase keywords and variables "
                      "for FX-850P/PB-1000/FP-200\n"
         "         -u[F|P] enable Unicode (UTF-8) input\n"
         "           F: FX-850P/VX/Z extended character set\n"
         "           P: PB-700/PB-1000/FP-200 graphics character set\n"
         "         -u make everything uppercase for FX-750P\n"
         "         -o replace old style keywords like PRT, VAC or CSR\n" 
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
    if ( argc < 1 && OutputMode != MODE_TEXT ) {
        fprintf( stderr, "Missing output file name\n" );
        return 2;
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

    case MODE_ASCII:
        /*
         *  Open output file in text mode
         */
        FileOut = fopen( *argv, "wt" );
        if ( NULL == FileOut ) {
            fprintf( stderr, "Cannot open ASCII output file %s\n", *argv );
            perror( *argv );
            return 2;
        }
        printf( "Creating ASCII file %s\n", *argv );
        break;

    case MODE_WAVE:
        /*
         *  Open the output WAV file
         */
        WaveOut = kcsOpen( *argv, "wb", baudrate, 8, 'E', 2 );
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
        printf( "Creating wave file %s at %d baud\n", *argv, baudrate );
        break;

    case MODE_TEXT:
        /*
         *  Open output file in binary mode for plain text output
         *  Except for piping
         */
        if ( argc < 1 ) {
            FileOut = stdout;
        }
        else {
            FileOut = fopen( *argv, "wb" );
            if ( NULL == FileOut ) {
                fprintf( stderr, "Cannot open text output file %s\n", *argv );
                perror( *argv );
                return 2;
            }
            printf( "Creating text file %s\n", *argv );
        }
        break;
    }

    switch ( Header.file_type ) {

    case TYPE_ASCII:
        if ( AsciiLargeBlocks ) {
            printf( "File type is ASCII with large blocks for FP-200/FX-850P\n" );
        }
        else {
            printf( "File type is ASCII, load with LOAD,A\n" );
        }
        break;

    case TYPE_DATA:
        if ( AsciiLargeBlocks ) {
            printf( "File type is DATA with large blocks for FP-200/FX-850P\n" );
        }
        else {
            if ( DataLines > 0 ) {
                printf( "File type is DATA with %d items per tape file\n",
                        DataLines );
            }
            else {
                printf( "File type is DATA\n" );
            }
        }
        break;

    case TYPE_PROGRAM:
        printf( "File type is program for PB-700/FX-750\n" );
        break;

    case TYPE_PROG850:
        if ( Fp200 ) {
            printf( "File type is program for FP-200\n" );
        }
        else {
            printf( "File type is program for FX-850/PB-1000\n" );
        }
        break;

    case TYPE_TEXT:
        if ( FileOut != stdout ) {
            printf( "File type is plain text for serial or USB interface\n" );
        }
        break;
    }

    /*
     *  Create file header
     */
    Header.segment_id = 'H';

    if ( OutputMode != MODE_TEXT ) {
        /*
         *  Filename = basename of output file + "TXT", "BAS" or "DAT" extension
         */
        memset( Header.file_name, ' ', 8 + 3 );
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
        while ( i < 8 && *p != '\0' && *p != '.' ) {
            Header.file_name[ i ] = Uppercase ? toupper( *p ) : *p;
            ++p;
            ++i;
        }

        if ( Header.file_type == TYPE_ASCII ) {
            memcpy( Header.file_ext, "TXT", 3 );
        }
        else if ( Header.file_type == TYPE_DATA ) {
            memcpy( Header.file_ext, "DAT", 3 );
        }
        else {
            memcpy( Header.file_ext, "BAS", 3 );
        }

        memset( Header.password, 0xFF, 8 );
        memset( Header.parameters, 0, 12 );
    }

    /*
     *  Allocate Data area
     */
    Data = malloc( sizeof( struct _data ) );
    if ( Data == NULL ) {
        perror( "Allocate memory for data area" );
        return 3;
    }

    /*
     *  Process file
     */
    linecount = 0;
    
    switch ( Header.file_type ) {

    case TYPE_ASCII:
    case TYPE_DATA:
        /*
         *  A standard ASCII file consists of one segment per line of code.
         *  FP-200/FX-850P allow blocks of 256 bytes in an ASCII file.
         */
        Data->segment_id = 'D';
        Data->file_type = Header.file_type;
        Data->eof_marker = 0;
        line_number = 0;

        /*
         *  Process the input file
         */
        while ( err == 0 ) {
            /*
             *  read the text input file line by line
             */
            l = line_number;
            p = Data->data + 256;
            line_number = read_line( p, &err );
            if ( line_number == 0 && Data->file_type == TYPE_ASCII ) {
                /*
                 *  Line has no line number - ignore
                 */
                continue;
            }
            if ( linecount == 0 ) {
                /*
                 *  Dump header to WAV file
                 */
                if ( DataLines > 0 ) {
                    file_number += 1;
                    sprintf( ext, "%03.3d", file_number ); 
                    memcpy( Header.file_ext, ext, 3 );
                }
                err = output_segment( &Header, HEADER_SIZE );
            }            
            if ( line_number == EOF 
                 || ( Data->file_type == TYPE_ASCII && line_number <= l ) ) {
                /*
                 *  EOF or error or lines out of order
                 */
                if ( err != 0 ) {
                    break;
                }
                if ( Fp200 ) {
                    err = output_ASCII( 0x0D, FALSE, FALSE );
                    if ( err == 0 ) {
                        err = output_ASCII( 0x0A, TRUE, TRUE );
                    }
                }
                else {
                    err = output_ASCII( 0x0D, TRUE, TRUE );
                }
            }
            else {
                /*
                 *  Output the line
                 */
                while ( *p != '\0' && err == 0 ) {
                    err = output_ASCII( *p++, FALSE, FALSE );
                }
                if ( err == 0 ) {
                    /*
                     *  Terminate line, flush if not creating large blocks
                     */
                    err = output_ASCII( 0x0D, !AsciiLargeBlocks, FALSE );
                    if ( err == 0 && Fp200 ) {
                        /*
                         *  FP-200 terminates lines with CR+LF
                         */
                        err = output_ASCII( 0x0A, FALSE, FALSE );
                    }
                }
                ++linecount;
                if ( DataLines > 0 && linecount == DataLines ) {
                    /*
                     *  Force a new header with next line
                     */
                    linecount = 0;
                }
            }
            if ( err != 0 || Data->eof_marker != 0 ) {
                /*
                 *  Error or last segment
                 */
                break;
            }
        }
        break;

    case TYPE_PROGRAM:
        /*
         *  A program consists of the header and a single data segment
         *  The data segment has no subtype, eof marker length or checksum
         *  The size of the program is coded into the header parameters
         */
        buff = Data->data + 256;
        *buff++ = 'D';

        /*
         *  Process the input file
         */
        while ( err == 0 ) {
            /*
             *  read the text input file line by line
             */
            line_number = read_line( Data->data, &err );
            if ( line_number == EOF ) {
                /*
                 *  EOF or error
                 */
                break;
            }
            if ( line_number == 0 ) {
                /*
                 *  Line has no line number - ignore
                 */
                continue;
            }
            if ( line_number > 9999 ) {
                /*
                 *  too large
                 */
                err = 4;
                break;
            }
            ++linecount;
        
            /*
             *  encode the line
             */
            l = encode700( line_number, Data->data, buff );
            buff += l;
        }
        if ( err == 0 ) {
            /*
             *  Delimiter
             */
            *buff++ = 0xF0;

            /*
             *  Set length in header and dump it to file
             */
            p = Data->data + 256;
            l = buff - p - 2;
            Header.parameters[ 6 ] = l & 0xff;
            Header.parameters[ 7 ] = l >> 8;
            err = output_segment( &Header, HEADER_SIZE );

            if ( err == 0 ) {
                /*
                 *  Dump the program after a lead-in
                 */
                err = output( KCS_LEAD_IN );
                l += 2;
                while ( err == 0 && l-- ) {
                    err = output( *p++ & 0xFF );
                }
            }
        }
        break;

    case TYPE_PROG850:
        /*
         *  An FX-850/PB-1000/FP-200 program is similar to a PB-700 program
         *  The data segment has subtype TYPE_PROG850 and regular checksum
         *  The size of the program is coded into the header parameters
         */
        buff = Data->data + 256;
        *buff++ = 'D';
        *buff++ = TYPE_PROG850;
        *buff++ = 0xFF;

        /*
         *  Process the input file
         */
        while ( err == 0 ) {
            /*
             *  read the text input file line by line
             */
            line_number = read_line( Data->data, &err );
            if ( line_number == EOF ) {
                /*
                 *  EOF or error
                 */
                break;
            }
            if ( line_number == 0 ) {
                /*
                 *  Line has no line number - ignore
                 */
                continue;
            }
            if ( line_number > 65535 ) {
                /*
                 *  too large
                 */
                err = 4;
                break;
            }
            ++linecount;
        
            /*
             *  encode the line
             */
            l = encode850_200( line_number, Data->data, buff );
            buff += l;
        }
        if ( err == 0 ) {
            /*
             *  Set length in header and dump it to file
             */
            p = Data->data + 256;
            l = buff - p - 3;
            Header.parameters[ 6 ] = l & 0xff;
            Header.parameters[ 7 ] = l >> 8;
            err = output_segment( &Header, HEADER_SIZE );

            if ( err == 0 ) {
                /*
                 *  Dump the program segment
                 */
                err = output_segment( p, l + 3 );
            }
        }
        break;

    case TYPE_TEXT:
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
            line_number = read_line( Data->data, &err );
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
             *  Terminate line with CR+LF and send it to file
             */
            p = Data->data;
            l = strlen( p );
            if ( FileOut != stdout ) {
                p[ l++ ] = 0x0D;
            }
            p[ l++ ] = 0x0A;
            while ( err == 0 && l-- ) {
                /*
                 *  Write each byte of the line
                 */
                err = output( *p++ & 0xFF );
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
    if ( err == 0 && linecount == 0 && Header.file_type != TYPE_DATA ) {
        /*
         *  No valid lines found
         */
        err = 1;
    }

    if ( err == 2 ) {
        perror( *argv );
    }
    else if ( err != 0 ) {
        fprintf( stderr, "Error encountered: %s\n", err_msg[ err ] );
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
