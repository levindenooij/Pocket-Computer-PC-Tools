/*
 *  list850.c - List Casio tape files
 *
 *  This program displays the contents of a tape image saved by the PB-700, 
 *  FX-750P, FX-850P or PB-1000 calculators through the sound card or with
 *  Piotr's serial interface.
 *
 *  Options:
 *    -b<skip>  read BIN file (Default); skip <skip> garbage bytes
 *    -a        read ASCII file (Piotr's format)
 *    -w[S|F|H] read WAV file, Slow (default) or Fast (FX-850P only)
 *                             High speed (PB-1000)
 *
 *  Inspired by work from Piotr Piatek
 *  Written by Marcus von Cube
 */
#ifndef DEBUG
#define DEBUG 0
#endif

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
    "Leader expected",                        /* message #1 */
    "Premature end of a header/data segment", /* message #2 */
    "Premature end of a file",                /* message #3 */
    "Header segment expected",                /* message #4 */
    "Unknown segment identifier",             /* message #5 */
    "Unexpected program separator",           /* message #6 */
    "Invalid subtype in data segment",        /* message #7 */
    "Missing line terminator",                /* message #8 */
    "Program table is last segment",          /* message #9 */
    "Program table is missing",               /* message #10 */
    "Program table doesn't have 10 entries",  /* message #11 */
    "Missing program separator",              /* message #12 */
};

/*
 *  PB-700 / FX-750 keywords
 *  Single byte tokens from 0x80 to 0xEC
 */
char *tokens[] =
{
    "SIN ",     "COS ",     "TAN ",     "EXP ",     /* 80 */
    "ASN ",     "ACS ",     "ATN ",     "LOG ",     /* 84 */
    "LGT ",     "SQR ",     "ABS ",     "FRAC ",    /* 88 */
    "INT ",     "SGN ",     "POINT(",   "ROUND(",   /* 8C */
    "ASC(",     "LEN(",     "VAL(",     "PI",       /* 90 */
    "RND",      "DEG(",     "HYP",      "HEX$(",    /* 94 */
    "DMS$(",    "INKEY$",   "CHR$(",    "STR$(",    /* 98 */
    "LEFT$(",   "RIGHT$(",  "MID$(",    " MOD ",    /* 9C */
    " GOTO ",   " GOSUB ",  "RETURN",   "FOR ",     /* A0 */
    "NEXT ",    "IF ",      "STOP",     "INPUT",    /* A4 */
    "READ ",    "RESTORE ", "END",      "DRAW(",    /* A8 */
    "DRAWC(",   "LOCATE ",  "PRINT ",   "LPRINT ",  /* AC */
    " ELSE ",   " STEP ",   " THEN ",   " TO ",     /* B0 */
    " USING ",  "TAB(",     "ALL",      "DATA ",    /* B4 */
    "REM ",     "LET",      "ANGLE ",   "BEEP ",    /* B8 */
    "DIM ",     "ERASE ",   "TRON",     "TROFF",    /* BC */
    "CLEAR",    "PROG ",    "PUT ",     "GET ",     /* C0 */
    "VERIFY ",  "CHAIN ",   "SAVE ",    "LOAD ",    /* C4 */
    "PASS ",    "NEW ",     "LIST ",    "LLIST ",   /* C8 */
    "RUN ",     "DELETE ",  "EDIT ",    "CONT",     /* CC */
    "SYSTEM",   "CLS",      "STAT ",    "ROUND",    /* D0 */
    "SRAM",     "LRAM",     "WAIT",     "ON ",      /* D4 */
    "OFF",      "AUTO",     "CNT",      "SUMY",     /* D8 */
    "SUMX",     "SUMXY",    "SUMX2",    "SUMY2",    /* DC */
    "MEANX",    "MEANY",    "SDX",      "SDY",      /* E0 */
    "SDXN",     "SDYN",     "LRA",      "LRB",      /* E4 */
    "COR",      "EOX",      "EOY",      "PEEK ",    /* E8 */
    "POKE "                                         /* EC */
};

/*
 *  FX-850 keywords (work for PB-1000 as well)
 *  Double byte tokens from 0x40 to 0xCF with prefixes 4 to 7
 */
char *tokens850[ 4 ][ 0xd0 - 0x40 ] = 
{ 
    { /* Prefix 4 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */  
        NULL,       NULL,       NULL,       NULL,       /* 44 */  
        NULL,       " GOTO ",   " GOSUB ",  "RETURN",   /* 48 */  
        "RESUME ",  "RESTORE ", "WRITE#",   NULL,       /* 4C */  
        NULL,       NULL,       "SYSTEM",   "PASS ",    /* 50 */  
        NULL,       "DELETE ",  "BSAVE ",   "LIST ",    /* 54 */  
        "LLIST ",   "LOAD ",    "MERGE ",   NULL,       /* 58 */  
        NULL,       "TRON",     NULL,       "TROFF",    /* 5C */  
        "VERIFY ",  "MON",      "CALL ",    "POKE ",    /* 60 */  
        NULL,       NULL,       NULL,       NULL,       /* 64 */  
        NULL,       "CHAIN ",   "CLEAR ",   "NEW ",     /* 68 */  
        "SAVE ",    "RUN ",     "ANGLE ",   "EDIT ",    /* 6C */  
        "BEEP ",    "CLS",      "CLOSE ",   NULL,       /* 70 */  
        NULL,       NULL,       "DEF ",     "DEFM ",    /* 74 */  
        "DEFSEG ",  NULL,       "VAC",      NULL,       /* 78 */  
        "DIM ",     "DRAW ",    NULL,       NULL,       /* 7C */  
        "DATA ",    "FOR ",     "NEXT ",    NULL,       /* 80 */  
        NULL,       "ERASE ",   "ERROR ",   "END",      /* 84 */  
        NULL,       NULL,       "FIELD ",   "FORMAT",   /* 88 */  
        "GET ",     "IF ",      NULL,       "LET ",     /* 8C */  
        "LINE ",    "LOCATE ",  NULL,       "LSET ",    /* 90 */  
        NULL,       NULL,       NULL,       "OPEN ",    /* 94 */  
        NULL,       "OUT ",     " ON ",     NULL,       /* 98 */  
        NULL,       NULL,       NULL,       "CALCJMP ", /* 9C */  
        "BLOAD ",   NULL,       "DRAWC ",   "PRINT ",   /* A0 */  
        "LPRINT ",  "PUT ",     NULL,       NULL,       /* A4 */  
        "READ ",    "REM ",     "RSET ",    NULL,       /* A8 */  
        "SET ",     "STAT",     "STOP",     NULL,       /* AC */  
        "MODE ",    NULL,       "VAR ",     "PBLOAD ",  /* B0 */  
        "PBGET ",   NULL,       NULL,       NULL,       /* B4 */  
        NULL,       NULL,       NULL,       NULL,       /* B8 */  
        NULL,       NULL,       NULL,       NULL,       /* BC */  
        NULL,       NULL,       NULL,       NULL,       /* C0 */  
        NULL,       NULL,       NULL,       NULL,       /* C4 */  
        NULL,       NULL,       NULL,       NULL,       /* C8 */  
        NULL,       NULL,       NULL,       NULL        /* CC */  
    },
    { /* prefix 5 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */
        NULL,       NULL,       NULL,       NULL,       /* 44 */
        NULL,       NULL,       NULL,       NULL,       /* 48 */
        NULL,       NULL,       NULL,       "ERL",      /* 4C */
        "ERR",      "CNT",      "SUMX",     "SUMY",     /* 50 */
        "SUMX2",    "SUMY2",    "SUMXY",    "MEANX",    /* 54 */
        "MEANY",    "SDX",      "SDY",      "SDXN",     /* 58 */
        "SDYN",     "LRA",      "LRB",      "COR",      /* 5C */
        "PI",       NULL,       NULL,       "CUR ",     /* 60 */
        NULL,       NULL,       NULL,       "FACT ",    /* 64 */
        NULL,       "EOX ",     "EOY ",     "SIN ",     /* 68 */
        "COS ",     "TAN ",     "ASN ",     "ACS ",     /* 6C */
        "ATN ",     "HYPSIN ",  "HYPCOS ",  "HYPTAN ",  /* 70 */
        "HYPASN ",  "HYPACS ",  "HYPATN ",  "LN ",      /* 74 */
        "LOG ",     "EXP ",     "SQR ",     "ABS ",     /* 78 */
        "SGN ",     "INT ",     "FIX ",     "FRAC ",    /* 7C */
        "RND ",     NULL,       NULL,       NULL,       /* 80 */
        NULL,       NULL,       "PEEK ",    NULL,       /* 84 */
        NULL,       "LOF ",     "EOF ",     NULL,       /* 88 */
        NULL,       "FRE ",     NULL,       "POINT ",   /* 8C */
        "ROUND",    "RND",      "VALF",     "RAN#",     /* 90 */
        "ASC",      "LEN",      "VAL",      NULL,       /* 94 */
        NULL,       NULL,       NULL,       NULL,       /* 98 */
        "DEG",      NULL,       NULL,       NULL,       /* 9C */
        NULL,       NULL,       NULL,       NULL,       /* A0 */
        NULL,       NULL,       NULL,       "REC",      /* A4 */
        "POL",      NULL,       "NPR",      "NCR",      /* A8 */
        "HYP",      NULL,       NULL,       NULL,       /* AC */
        NULL,       NULL,       NULL,       NULL,       /* B0 */
        NULL,       NULL,       NULL,       NULL,       /* B4 */
        NULL,       NULL,       NULL,       NULL,       /* B8 */
        NULL,       NULL,       NULL,       NULL,       /* BC */
        NULL,       NULL,       NULL,       NULL,       /* C0 */
        NULL,       NULL,       NULL,       NULL,       /* C4 */
        NULL,       NULL,       NULL,       NULL,       /* C8 */
        NULL,       NULL,       NULL,       NULL        /* CC */
    },
    { /* prefix 6 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */
        NULL,       NULL,       NULL,       NULL,       /* 44 */
        NULL,       NULL,       NULL,       NULL,       /* 48 */
        NULL,       NULL,       NULL,       NULL,       /* 4C */
        NULL,       NULL,       NULL,       NULL,       /* 50 */
        NULL,       NULL,       NULL,       NULL,       /* 54 */
        NULL,       NULL,       NULL,       NULL,       /* 58 */
        NULL,       NULL,       NULL,       NULL,       /* 5C */
        NULL,       NULL,       NULL,       NULL,       /* 60 */
        NULL,       NULL,       NULL,       NULL,       /* 64 */
        NULL,       NULL,       NULL,       NULL,       /* 68 */
        NULL,       NULL,       NULL,       NULL,       /* 6C */
        NULL,       NULL,       NULL,       NULL,       /* 70 */
        NULL,       NULL,       NULL,       NULL,       /* 74 */
        NULL,       NULL,       NULL,       NULL,       /* 78 */
        NULL,       NULL,       NULL,       NULL,       /* 7C */
        NULL,       NULL,       NULL,       NULL,       /* 80 */
        NULL,       NULL,       NULL,       NULL,       /* 84 */
        NULL,       NULL,       NULL,       NULL,       /* 88 */
        NULL,       NULL,       NULL,       NULL,       /* 8C */
        NULL,       NULL,       NULL,       NULL,       /* 90 */
        NULL,       NULL,       NULL,       "DMS$",     /* 94 */
        NULL,       NULL,       "MID",      "INPUT ",   /* 98 */
        "MID$",     "RIGHT$",   "LEFT$",    NULL,       /* 9C */
        "CHR$",     "STR$",     NULL,       "HEX$",     /* A0 */
        NULL,       NULL,       NULL,       NULL,       /* A4 */
        "INKEY$",   "KEY",      NULL,       "DATE$",    /* A8 */
        "TIME$",    "CALC$",    NULL,       NULL,       /* AC */
        NULL,       NULL,       NULL,       NULL,       /* B0 */
        NULL,       NULL,       NULL,       NULL,       /* B4 */
        NULL,       NULL,       NULL,       NULL,       /* B8 */
        NULL,       NULL,       NULL,       NULL,       /* BC */
        NULL,       NULL,       NULL,       NULL,       /* C0 */
        NULL,       NULL,       NULL,       NULL,       /* C4 */
        NULL,       NULL,       NULL,       NULL,       /* C8 */
        NULL,       NULL,       NULL,       NULL        /* CC */
    },
    { /* prefix 7 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */
        NULL,       NULL,       NULL,       " THEN ",   /* 44 */
        " ELSE ",   NULL,       NULL,       NULL,       /* 48 */
        NULL,       NULL,       NULL,       NULL,       /* 4C */
        NULL,       NULL,       NULL,       NULL,       /* 50 */
        NULL,       NULL,       NULL,       NULL,       /* 54 */
        NULL,       NULL,       NULL,       NULL,       /* 58 */
        NULL,       NULL,       NULL,       NULL,       /* 5C */
        NULL,       NULL,       NULL,       NULL,       /* 60 */
        NULL,       NULL,       NULL,       NULL,       /* 64 */
        NULL,       NULL,       NULL,       NULL,       /* 68 */
        NULL,       NULL,       NULL,       NULL,       /* 6C */
        NULL,       NULL,       NULL,       NULL,       /* 70 */
        NULL,       NULL,       NULL,       NULL,       /* 74 */
        NULL,       NULL,       NULL,       NULL,       /* 78 */
        NULL,       NULL,       NULL,       NULL,       /* 7C */
        NULL,       NULL,       NULL,       NULL,       /* 80 */
        NULL,       NULL,       NULL,       NULL,       /* 84 */
        NULL,       NULL,       NULL,       NULL,       /* 88 */
        NULL,       NULL,       NULL,       NULL,       /* 8C */
        NULL,       NULL,       NULL,       NULL,       /* 90 */
        NULL,       NULL,       NULL,       NULL,       /* 94 */
        NULL,       NULL,       NULL,       NULL,       /* 98 */
        NULL,       NULL,       NULL,       NULL,       /* 9C */
        NULL,       NULL,       NULL,       NULL,       /* A0 */
        NULL,       NULL,       NULL,       NULL,       /* A4 */
        NULL,       NULL,       NULL,       NULL,       /* A8 */
        NULL,       NULL,       NULL,       NULL,       /* AC */
        NULL,       NULL,       NULL,       NULL,       /* B0 */
        NULL,       NULL,       "TAB ",     NULL,       /* B4 */
        "CSR ",     "REV ",     "NORM ",    "ALL ",     /* B8 */
        " AS ",     "APPEND ",  NULL,       "OFF",      /* BC */
        " STEP ",   " TO ",     "USING ",   "NOT ",     /* C0 */
        " AND ",    " OR ",     " XOR ",    " MOD ",    /* C4 */
        NULL,       NULL,       NULL,       NULL,       /* C8 */
        NULL,       NULL,       NULL,       NULL        /* CC */
    }
};

/*
 *  FP-200 keywords
 *  Double byte tokens from 0x25 to 0xB1 with prefix 4
 */
char *tokens200[] = 
{ 
                 "GOTO ",     "GOSUB ",    " THEN ",   /* 25 */  
    " ELSE ",    "RESTORE ",  "RUN ",      "EDIT ",    /* 28 */  
    "LIST ",     "LLIST ",    "RENUM ",    "PROG ",    /* 2C */  
    "NEW ",      "STAT ",     "DIM ",      "IF ",      /* 30 */  
    "ON ",       "RETURN",    "OPTION ",   "TRON",     /* 34 */  
    "TROFF",     "DEFSNG ",   "DEFDBL ",   "DEFSTR ",  /* 38 */  
    "DEF",       NULL,        NULL,        "REM ",     /* 3C */  
    "DATA ",     "READ ",     "END",       "STOP",     /* 40 */  
    "FOR ",      "NEXT ",     "INPUT ",    "LOCATE ",  /* 44 */  
    "CLS",       "INIT",      "DRAW",      "QUAD",     /* 48 */  
    "POKE ",     "CALL ",     "SAVE",      "LOAD",     /* 4C */  
    "LSET ",     "RSET ",     "FIELD",     "PUT",      /* 50 */  
    "GET",       "OPEN ",     "CLOSE",     "SYSTEM",   /* 54 */  
    "CLEAR ",    "MOUNT ",    "PRINT ",    "LPRINT ",  /* 58 */  
    "PASS ",     "KEY ",      "RANDOMIZE", "LET ",     /* 5C */  
    "ANGLE ",    "FILES ",    "FORMAT ",   "KILL ",    /* 60 */  
    "VERIFY ",   "AREA ",     "RESET ",    "FILE ",    /* 64 */  
    "ALL",       " AS ",      "USING ",    "BASE ",    /* 68 */  
    "OUTPUT",    " STEP ",    " TO ",      " OR ",     /* 6C */  
    " AND ",     " XOR ",     "NOT ",      " MOD ",    /* 70 */  
    "CNT",       "SUMX",      "SUMY",      "SUMX2",    /* 74 */  
    "SUMY2",     "SUMXY",     "MEANX",     "MEANY",    /* 78 */  
    "SDX",       "SDY",       "SDXN",      "SDYN",     /* 7C */  
    "LRA",       "LRB",       "FN",        "FRE",      /* 80 */  
    "TAB",       "ROUND",     "POINT",     "FL",       /* 84 */  
    "RC",        "IT",        "SUMRC",     "SUMIT",    /* 88 */  
    "SIN",       "COS",       "TAN",       "ASN",      /* 8C */  
    "ACS",       "ATN",       "LOG ",      "LGT ",     /* 90 */  
    "EXP",       "SQR",       "ABS",       "SGN",      /* 94 */  
    "INT",       "FIX",       "FRAC",      "RND",      /* 98 */  
    "CSNG",      "CDBL",      "PEEK",      "LOC",      /* 9C */  
    "LOF",       "EOF",       "LEN",       "ASC",      /* A0 */  
    "VAL",       "CVD",       "CVS",       "CHR$",     /* A4 */  
    "STR$",      "MKS$",      "MKD$",      "MID$",     /* A8 */  
    "RIGHT$",    "LEFT$",     "INKEY$",    "TIME$",    /* AC */  
    "DATE$"                                            /* B0 */
};

/*
 *  Escape codes with \
 *  FX-850P variant
 */
struct _escapes {
    char *text;
    unsigned char token;
} Escapes_FX[] = 
{
    { "\\", 0x5C },  /* backslash */
    { "YN", 0x5C },  /* Yen */
    { "AN", 0x80 },  /* Angstrom */
    { "IN", 0x81 },  /* integral */
    { "RT", 0x82 },  /* root */
    { "'",  0x83 },  /* closing quote */
    { "SM", 0x84 },  /* Sigma, sum */
    { "OM", 0x85 },  /* Omega */
    { "]",  0x86 },  /* grey block */
    { "#",  0x87 },  /* block */
    { "AL", 0x88 },  /* alpha */
    { "BT", 0x89 },  /* beta */
    { "GA", 0x8A },  /* gamma */
    { "EP", 0x8B },  /* epsilon */
    { "TH", 0x8C },  /* theta */
    { "MU", 0x8D },  /* mu */
    { "SI", 0x8E },  /* sigma */
    { "PS", 0x8F },  /* psi */
    { "S0", 0x90 },  /* small 0 */
    { "S1", 0x91 },  /* small 1 */
    { "S2", 0x92 },  /* small 2 */
    { "S3", 0x93 },  /* small 3 */
    { "S4", 0x94 },  /* small 4 */
    { "S5", 0x95 },  /* small 5 */
    { "S6", 0x96 },  /* small 6 */
    { "S7", 0x97 },  /* small 7 */
    { "S8", 0x98 },  /* small 8 */
    { "S9", 0x99 },  /* small 9 */
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
    { "TR", 0xEE },  /* triangle */
    { "/",  0xEF },  /* over */
    { "*",  0xF0 },  /* multiply */
    { "PN", 0xF7 },  /* pound */
    { "CN", 0xF8 },  /* cent */
    { "+-", 0xF9 },  /* +/- */
    { "-+", 0xFA },  /* -/+ */
    { NULL, 0    }
};

/*
 *  Escape codes with \
 *  PB-700/PB-1000 variant
 */
struct _escapes Escapes_PB[] = 
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
 *  Flag for binary mode 
 */
bool BinMode = TRUE;

/*
 *  Flag set if FX-850P or PB-1000
 */
bool Fx850P = FALSE;

/*
 *  Flag set if FP-200
 */
bool Fp200 = FALSE;

/*
 *  Escape mode
 */
enum { ESC_NONE, ESC_FX, ESC_PB, ESC_HEX, UTF8_PB, UTF8_FX 
} EscapeMode = ESC_NONE;

/*
 *  Logarithm mode
 */
enum { LOG_NONE, LOG_FX, LOG_PB } LogarithmMode = LOG_NONE;

/*
 *  Suppress Header
 */
bool NoHeader = FALSE;

/*
 *  File is from MD100 disk drive
 */
bool DiskMode = FALSE;

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

/*
 *  MD100 header bytes
 */
struct _disk_header {
    unsigned char boot_flag;
    unsigned char file_top[ 2 ];
    unsigned char file_end[ 2 ];
    unsigned char file_type;
    char file_name[ 8 ];
    char file_ext[ 3 ];
    unsigned char password[ 8 ];
    unsigned char ml_top[ 2 ];
    unsigned char ml_end[ 2 ];
    unsigned char ml_entry[ 2 ];
    unsigned char unused[ 256 - 31 ];
} DiskHeader;

/*
 *  Definitions for MD100 header
 */
#define BOOT_NONE  0x00
#define BOOT_START 0x01
#define BOOT_TIMER 0x02
#define DTYPE_M    0x0D           /* machine code */
#define DTYPE_B    0x10           /* tokenized BASIC */
#define DTYPE_S    0x24           /* sequential ASCII file */
#define DTYPE_R    0xA4           /* relative file for random access */
#define DTYPE_C    0xD4           /* C source file (ASCII) */

void printHeader( void )
{
    int i;
    static bool first = TRUE;

    if ( NoHeader ) {
        return;
    }

    if ( first ) {
        first = FALSE;
    }
    else {
        putchar( '\n' );
    }

    switch ( Header.file_type ) {

    case TYPE_PROGRAM:
        printf( "Program: " );
        break;

    case TYPE_ALL:
        printf( "All Programs: " );
        break;

    case TYPE_ASCII:
        printf( "Program (ASCII): " );
        break;

    case TYPE_DATA:
        printf( "Data: " );
        break;

    case TYPE_PROG850:
        printf( "Program (%s): ", Fp200 ? "FP-200" : "FX-850P/PB-1000" );
        break;

    case TYPE_ALL850:
        printf( "All Programs (%s): ", Fp200 ? "FP-200" : "FX-850P/PB-1000" );
        break;

    }

    /*
     *  Filename
     */
    for ( i = 7; i >= 0 && Header.file_name[ i ] == ' '; --i );
    if ( i < 0 ) {
        printf( "No name\n" );
    }
    else {
        printf( "\"%.*s", i + 1, Header.file_name );

        for ( i = 2; i >= 0 && Header.file_ext[ i ] == ' '; --i );
        if ( i >= 0 ) {
            printf( ".%.*s", i + 1, Header.file_ext );
        }
        printf( "\"\n" );
    }

    /*
     *  Password
     */
    if ( Header.password[ 0 ] != 0xff ) {
        printf( "Password: \"" );
        for ( i = 0; i < 8 && Header.password[ i ] != 0xff; ++i ) {
            putchar( Header.password[ i ] ^ 0xff );
        }
        printf( "\"\n" );
    }

#if DEBUG
    printf( "Parameters:" );
    for ( i = 0; i < 12; ++i ) {
        printf( " %02.2X", Header.parameters[ i ] );
    }
    printf( "\n" );
    printf( "Program length: %d\n", Header.parameters[ 6 ]
                                  + Header.parameters[ 7 ] * 256 );
#endif
}


/*
 *  Print MD100 header
 */
void printDiskHeader( void )
{
    int i;
    unsigned int end;

    if ( NoHeader ) {
        return;
    }

    switch ( DiskHeader.file_type ) {

    case DTYPE_M:
        /*
         *  Address info
         */
        printf( "Machine Language: " );
        end = DiskHeader.ml_end[ 0 ] + DiskHeader.ml_end[ 1 ] * 256 - 1,
        printf( "%02.2X%02.2X-%04.4X, Entry: %02.2X%02.2X\n",
                DiskHeader.ml_top[ 1 ],   DiskHeader.ml_top[ 0 ], 
                end, 
                DiskHeader.ml_entry[ 1 ], DiskHeader.ml_entry[ 0 ] );
        break;

    case DTYPE_B:
        /*
         *  BASIC Filename
         */
        printf( "BASIC Program: " );
        for ( i = 7; i >= 0 && DiskHeader.file_name[ i ] == ' '; --i );
        if ( i < 0 ) {
            printf( "No name" );
        }
        else {
            printf( "\"%.*s", i + 1, DiskHeader.file_name );

            for ( i = 2; i >= 0 && DiskHeader.file_ext[ i ] == ' '; --i );
            if ( i >= 0 ) {
                printf( ".%.*s", i + 1, DiskHeader.file_ext );
            }
            putchar( '"' );
        }

        /*
         *  Boot flag
         */
        switch ( DiskHeader.boot_flag ) {

        case BOOT_NONE:
            putchar( '\n' );
            break;

        case BOOT_START:
            printf( ", Boot\n" );
            break;

        case BOOT_TIMER:
            printf( ", Timer\n" );
            break;

        default:
            printf( ", Start flag: %02.2X\n" );
            break;
        }

        /*
         *  Password
         */
        if ( DiskHeader.password[ 0 ] != 0xff ) {
            printf( "Password: \"" );
            for ( i = 0; i < 8 && DiskHeader.password[ i ] != 0xff; ++i ) {
                putchar( DiskHeader.password[ i ] ^ 0xff );
            }
            printf( "\"\n" );
        }
        break;

    default:
        printf( "Type %02.2X\n", DiskHeader.file_type );
    }
    putchar( '\n' );
}



/*
 *  Print a character, translate tokens
 */
void casioprint( int c, bool is_data )
{
    static bool insert_space = FALSE;
    static bool pending_colon = FALSE;
    static int last = 0;
    static bool quoted = FALSE;
    static bool quoted_until_eol = FALSE;
    char *p = NULL;
    char buffer[] = "\\XX\\XX";
    struct _escapes *ep;
    struct _utf8 *up;

    if ( is_data ) {
        /*
         *  No token detection
         */
        quoted = quoted_until_eol = TRUE;
    }

    if ( Fx850P || Fp200 ) {
        /*
         *  FX-850P / PB-1000
         */
        static int prefix = 0;
        static int lsb = -1;

        if ( Fx850P && LogarithmMode == LOG_FX 
          || Fp200  && LogarithmMode == LOG_PB )
        {
            /*
             *  Logs are already correct
             */
            LogarithmMode = LOG_NONE;
        }

        if ( prefix == 3 ) {
            /*
             *  Binary line number
             */
            if ( lsb == -1 ) {
                lsb = c;
                c = -1;
            }
            else {
                sprintf( buffer, "%d", lsb + c * 256 );
                p = buffer;
                lsb = -1;
                prefix = 0;
            }
        }
        else {
            /*
             *  Text or token
             */
            switch ( c ) {

            case 0x00:
                p = "\n";
                break;

            case 0x01:
                pending_colon = TRUE;
                c = -1;
                break;

            case 0x02:
                p = "'";
                quoted_until_eol = TRUE;
                break;

            case 0x03:
                prefix = c;
                lsb = -1;
                c = -1;
                break;

            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
                prefix = c;
                c = -1;
                break;

            default:
                if ( prefix != 0 ) {
                    /*
                     *  Token
                     */
                    if ( Fp200 ) {
                        if ( prefix != 4 || c < 0x25 || c > 0xB1 ) {
                            p = NULL;
                        }
                        else {
                            p = tokens200[ c - 0x25 ];
                        }
                    }
                    else {
                        if ( prefix < 4 || prefix > 7 || c < 0x40 || c > 0xCF ) {
                            p = NULL;
                        }
                        else {
                            p = tokens850[ prefix - 4 ][ c - 0x40 ];
                        }
                    }
                    if ( p == NULL ) {
                        sprintf( buffer, "\\%02.2X\\%02.2X", prefix,
                                         (unsigned char) c );
                        p = buffer;
                    }
                }
                prefix = 0;
            }
        }
    }
    else {
        /*
         *  PB-700 or FX-750
         */
        if ( LogarithmMode == LOG_PB ) {
            /*
             *  Logs are already correct
             */
            LogarithmMode = LOG_NONE;
        }
        if ( c == 0xFF ) {
            p = "\n";
        }
        else if ( !quoted_until_eol && c == 0xFE ) {
            p = ":";
        }  
        else if ( !quoted && ( c < ' ' || c >= 0x80 ) ) {
            /*
             *  replacable token
             */
            switch ( c ) {

            case 0xFE:
                p = ":";
                break;

            default:
                if ( c >= 0x80 && c <= 0xEC ) {
                    p = tokens[ c - 0x80 ];
                    if ( c == 0xB7 || c == 0xB8 ) {
                        /*
                         *  REM or DATA
                         */
                        quoted = quoted_until_eol = TRUE;
                    }
                }
                else {
                    /*
                     *  Not a valid token, escape it
                     */
                    sprintf( buffer, "\\%02.2X", (unsigned char) c );
                    p = buffer;
                }
            }
        }
    }

    if ( c < 0 ) {
        /*
         *  Character is already handled
         */
    }
    else if ( c == ' ' ) {
        /*
         *  Literal space is always printed
         */
        if ( pending_colon ) {
            /*
             *  print the postponed colon
             */
            putchar( ':' );
            pending_colon = FALSE;
        }
        putchar( c );
        insert_space = FALSE;
        last = ' ';
    }
    else {
        if ( p == NULL ) {
            
            if ( EscapeMode ) {
                /*
                 *  UTF-8 takes precedence of escaping
                 */
                up = EscapeMode == UTF8_FX ? Utf8_FX
                   : EscapeMode == UTF8_PB ? Utf8_PB
                   : NULL;

                if ( up != NULL ) {
                    for ( ; up != NULL && up->text != NULL; ++up ) {
                        if ( c == up->token ) {
                            p = up->text;
                            break;
                        }
                    }
                }
                else {
                    /*
                     *  Create escape sequences like \PI
                     */
                    ep = EscapeMode == ESC_FX ? Escapes_FX
                       : EscapeMode == ESC_PB ? Escapes_PB
                       : NULL;

                    for ( ; ep != NULL && ep->text != NULL; ++ep ) {
                        if ( c == ep->token ) {
                            sprintf( buffer, "\\%s", ep->text );
                            p = buffer;
                            break;
                        }
                    }
                }
                if ( p == NULL && ( c >= 0x80 || c == '\\' ) ) {
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

        switch ( LogarithmMode ) {

        case LOG_PB:
            /*
             *  Translate LOG/LN to LGT/LOG
             */
            if ( 0 == strcmp( p, "LOG " ) ) {
                p = "LGT ";
            } 
            else if ( 0 == strcmp( p, "LN " ) ) {
                p = "LOG ";
            }
            break;

        case LOG_FX:
            /*
             *  Translate LGT/LOG to LOG/LN
             */
            if ( 0 == strcmp( p, "LGT " ) ) {
                p = "LOG ";
            } 
            else if ( 0 == strcmp( p, "LOG " ) ) {
                p = "LN ";
            }
            break;

        }         

        if ( *p == ' ' && last == ' ' ) {
            /*
             *  Collapse spaces in tokens
             */
            ++p;
        }

        if ( pending_colon ) {
            if ( strncmp( p, "ELSE", 4 ) != 0 &&
                 strncmp( p, " ELSE", 5 ) != 0 )
            {
                /*
                 *  Print the postponed colon except before ELSE
                 */
                putchar( ':' );
            }
            pending_colon = FALSE;
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

            if ( quoted ) {
                /*
                 *  Print char as is, no further translation
                 */
                putchar( c );
                insert_space = FALSE;
            }
            else {
                /*
                 *  Check for spaces and unprintable characters
                 */
                if ( c < ' ' && c != '\n' ) {
                    printf( "\\%02.2X", (unsigned char) c );
                }
                else if ( c == ' ' && *p == '\0' ) {
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
 *  Convert a BCD coded byte to binary
 */
int bcd2bin( int c )
{
    int x;
    x = c / 16;
    return c - 6 * x;
}


/*
 *  Handle next character from file
 *
 *  Returns 0 when OK or the error code when invalid data encountered
 */
int list( int c )
{
    static int skipped = 5;
    static int idle_counter = 0;
    static int data_counter = 0;
    static int line_counter = 0;
    static int disk_header_counter = 0;
    static int line_number = 0;
    static int line_length = 0;
    static int prog_number = 0;
    static int segment_id = 0;
    static int ignore_counter = 0;
    static int transparent_counter = 0;
    static int length_counter = 0;
    static int ptlength_counter = 0;
    static int progtab_counter = 0;
    static int progtab_length = 0;
    static int program_length = 0;

    static bool leader_expected = TRUE;
    static bool dataseg_expected = FALSE;
    static bool textline_expected = FALSE;
    static bool progtab_expected = FALSE;
    static bool last_segment = FALSE;

    static int progtab[ 10 ];

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
                 *  "Premature end of a header/data segment" error
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
        if ( DiskMode ) {
            /*
             *  File is copied from an MD100
             */
            static int address = 0;
            static int binary_counter = 0;
            static unsigned char binary[ 16 ];
            
            if ( disk_header_counter < sizeof( DiskHeader ) ) {
                /*
                 *  Copy and print the header
                 */
                unsigned char *p = (unsigned char *) &DiskHeader;
                p[ disk_header_counter++ ] = (unsigned char) c;

                if ( disk_header_counter == sizeof( DiskHeader ) ) {
                    /*
                     *  End of header, mimic data segment
                     */
                    printDiskHeader();
                    leader_expected = FALSE;
                    data_counter = 3;
                    segment_id = 'D';
                    if ( DiskHeader.file_type == DTYPE_B ) {
                        /*
                         *  Determine BASIC program length
                         */
                        program_length = DiskHeader.file_end[ 0 ] 
                                       + DiskHeader.file_end[ 1 ] * 256
                                       - DiskHeader.file_top[ 0 ] 
                                       - DiskHeader.file_top[ 1 ] * 256
                                       + 1;
                        Header.file_type = TYPE_PROG850;
                        Fx850P = TRUE;
                    } 
                    else if ( DiskHeader.file_type == DTYPE_M ) {
                        /*
                         *  Determine binary start address
                         */
                        address = DiskHeader.ml_top[ 0 ]
                                + DiskHeader.ml_top[ 1 ] * 256;
                    }
                }
                return 0;
            }
            else if ( DiskHeader.file_type != DTYPE_B ) {
                /*
                 *  Binary
                 */
                if ( c != EOF ) {
                    binary[ binary_counter++ ] = (unsigned char) c;
                }
                if ( binary_counter == 16 || c == EOF && binary_counter > 0 ) {
                    /*
                     *  Dump up to 16 bytes
                     */
                    int i;
                    
                    printf( "%04.4X:", address );
                    for ( i = 0; i < binary_counter; ++i ) {
                        printf( " %02.2X", binary[ i ] );
                    }
                    while ( i++ < 16 ) {
                        printf( "   " );
                    }
                    printf( "  " );
                    for ( i = 0; i < binary_counter; ++i ) {
                        c = binary[ i ];
                        if ( c < ' ' || c >= 0x80 ) {
                            c = '.';
                        }
                        putchar( c );
                    }
                    putchar( '\n' );
                    binary_counter = 0;
                    address += 16;
                }
                return 0;
            }
        }            

        if ( leader_expected ) {
            /*
             *  Check for 'H'eader or 'D'ata segment
             */
            if ( !dataseg_expected && c == 'H' || c == 'D' ) {
                /*
                 *  Valid header byte encountered
                 */
                data_counter = 0;
                leader_expected = FALSE;
#if DEBUG
                printf( "Segment: %c\n", c );
#endif
            }
            else {
#if DEBUG
                printf( " skipped\n" );
#endif                    
                return 0;
            }
        }
    }

    if ( DiskMode && c == EOF ) {
        return 0;
    }

    if ( ++data_counter == 1 ) {
        /*
         *  Handle the first segment byte
         */
        segment_id = c;
        transparent_counter = 0;
        length_counter = 0;

        if ( dataseg_expected ) {
            /*
             *  Must be data segment
             */
            if ( c == 'H' ) {
                /*
                 *  "Premature end of a file" error
                 */
                return 3;
            }
            else if ( c != 'D' ) {
                /*
                 *  "Unknown segment identifier" error
                 */
                return 5;
            }
        }
        else {
            /*
             *  Header segment expected
             */
            if ( c == 'D' ) {
                /*
                 *  Unexpected Data segment: "Header segment expected" error
                 */
                return 4;
            }
            else if ( c != 'H' ) {
                /*
                 *  "Unknown segment identifier" error
                 */
                return 5;
            }
        }
        return 0;
    }

    /*
     *  Handle subsequent segment bytes
     */
    if ( ignore_counter > 0 ) {
        /*
         *  Skip checksums, termination chars and the like
         */
        if ( --ignore_counter == 0 ) {
            leader_expected = TRUE;
            skipped = 5;
        }
        return 0;
    }

    if ( segment_id == 'D' ) {
        /*
         *  Handle the data segment
         */
        if ( textline_expected ) {
            /*
             *  Handle textlines in DATA or ASCII file
             */
            last_segment = ( c != 0 );
#if DEBUG
            printf( "last_segment = %d\n", last_segment );
#endif
            length_counter = 2;
            transparent_counter = 0;
            textline_expected = FALSE;
            return 0;
        }

        if ( length_counter > 0 ) {
            /*
             *  Length, LSB first
             */
            if ( --length_counter == 1 ) {
                transparent_counter = c;
            }
            else {
                transparent_counter += c * 256;
            }
            return 0;
        }

        if ( transparent_counter > 0 ) {
            /*
             *  Data of variable length
             */
            if ( --transparent_counter == 0 ) {
                /*
                 *  End of line or block
                 */
                putchar( c );

                /*
                 *  Skip checksum byte(s)
                 */
                if ( last_segment ) {
                    dataseg_expected = FALSE;
                }
                ignore_counter = Fp200 ? 2 : 1;
            }
            else if ( c != 0x0d ) {
                /*
                 *  Output the data as is, except for redundant CRs
                 *  If EscapeMode  is set, translate extended codes
                 */
                if ( EscapeMode ) {
                    casioprint( c, TRUE ); // force data mode
                }
                else {
                    putchar( c );
                }
            }
            return 0;
        }

        if ( progtab_expected ) {
            /*
             *  Table of ten length words follows
             */
            if ( c != 0 ) {
                /*
                 *  "Program table is last segment" error
                 */
                return 9;
            }
            last_segment = FALSE;
            ptlength_counter = 2;
            progtab_expected  = FALSE;
            return 0;
        }

        if ( ptlength_counter > 0 ) {
            /*
             *  Program table length, LSB first
             */
            if ( --ptlength_counter == 1 ) {
                progtab_length = c;
            }
            else {
                progtab_length += c * 256;
                progtab_counter = 2;
                prog_number = progtab_length / 2;
                if ( prog_number != 10 ) {
                    return 11;
                }
            }
            return 0;
        }

        if ( progtab_counter > 0 ) {
            /*
             *  Program table entry, LSB first
             */
            if ( --progtab_counter == 1 ) {
                --prog_number;
                progtab[ prog_number ] = c;
            }
            else {
                progtab[ prog_number ] += c * 256;
                if ( prog_number == 0 ) {
                    /*
                     *  end of segment, skip checksum
                     */
                    ignore_counter = Fp200 ? 2 : 1;
                }
                else {
                    /*
                     *  Next entry
                     */
                    progtab_counter = 2;
                }
            }
            return 0;
        }

        if ( !Fx850P && !Fp200 && ( c == 0xF0 || c == 0xF1 ) ) {
            /*
             *  End of segment marker
             */
            leader_expected = TRUE;
            skipped = 5;
            dataseg_expected = ( c == 0xF1 );
            return 0;
        }

        if ( data_counter == 2 ) {
            /*
             *  Special handling of first data byte of segment
             */
            switch ( Header.file_type ) {

            case TYPE_ALL:
                printf (" \n P0\n");
                prog_number = 0;
                /* no break */

            case TYPE_PROGRAM:  
                line_counter = 0;
                printf( "\n" );
                /* no return! */
                break;

            case TYPE_ASCII:
            case TYPE_DATA:
                if ( c != Header.file_type ) {
                    /*
                     *  "Invalid subtype in data segment" error
                     */
                    return 7;
                }
                textline_expected = TRUE;
                return 0;

            case TYPE_ALL850:
                if ( c == 0x24 ) {
                    /*
                     *  Program table for 10 Programs follows
                     */
                    progtab_expected = TRUE;
                    return 0;
                }
                if ( c != 0x10 ) {
                    /*
                     *  "Invalid subtype in data segment" error
                     */
                    return 7;
                }
                prog_number = 0;
                program_length = progtab[ prog_number ];
                if ( program_length == 0 ) {
                    /*
                     *  "Missing program table" error
                     */
                    return 10;
                }
                if ( program_length > 1 ) {
                    printf( " \n P0\n\n" );
                }
#if DEBUG
                printf( "program_length = %d\n", program_length );
#endif
                return 0;

            case TYPE_PROG850:
                program_length = Header.parameters[ 6 ]
                               + Header.parameters[ 7 ] * 256;
                printf( "\n" );
#if DEBUG
                printf( "program_length = %d\n", program_length );
#endif
                return 0;

            default:
                /*
                 *  "Invalid subtype in data segment" error
                 */
                return 7; /* invalid file type */
            }
        }

        if ( !Fx850P && !Fp200 ) {
            /*
             *  PB-700 / FX-750P
             */
            if ( c == 0xE0 && line_counter == 0 ) {
                /*
                 *  Programs separator
                 */
                prog_number++;
                if ( Header.file_type != TYPE_ALL || prog_number > 10 ) {
                    /*
                     *  "Unexpected program separator" error
                     */
                    return 6;
                }
                if ( prog_number < 10 ) {
                    printf( "\n P%d\n\n", prog_number );
                }
                else {
                    printf( "\n END\n" );
                }
                return 0;
            }

            if ( ++line_counter == 1 ) {
                /*
                 *  The least significant byte of the line number
                 */
                line_number = bcd2bin( c );
                return 0;
            }
            else if ( line_counter == 2 ) {
                /*
                 *  The most significant byte of the line number
                 */
                line_number += 100 * bcd2bin( c );
                printf( "%d ", line_number );
                return 0;
            }

            /*
             *  Print remaining bytes of the data segment
             */
            if ( c == 0xFF ) {
                /*
                 *  End of line: prepare for new linenumber
                 */
                line_counter = 0;
            }
            casioprint( c, FALSE );
        }
        else {
            /*
             *  FX-850P / FP-200
             */
            if ( data_counter == 3 ) {
                /*
                 *  "Last segment" Flag
                 */
                last_segment = ( c != 0 );
#if DEBUG
                printf( "last_segment = %d\n", last_segment );
#endif
                line_counter = 0;
                return 0;
            }
            if ( --program_length <= 0 ) {
                /*
                 *  End of current program
                 */
                if ( Header.file_type == TYPE_ALL850 ) {
                    /*
                     *  Proceed to next program
                     */
                    if ( c != 0 ) {
                        /*
                         *  "Missing Program seperator" error
                         */
                        return 12;
                    }
                    ++prog_number;
                    if ( prog_number < 10 ) {
                        /*
                         *  Print program number
                         */
                        program_length = progtab[ prog_number ];
                        if ( program_length > 1 ) {
                            printf( "\n P%d\n\n", prog_number );
                        }
#if DEBUG
                        printf( "program_length = %d\n", program_length );
#endif
                    }
                    else {
                        /*
                         *  End of segment: skip checksum
                         */
                        if ( last_segment ) {
                            dataseg_expected = FALSE;
                        }
                        ignore_counter = Fp200 ? 2 : 1;
                        printf( "\n END\n" );
                    }
                }
                else {
                    /*
                     *  End of single program
                     */
                    if ( c != 0 ) {
                        /*
                         *  "Missing line terminator" error
                         */
                        return 8;
                    }
                    /*
                     *  End of segment: skip checksum
                     */
                    if ( last_segment ) {
                        dataseg_expected = FALSE;
                    }
                    ignore_counter = Fp200 ? 2 : 1;
                    printf( "\n" );
                }
                return 0;
            }

            /*
             *  Count position within line
             */
            if ( ++line_counter == 1 ) {
                /*
                 *  length byte
                 */
                line_length = c + 1;
#if DEBUG
                printf( "line_length = %d\n", line_length );
#endif
                if ( line_length == 1 ) {
                    /*
                     *  Empty line marks end of program
                     *  Skip rest of segment
                     */
                    if ( last_segment ) {
                        dataseg_expected = FALSE;
                    }
                    line_counter = 0;
                    ignore_counter = program_length; 
                    program_length = 0;
                    printf( "\n" );
                }
                return 0;
            }
            else if ( line_counter == 2 ) {
                /*
                 *  The least significant byte of the line number
                 */
                line_number = c;
                return 0;
            }
            else if ( line_counter == 3 ) {
                /*
                 *  The most significant byte of the line number
                 */
                line_number += 256 * c;
                printf( "%d", line_number );
                return 0;
            }
            else if ( line_counter == 4 && c != ' ' ) {
                /*
                 *  Insert a space after the line number
                 */
                putchar( ' ' );
            }
            if ( line_counter >= line_length ) {
                /*
                 *  End of line
                 */
                if ( c != 0 ) {
                    /*
                     *  "Missing line terminator" error
                     */
                    return 8;
                }
                /*
                 *  Prepare for new linenumber
                 */
                line_counter = 0;
            }

            /*
             *  Print remaining bytes of the data segment
             */
            casioprint( c, FALSE );
        }
        return 0;
    }

    if ( !DiskMode ) {
        /*
         *  Handle the header segment
         */
        ((char *) &Header)[ data_counter - 1 ] = c;

        if ( data_counter == HEADER_SIZE ) {
            /*
             *  End of header
             */
            printHeader();
            Fx850P = Header.file_type == TYPE_PROG850
                  || Header.file_type == TYPE_ALL850;
            if ( Fp200 ) {
                Fx850P = FALSE;
            }
            if ( !Fx850P && EscapeMode == ESC_FX ) {
                /*
                 *  Wrong escape mode, correct it
                 */
                EscapeMode = ESC_PB;
            }
            ignore_counter = Fp200 ? 2 : 1;
            dataseg_expected = TRUE;
            if ( Header.file_type == TYPE_DATA
              || Header.file_type == TYPE_ASCII )
            {
                printf( "\n" );
            }
        }
    }
    return 0;
}


/*
 *  Main program
 */
int main( int argc, char *argv[] )
{
    int c1;
    int counter;          /* character counter */
    long position = 0;    /* file position */
    int err_code;
    unsigned int x;       /* 12-bit word */
    FILE *infp = NULL;
    KCS_FILE *wfp = NULL;
    int skip = 0;
    enum { NO, SLOW, FAST, HIGH } wavemode = NO;
    int ignore = FALSE;

    ++argv;
    --argc;

    while ( argc > 1 && **argv == '-' ) {

        if ( strncmp( *argv, "-2", 2 ) == 0 ) {
            Fp200 = TRUE;
        }
        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            BinMode = TRUE;
            skip = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-d", 2 ) == 0 ) {
            BinMode = TRUE;
            DiskMode = TRUE;
        }
        if ( strncmp( *argv, "-a", 2 ) == 0 ) {
            BinMode = FALSE;
            skip = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-i", 2 ) == 0 ) {
            ignore = TRUE;
        }
        if ( strncmp( *argv, "-n", 2 ) == 0 ) {
            NoHeader = TRUE;
        }

        if ( strncmp( *argv, "-w", 2 ) == 0 ) {
            BinMode = FALSE;
            wavemode = SLOW;
            if ( toupper( (*argv)[ 2 ] ) == 'F' ) {
                wavemode = FAST;
            }
            else if ( toupper( (*argv)[ 2 ] ) == 'H' ) {
                wavemode = HIGH;
            }
        }
        if ( strncmp( *argv, "-e", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'F':
                EscapeMode = ESC_FX; break;
            case 'P':
                EscapeMode = ESC_PB; break;
            case 'X':
            case 'H':
            case '\0':
                EscapeMode = ESC_HEX; break;
            }
        }
        if ( strncmp( *argv, "-u", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case '\0':
            case 'F':
                EscapeMode = UTF8_FX; break;
            case 'P':
                EscapeMode = UTF8_PB; break;
            }
        }
        if ( strncmp( *argv, "-l", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'F':
                LogarithmMode = LOG_FX; break;
            case 'P':
                LogarithmMode = LOG_PB; break;
            }
        }
        ++argv;
        --argc;
    }
#if DEBUG
    printf( "binmode = %d, wavemode = %d\n", BinMode, wavemode );
#endif

    if ( argc < 1 ) {
        printf( "usage: list850 <options> infile > outfile\n"
                "         -2 file is from FP-200\n"
                "         -w[S|F|H] reads a WAV file directly\n"
                "           S: slow (PB-700, FX-750P)\n"
                "           F: fast (FX-850P, PB-1000)\n"
                "           H: high speed (PB-1000 with FA-7 at 2400 baud)\n"
                "         -a reads an ASCII encoded file from Piotr's interface\n"
                "         -b<skip> reads a binary file, "
                          "<skip> is an optional offset\n"
                "         -d reads a file copied from an MD100 disk\n" 
                "         -i ignores any errors on the input stream\n"
                "         -e[F|P|X] use backslash escapes in strings\n"
                "           F: FX-850P extended character set\n"
                "           P: PB-1000 graphics character set\n"
                "           X: generic hexadecimal escapes (default)\n"
                "         -u[F|P] create unicode (UTF-8) instead of escapes\n"
                "         -l[F|P] handling of LOG/LN versus LGT/LOG\n"
                "           F: FX-850P/VX/Z logarithm syntax LOG and LN\n"
                "           P: PB-700/PB-1000 logarithm syntax LGT and LOG\n"
                "         -n no header output (good for data files)\n"
              );
        return 2;
    }

    if ( wavemode ) {
        /*
         *  Open WAV file
         */
        int baud = wavemode == HIGH ? 2400
                 : wavemode == FAST ? 1200
                 : 300;
        if ( ( wfp = kcsOpen( *argv, "rb", baud, 8, 'E', 2 ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    else {
        /*
         *  Open ASCII or binary file
         */
        if ( ( infp = fopen( *argv, BinMode ? "rb" : "rt" ) ) == NULL ) {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }

    x = 0;
    counter = 0;

    if ( wavemode != NO ) {
        /*
         *  Process WAV file
         */
        while ( ( c1 = kcsReadByte( wfp ) ) >= 0 ) {
#if DEBUG
            printf( "[%02.2X]", c1 & 0xff );
#endif
            if ( ignore 
                 && c1 != KCS_LEAD_IN && c1 & ( KCS_FRAMING | KCS_PARITY ) ) 
            {
                continue;
            }
            if ( ( err_code = list( c1 ) ) != 0 ) {
                printf( "\nInvalid data encountered \\%02.2X - %s.\n",
                        c1 & 0xff, err_msg[ err_code ] );
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
         *  Process ASCII or binary file
         */
        while ( ( c1 = fgetc( infp ) ) != EOF ) {
#if DEBUG
            printf( "[%02.2X]", c1 );
#endif
            if ( BinMode ) {
                /*
                 *  Plain binary data
                 */
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
                continue;
            }

            /*
             *  Piotr's ASCII format
             */
            if ( c1 < 0x30 || c1 > 0x6F ) {
                /*
                 *  Skip invalid characters
                 */
                counter = 0;
            }
            else {
                /*
                 *  Shift the received 6-bit data into the 12-bit word
                 */
                x = ( x >> 6 ) & 0x003F;
                x |= ( ( (unsigned int) ( c1 - 0x30 ) ) << 6 );

                if ( ++counter == 2 ) {
                    /*
                     *  Already 2 characters processed
                     */
                    counter = 0;

                    /*
                     *  strip the start, stop and parity bits
                     */
                    c1 = (int) ( ( x >> 1 ) & 0xFF );

                    if ( ( x & 0x01 ) != 0 ) {
                        /*
                         *  idle string (leader)
                         */
                        c1 = KCS_LEAD_IN;
                    }

                    if ( ( err_code = list( c1 ) ) != 0 ) {
                        /*
                         *  Error
                         */
                        printf( "\nInvalid data encountered [%02.2X] - %s.\n",
                                c1, err_msg[ err_code ] );
                        break;
                    }
                }
            }
        }
        if ( DiskMode ) {
            /*
             *  Allow end of file processing
             */
            list( EOF );
        }
        fclose( infp );
    }

    return 0;
}
