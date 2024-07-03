/*
 *  list74.c - List the contents of a TI-74 tape file
 *
 *  This program translates a WAV file recorded through the sound card from
 *  a TI-74 to a human readable listing
 *
 *  Options:
 *    -b read binary file, may also be in PGM (PC-Interface) format
 *    -w read wave file
 *
 *  Written by Marcus von Cube
 */
#define _DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"

/*
 *  Error messages
 */
char *ErrorMsg[] = 
{
    "",                                       /* OK */
    "Read error",                             /* message #1 */
    "Checksum error",                         /* message #2 */
    "Can't find block",                       /* message #3 */
    "Premature end of block",                 /* message #4 */
    "Block size inconsistent",                /* message #5 */
    "Block too large",                        /* message #6 */
    "Bad line length or missing terminator",  /* message #7 */
    "Variable table is missing",              /* message #8 */
};

/*
 *  token data
 *  0x20 .. 0x7f: variables
 */
char *Tokens[ 256 ] = 
{
    "\\00",      /* Delimiter */
    "\\01",      /* unused codes, shouldn't appear */
    "\\02",
    "\\03",
    "\\04",
    "\\05",
    "\\06",
    "\\07",
    "\\08",
    "\\09",
    "\\0A",
    "\\0B",
    "\\0C",
    "\\0D",
    "\\0E",
    "\\0F",
    "\\10",
    "\\11",
    "\\12",
    "\\13",
    "\\14",
    "\\15",
    "\\16",
    "\\17",
    "\\18",
    "\\19",
    "\\1A",
    "\\1B",
    "\\1C",
    "\\1D",
    "\\1E",
    "\\1F",
    "\\20",      /* variables start here */
    "\\21",
    "\\22",
    "\\23",
    "\\24",
    "\\25",
    "\\26",
    "\\27",
    "\\28",
    "\\29",
    "\\2A",
    "\\2B",
    "\\2C",
    "\\2D",
    "\\2E",
    "\\2F",
    "\\30",
    "\\31",
    "\\32",
    "\\33",
    "\\34",
    "\\35",
    "\\36",
    "\\37",
    "\\38",
    "\\39",
    "\\3A",
    "\\3B",
    "\\3C",
    "\\3D",
    "\\3E",
    "\\3F",
    "\\40",
    "\\41",
    "\\42",
    "\\43",
    "\\44",
    "\\45",
    "\\46",
    "\\47",
    "\\48",
    "\\49",
    "\\4A",
    "\\4B",
    "\\4C",
    "\\4D",
    "\\4E",
    "\\4F",
    "\\50",
    "\\51",
    "\\52",
    "\\53",
    "\\54",
    "\\55",
    "\\56",
    "\\57",
    "\\58",
    "\\59",
    "\\5A",
    "\\5B",
    "\\5C",
    "\\5D",
    "\\5E",
    "\\5F",
    "\\60",
    "\\61",
    "\\62",
    "\\63",
    "\\64",
    "\\65",
    "\\66",
    "\\67",
    "\\68",
    "\\69",
    "\\6A",
    "\\6B",
    "\\6C",
    "\\6D",
    "\\6E",
    "\\6F",
    "\\70",
    "\\71",
    "\\72",
    "\\73",
    "\\74",
    "\\75",
    "\\76",
    "\\77",
    "\\78",
    "\\79",
    "\\7A",
    "\\7B",
    "\\7C",
    "\\7D",
    "\\7E",
    "\\7F",      /* variables end here */
    "\\80",      /* extended token delimiter */
    "DISPLAY ",  /* 81 */
    "REM ",      /* 82 */
    "DIM ",      /* 83 */
    "IMAGE\xFF", /* 84, force output of a space after keyword */
    "STOP",      /* 85 */
    "END",       /* 86 */
    "LET ",      /* 87 */
    "INPUT ",    /* 88 */
    "LINPUT ",   /* 89 */
    "PRINT ",    /* 8A */
    "PAUSE ",    /* 8B */
    "OPEN ",     /* 8C */
    "CLOSE ",    /* 8D */
    "RESTORE ",  /* 8E */
    "\\8F",      /* 8F */
    "RANDOMIZE ",/* 90 */
    "ON ",       /* 91 */
    " GOTO ",    /* 92 */
    " GOSUB ",   /* 93 */
    "RETURN ",   /* 94 */
    "CALL ",     /* 95 */
    "\\96",      /* 96 */
    "\\97",      /* 97 */
    "SUB ",      /* 98 */
    "SUBEXIT",   /* 99 */
    "SUBEND",    /* 9A */
    "FOR ",      /* 9B */
    "NEXT ",     /* 9C */
    "IF ",       /* 9D */
    "ELSE ",     /* 9E */
    "\\9F",      /* 9F */
    "!",         /* A0 */
    "READ ",     /* A1 */
    "DATA\xFF",  /* A2, force output of space after keyword */
    "ACCEPT ",   /* A3 */
    "\\A4",      /* A4 */
    "\\A5",      /* A5 */
    "\\A6",      /* A6 */
    "\\A7",      /* A7 */
    "\\A8",      /* A8 */
    "\\A9",      /* A9 */
    " THEN ",    /* AA */
    " TO ",      /* AB */
    " STEP ",    /* AC */
    ",",         /* AD */
    ";",         /* AE */
    ") ",        /* AF */
    " OR ",      /* B0 */
    " AND ",     /* B1 */
    " XOR ",     /* B2 */
    "<>",        /* B3 */
    "<=",        /* B4 */
    ">=",        /* B5 */
    "=",         /* B6 */
    "<",         /* B7 */
    ">",         /* B8 */
    "&",         /* B9 */
    "+",         /* BA */
    "-",         /* BB */
    "*",         /* BC */
    "/",         /* BD */
    "^",         /* BE */
    "\\BF",      /* BF */
    "(",         /* C0 */
    "NOT ",      /* C1 */
    "\\C2",      /* BCD number (2 Byte) 1-2 digits */
    "\\C3",      /* BCD number (3 Byte) 3-4 digits */
    "\\C4",      /* BCD number (4 Byte) 5-6 digits */
    "\\C5",      /* BCD number (5 Byte) 7-8 digits */
    "\\C6",      /* BCD number (6 Byte) 9-10 digits */
    "\\C7",      /* BCD number (7 Byte) 11-12 digits */
    "\\C8",      /* BCD number (8 Byte) 13-14 digits */
    "\\C9",      /* "Quoted String" */
    "\\CA",      /* String without quotes */
    "\\CB",      /* binary value */
    "\\CC",      /* CC */
    "SEG$",      /* CD */
    "RPT$",      /* CE */
    "POS",       /* CF */
    "LEN",       /* D0 */
    "VAL",       /* D1 */
    "NUMERIC",   /* D2 */
    "ASC",       /* D3 */
    "RND",       /* D4 */
    "PI",        /* D5 */
    "KEY$",      /* D6 */
    "CHR$",      /* D7 */
    "STR$",      /* D8 */
    "ABS",       /* D9 */
    "ACOS",      /* DA */
    "ASIN",      /* DB */
    "ATN",       /* DC */
    "COS",       /* DD */
    "EXP",       /* DE */
    "INT",       /* DF */
    "ATANH",     /* E0 */
    "LN",        /* E1 */
    "LOG",       /* E2 */
    "SGN",       /* E3 */
    "SIN",       /* E4 */
    "SQR",       /* E5 */
    "TAN",       /* E6 */
    "EOF",       /* E7 */
    "FRE",       /* E8 */
    "SINH",      /* E9 */
    "COSH",      /* EA */
    "TANH",      /* EB */
    "ASINH",     /* EC */
    "ACOSH",     /* ED */
    "NULL",      /* EE */
    "VALIDATE",  /* EF */
    "#",         /* F0 */
    "ALL",       /* F1 */
    "TAB ",      /* F2 */
    "USING ",    /* F3 */
    "INTERNAL",  /* F4 */
    "OUTPUT",    /* F5 */
    "UPDATE",    /* F6 */
    "APPEND",    /* F7 */
    "VARIABLE",  /* F8 */
    "SIZE",      /* F9 */
    "AT",        /* FA */
    "REC ",      /* FB */
    "ERASE ",    /* FC */
    "RELATIVE ", /* FD */
    "\\FE",      /* FE */
    "\\FF"       /* FF */
};

/*
 *  tokens prefixed with 80, from 40 to 4F
 */
char *ExtendedTokens[ 16 ] = {
    "RUN",       /* 80 40 */
    "DELETE",    /* 80 41 */
    "FORMAT",    /* 80 42 */
    "BREAK",     /* 80 43 */
    "UNBREAK",   /* 80 44 */
    "DEG",       /* 80 45 */
    "RAD",       /* 80 46 */
    "GRAD",      /* 80 47 */
    "WARNING",   /* 80 48 */
    "ERROR ",    /* 80 49 */
    "PROTECTED", /* 80 4A */
    "DIGIT",     /* 80 4B */
    "UALPHANUM", /* 80 4C */
    "UALPHA",    /* 80 4D */
    "ALPHANUM",  /* 80 4E */
    "ALPHA"      /* 80 4F */
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
    { "->", 0x7E },  /* right arrow */
    { "<-", 0x7F },  /* left arrow */
    { "al", 0xEC },  /* alpha */
    { ":a", 0xE1 },  /* a umlaut */
    { "bt", 0xE2 },  /* beta */
    { "ep", 0xE3 },  /* epsilon */
    { "mu", 0xE4 },  /* mu */
    { "si", 0xE5 },  /* sigma */
    { "RT", 0xE8 },  /* root */
    { "-1", 0xE9 },  /* lifted -1 */
    { "^X", 0xEB },  /* lifted X */
    { "CN", 0xEC },  /* cent */                     
    { "PN", 0xED },  /* pound */
    { "~n", 0xEE },  /* n tilde */
    { ":o", 0xEF },  /* o umlaut */
    { "PS", 0xF2 },  /* Psi */
    { "oo", 0xF3 },  /* Infinity */
    { "OM", 0xF4 },  /* Omega */
    { ":u", 0xF5 },  /* U umlaut */
    { "SM", 0xF6 },  /* Sum */
    { "PI", 0xF7 },  /* Pi */
    { "X-", 0xF8 },  /* X bar */
    { ":-", 0xFD },  /* divide */
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
    { 0x7E, "->" },
    { 0x7F, "<-" },
    { 0xA2, "|"  },
    { 0xA3, "|"  },
    { 0xA4, "`"  },
    { 0xE1, "ae" },
    { 0xE4, "u"  },
    { 0xE8, "/"  },
    { 0xE9, "-1" },
    { 0xEC, "ct" },
    { 0xED, "L"  },
    { 0xEE, "n"  },
    { 0xEF, "oe" },
    { 0xF2, "0"  },
    { 0xF3, "oo" },
    { 0xF4, "O"  },
    { 0xF5, "ue" },
    { 0xF6, "S"  },
    { 0xF7, "pi" },
    { 0xF8, "x"  },
    { 0xFD, ":"  },
    { 0x00, NULL }
};

TRANSLATE ToDos[] = 
{
    { 0x5C, "\x9D" },
    { 0x7E, "\xAF" },
    { 0x7F, "\xAE" },
    { 0xA0, "\xFB" },
    { 0xA2, "\xDA" },
    { 0xA3, "\xD9" },
    { 0xE0, "\xE0" },
    { 0xE1, "\x84" },
    { 0xE2, "\xE1" },
    { 0xE3, "\xEE" },
    { 0xE4, "\xE6" },
    { 0xE5, "\xE5" },
    { 0xE8, "\xFB" },
    { 0xEC, "\x9B" },
    { 0xED, "\x9C" },
    { 0xEE, "\xA4" },
    { 0xEF, "\x94" },
    { 0xF2, "\xE9" },
    { 0xF3, "\xEC" },
    { 0xF4, "\xEA" },
    { 0xF5, "\x81" },
    { 0xF6, "\xE4" },
    { 0xF7, "\xE3" },
    { 0xFD, "\xF6" },
    { 0x00, NULL }
};


TRANSLATE ToWindows[] = 
{
    { 0x5C, "\xA5" },
    { 0x7E, "\xBB" },
    { 0x7F, "\xAB" },
    { 0xE1, "\xE4" },
    { 0xE2, "\xDF" },
    { 0xE4, "\xB5" },
    { 0xEC, "\xA2" },
    { 0xED, "\xA3" },
    { 0xEE, "\xF1" },
    { 0xEF, "\xF6" },
    { 0xF5, "\xFC" },
    { 0xFD, "\xF7" },
    { 0x00, NULL }
};

/*
 *  File handles for input
 */
FILE *InFile = NULL;
KCS_FILE *WaveFile = NULL;

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
enum { TRANS_NONE, TRANS_ASCII, TRANS_DOS, TRANS_WINDOWS } 
    TransMode = TRANS_NONE;

/*
 *  Header bytes
 */
struct _header {
    unsigned char size_l, size_h, type;
    char file_name[ 14 + 1 ];
} Header;

/*
 *  Block buffer
 */
unsigned char Block[ 64 ];
int BlockSize;

/*
 *  Layout of length block
 */
typedef struct _length_block 
{
    unsigned char length_h, length_l;
    unsigned char blocks_h, blocks_l;
    unsigned char last;
} LENGTH_BLOCK;

/*
 *  Program buffer
 */
unsigned char *Buffer = NULL;

/*
 *  Set by readBuffer after last block is read
 */
bool LastBlock = FALSE;


/*
 *  Output header information
 */
void printHeader( void )
{
    printf( "Program: %s (record size %d)\n\n", 
            Header.file_name,
            Header.size_l + Header.size_h * 256 );
}


/*
 *  read a single byte from the input file
 */
int readByte( void )
{
    if ( BinMode ) {
        return fgetc( InFile );
    }

    return kcsReadByte( WaveFile );
}


/*
 *  Read low level block
 *  Returns internal error code
 */
int readBlock( int length )
{
    int c, count;
    unsigned char check = 0;
    bool retry = FALSE;

#if _DEBUG
    printf( "readBlock(%d):", length );
#endif
    if ( length < 0 ) {
        retry = TRUE;
        length = -length;
    }
    
    BlockSize = -1;
    count = 0;

    while ( BlockSize < length ) {
        c = readByte();
        if ( c < 0 ) {
            /*
             *  Read error
             */
            return 1;            
        }
#if _DEBUG
        printf( "[%2.2X]", c );
#endif
        if ( c == KCS_LEAD_IN ) {
            if ( BlockSize != -1 ) {
                /*
                 *  Block ends too early
                 */
                return 4;
            }
        }
        else if ( BlockSize == -1 && c == 0xFF ) {
            /*
             *  Block starts here
             */
            count = 0;
            BlockSize = 0;
            check = 0;
        }
        else if ( BlockSize != -1 ) {
            /*
             *  Store byte
             */
            Block[ BlockSize++ ] = (unsigned char) c;
            check += (unsigned char) c;
        }
        else if ( ++count > 8 ) {
            /*
             *  Block not found
             */
            return 3;
        }
    }

    /*
     *  Compare check sum
     */
    c = readByte();
    if ( c < 0 ) {
        /*
         *  Read error
         */
        return 1;
    }
#if _DEBUG
        printf( "[%2.2X]", c );
#endif
#if _DEBUG
    printf( "Checksum: %X, BlockSize: %d\n", check, BlockSize );
#endif
    if ( (unsigned char) c != check ) {
        /*
         *  Bad check sum
         */
        if ( retry ) {
            return 2;
        }
        /*
         *  Try again
         */
        return readBlock( -length );
    }

    if ( retry ) {
        /*
         *  Retry attempt was successful
         */
#if _DEBUG
        printf( "Success on retry\n" );
#endif
        return 0;
    }

    /*
     *  Skip the duplicate
     */
    count = 0;
    do {
        c = readByte();
#if _DEBUG
        printf( "<%2.2X>", c );
#endif
        if ( c < 0 ) {
            /*
             *  Read error - ignore
             */
            continue;
        }
    } while ( c != 0xFF && count++ < 9 );

    count = 0;
    do {
        c = readByte();
#if _DEBUG
        printf( "<%2.2X>", c );
#endif
    } while ( count++ < BlockSize && c >= 0 );

#if _DEBUG
    putchar( '\n' );
#endif
    /*
     *  OK
     */
    return 0;
}


/*
 *  Read a logical Block
 */
int readBuffer( unsigned char *buffer, unsigned int *plength )
{
    int err;
    LENGTH_BLOCK *lp = (LENGTH_BLOCK *) Block;
    unsigned int length;
    unsigned int blocks;
    int l, last;

    /*
     *  Read length block first
     */
    err = readBlock( 6 );
    if ( err ) return err;
    
    length = lp->length_l + 256 * lp->length_h;
    blocks = lp->blocks_l + 256 * lp->blocks_h;
    last = lp->last;

    if ( length != blocks * 64 + last ) {
        /*
         *  Sizes do not properly match
         */
        return 5;
    }

    if ( plength != NULL ) {
        /*
         *  Return the true length to the caller
         */
        if ( *plength > 0 && length > *plength ) {
            /*
             *  Too long
             */
            return 5;
        }
        *plength = length;
    }

    /*
     *  Read all sub blocks
     */
    do {
        err = readBlock( l = ( blocks == 0 ? last : 64 ) );
        if ( err ) return err;

        memcpy( buffer, Block, l );
        buffer += l;
    }
    while ( blocks-- != 0 );

    /*
     *  Read the marker block (C1/C2)
     */
    err = readBlock( 6 );
    if ( err ) return err;

    LastBlock = Block[ 5 ] == 0xC1;
    return 0;
}


/*
 *  Print single char
 *  Handle ESC sequences and character translation
 */
void tiprintc( unsigned char c )
{
    char buffer[ 4 ];
    char *p = NULL;
    TRANSLATE *tp;
    struct _escapes *ep;

    if ( c == 0xff ) {
        /*
         *  Forced space after IMAGE and DATA
         */
        c = ' ';
    }
    else if ( TransMode != TRANS_NONE ) {
        /*
         *  Translate special characters
         */
        tp = TransMode == TRANS_ASCII ? ToAscii 
           : TransMode == TRANS_DOS   ? ToDos
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

    /*
     *  Output the converted character(s)
     */
    printf( "%s", p );
}


/*
 *  Print text or token
 *  Handle ESC sequences and character translation
 */
void tiprint( const char *buffer, int length )
{
    static bool space_pending = FALSE;
    static char last = '\0';
    bool handle_spaces = FALSE;
    char c;

    if ( length < 0 ) {
        length = (int) strlen( buffer );
        handle_spaces = TRUE;
    }
    while ( length-- ) {
        c = *buffer++;
        /*
         *  Avoid superfluous spaces
         */
        if ( space_pending ) {
            /*
             *  Output missing space if followed by char or digit
             */
            if ( isalnum( c ) ) {
                tiprintc( ' ' );
            }
            space_pending = FALSE;
            last = ' ';
        }
        if ( handle_spaces && c == ' ' ) {
            space_pending = last != ':';
            continue;
        }
        else {
            space_pending = FALSE;
        }
        last = c;
        tiprintc( (unsigned char) c );
    }
}


/*
 *  Parse and output a token
 */
int parse( unsigned char *buffer, int pos, int last )
{
    static char line[ 256 ];
    int c = buffer[ pos++ ];
    int len = -1;
    int exp, exp2;
    int i;
    int d;
    char *p;

#if _DEBUG_
    printf( "parse %2.2X %d %d\n", c, pos, last );
    fflush( stdout );
#endif

    switch ( c ) {

    case 0x00:
        /*
         *  end of statement
         *  check for ELSE or !
         */
        c = buffer[ pos ];
        if ( c != 0x9E && c != 0xA0 ) {
            p = ":";
        }
        else {
            p = " ";
        }
        break;

    case 0x80:
        /*
         *  extended token
         */
        c = buffer[ pos++ ];
        if ( c < 0x40 || c > 0x4F ) {
            /* 
             *  Invalid extended token
             */
            sprintf( line, "\\%02.2X", c );
            p = line;
            break;
        }
        p = ExtendedTokens[ c - 0x40 ];
        break;

    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
        /*
         *  BCD
         */
        len = c & 0x0F;
        if ( pos + len > last ) {
            /*
             *  Invalid number or line too short
             */
            sprintf( line, "\\%02.2X", c );
            p = line;
            len = -1;
            break;
        }
        exp = buffer[ pos++ ];
        if ( len == 2 && exp == 0 && buffer[ pos ] == 0 ) {
            /*
             *  special case 0.0
             */
            p = "0";
            ++pos;
            len = -1;
            break;
        }

        /*
         *  calculate exponent
         */
        exp -= 0x3F;
        if ( exp >= len + 2 || exp <= -2 ) {
            exp2 = 2 * exp - 1;
            exp = 9999;
        }
        else {
            exp2 = 0;
        }

        p = line;
        len -= 1;
        if ( exp <= 0 ) {
            /*
             *  nullfill to the left
             */
            *p++ = '0';
            if ( exp < 0 ) {
                *p++ = '.';
                for ( i = exp; i < 0; ++i ) {
                    *p++ = '0';
                    *p++ = '0';
                }
            }
        }
        for ( i = 0; i < len; ++i ) {
            /*
             *  BCD digits
             */
            if ( i == exp ) {
                *p++ = '.';
            }
            /*
             *  left digit
             */
            d = ( buffer[ pos ] & 0xF0 ) >> 4;
            if ( i != 0 || i >= exp || d != 0 ) {
                *p++ = d + '0';
                if ( exp == 9999 ) {
                    *p++ = '.';
                    exp = 0;
                }
            }
            /*
             *  right digit
             */
            d = buffer[ pos++ ] & 0x0F;
            if ( i != len - 1 || i < exp || d != 0 ) {
                *p++ = d + '0';
                if ( exp == 9999 ) {
                    *p++ = '.';
                    exp = 0;
                    --exp2;
                }
            }
        }
        for ( i = len; i < exp; ++i ) {
            /*
             *  nullfill to the right
             */
            *p++ = '0';
            *p++ = '0';
        }
        if ( exp2 != 0 ) {
            /*
             *  exponent if neccessary
             */
            sprintf( p, "E%+03d", exp2 );
        }
        else {
            *p = '\0';
        }
        p = line;
        len = -1;
        break;

    case 0xC9:
    case 0xCA:
        /*
         *  string with(out) quotes
         */
        len = buffer[ pos++ ];
        if ( pos + len > last ) {
            /* 
             *  Invalid string or line too short
             */
            sprintf( line, "\\%02.2X", c );
            p = line;
            break;
        }
        p = c == 0xCA ? line : line + 1;
        memcpy( p, buffer + pos, len );
        if ( p == line + 1 ) {
            p[ -1 ] = '"';
            p[ len ] = '"';
            p[ len + 1 ] = '\0';
            --p;
        }
        else {
            p[ len ] = '\0';
        }
        pos += len;
        len = (int) strlen( p );
        break;

    case 0xCB:
        /*
         *  short integer (line number)
         */
        i = buffer[ pos++ ];
        i += buffer[ pos++ ] * 256;
        sprintf( line, "%d", i );
        p = line;
        break;

    default:
        /*
         *  token or variable
         */
        p = Tokens[ c ];
    }

    /*
     *  Print
     */
    tiprint( p, len );

    return pos;
}


/*
 *  processing loop
 */
int process( unsigned char *buffer, int length )
{
    int lsb, msb;
    int len;
    int i;
    int lineno;
    char *p;
    unsigned char *buff_ptr = buffer;
    char temp[ 6 + 1 ];

#if _DEBUG
    printf( "Processing %d bytes\n", length );
#endif
    /*
     *  read header
     */
    if ( buffer[ 0 ] != 0x80 || buffer[ 1 ] != 0x03 ) {
        /*
         *  Assume printable text
         */
#if _DEBUG_
        printf( "Text line\n", length );
#endif
        tiprint( (const char *) buffer, length );
        putchar( '\n' );
        return 0;
    }

    /*
     *  look for variable area
     */
    buff_ptr += 2;
    lsb = *buff_ptr++; 
    msb = *buff_ptr;
    len = 256 * msb + lsb;
    buff_ptr = buffer + len + 2;
    if ( *buff_ptr++ != 0x86 || *buff_ptr++ != 0x00 ) {
        /*
         *  implicit last line with END statement not found
         */
#if _DEBUG
        printf( "Error 8 @%4.4X: %2.2X\n", len + 2, buffer[ len + 2 ] );
#endif
        return 8;
    }


    /*
     *  read in the variables
     */
    i = *buff_ptr++; /* highest variable token + 1 */
    while ( --i >= 0x20 ) {
        /*
         *  get variable name
         *  [len]Variablename, last byte with high bit set
         */
        len = *buff_ptr++;
    
        /*
         *  store variable name in token table
         */
        p = malloc( len + 1 );
        if ( p == NULL ) {
            perror( "Can't allocate variable" );
            return 1;
        }
        memcpy( p, buff_ptr, len );
        p[ len ] = '\0';
        p[ len - 1 ] &= 0x7f;
        Tokens[ i ] = p;
        buff_ptr += len;
#if _DEBUG
        printf( "%2.2X(%d): %.*s\n", i, len, len, p );
#endif
    }

    /*
     *  Go back to first line
     */
    buff_ptr = buffer + 4;

    /*
     *  read program lines
     */
    lineno = 0;
    while ( lineno != 0x7fff ) {
        /*
         *  lineno and length
         */
        lsb = *buff_ptr++;
        msb = *buff_ptr++;
        lineno = 256 * msb + lsb;
        if ( lineno == 0x7fff ) {
            /*
             *  done
             */
            continue;
        }
        sprintf( temp, "%d ", lineno );
        tiprint( temp, -1 );
        len = *buff_ptr++ - 2;
        if ( len < 0 || buff_ptr[ len ] != '\0' ) {
            /*
             *  Line terminator not correct
             */
            return 7;
        }

        /*
         *  parse line
         */
        for ( i = 0; i < len; ) {
            /*
             *  parse and output token
             */
            i = parse( buff_ptr, i, len );
        }
        buff_ptr += len + 1;

        /*
         *  End of line
         */
        tiprint( "\n", -1 );
    }
    return 0;
}


/*
 *  Main program
 */
int main( int argc, char *argv[] )
{
    int err;
    bool is_pgm_file = FALSE;
    FILE *raw = NULL;
    char *rawfile = NULL;
    unsigned int length;
    long count = 0;

    ++argv;
    --argc;

    while ( argc > 1 && **argv == '-' ) {

        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            BinMode = TRUE;
        }
        if ( strncmp( *argv, "-w", 2 ) == 0 ) {
            BinMode = FALSE;
        }
        if ( strncmp( *argv, "-r", 2 ) == 0 ) {
            rawfile = *argv + 2;
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
            case 'W':
            case '\0':
                TransMode = TRANS_WINDOWS; break;
            }
        }
        ++argv;
        --argc;
    }

    if ( argc < 1 ) {
        printf( "usage: list74 [-w|-b] -r<filename> -e[C|X] -c[A|D|W] "
                          "infile > outfile\n"
                "         -w reads a WAV file directly\n"
                "         -b reads a binary or *.PGM file\n"
                "         -r creates a raw binary (*.PGM) file\n"
                "         -e[C|X] use backslash escapes in strings\n"
                "           C: character escapes like '\\pi'\n"
                "           X: generic hexadecimal escapes (default)\n"
                "         -c[A|D|W] use charater set translation in strings\n"
                "           A: ASCII codes only (very simple)\n"
                "           D: DOS code page 437\n"
                "           W: Windows code page 1252 (default)\n"
                "         -c takes precedence over -e. Both can be mixed.\n"
              );
        return 2;
    }

    if ( !BinMode ) {
        /*
         *  Open WAV file
         */
        if ( ( WaveFile = kcsOpen( *argv, "rb", 1400, 8, 'N', 0 ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open wave file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    else {
        /*
         *  Open binary file
         */
        if ( ( InFile = fopen( *argv, "rb" ) ) == NULL ) {
            fprintf( stderr, "\nCannot open binary file %s\n", *argv );
            perror( "error" );
            return 2;
        }
        /*
         *  Check if the file starts with 80 03
         */
        if ( fgetc( InFile ) == 0x80 && fgetc( InFile ) == 0x03 ) {
            is_pgm_file = TRUE;
            rawfile = NULL;
        }
        fseek( InFile, 0, SEEK_SET );
    }

    if ( rawfile != NULL ) {
        /*
         *  Open raw (*.PGM) output file
         */
        if ( ( raw = fopen( rawfile, "wb" ) ) == NULL ) {
            fprintf( stderr, "\nCannot open output file %s\n", *argv );
        }
    }

    if ( !is_pgm_file ) {
        /*
         *  Read header block
         */
        length = sizeof( Header ) - 1;
        err = readBuffer( (unsigned char *) &Header, &length );
        if ( err ) {
            fprintf( stderr, "%s", ErrorMsg[ err ] );
            return 2;
        }
        printHeader();

        /*
         *  Allocate the buffer
         */
        length = Header.size_l + Header.size_h * 256;
        Buffer = malloc( length );
        if ( Buffer == NULL ) {
            perror( "Buffer allocation failed" );
            return 3;
        }

        /*
         *  Read data block(s)
         */
        while ( !LastBlock ) {
            length = Header.size_l + Header.size_h * 256;
            err = readBuffer( Buffer, &length );
            if ( err ) {
                fprintf( stderr, "%s", ErrorMsg[ err ] );
                return 2;
            }
            count += length;
            if ( raw != NULL ) {
                if ( 1 != fwrite( Buffer, length, 1, raw ) ) {
                    perror( "write raw" );
                    return 2;
                }
            }
            else {
                err = process( Buffer, length );
                if ( err ) {
                    fprintf( stderr, "%s", ErrorMsg[ err ] );
                    return 2;
                }
            }
        }
        if ( raw != NULL ) {
            printf( "%ld bytes written to %s\n", count, rawfile );
        }
    }
    else {
        /*
         *  PGM-File from PC interface
         */
        fseek( InFile, 0, SEEK_END );
        length = (int) ftell( InFile );

        /*
         *  Allocate the buffer
         */
        Buffer = malloc( length );
        if ( Buffer == NULL ) {
            perror( "Buffer allocation failed" );
            return 3;
        }
        fseek( InFile, 0, SEEK_SET );
        if ( length != fread( Buffer, 1, length, InFile ) ) {
            perror( "Read error" );
            return 3;
        }
        err = process( Buffer, length );
        if ( err ) {
            fprintf( stderr, "%s", ErrorMsg[ err ] );
            return 2;
        }
    } 

    if ( !BinMode ) {
        kcsClose( WaveFile );
    }
    else {
        fclose( InFile );
    }

    return 0;
}

