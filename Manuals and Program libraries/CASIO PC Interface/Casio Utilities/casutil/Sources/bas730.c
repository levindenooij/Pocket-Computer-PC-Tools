/*
 *  bas730.c - Create Casio tape images from BASIC source
 *
 *  This program creates a binary tape image file for the Casio
 *  Casio calculators PB-100/220/300, FX-720P, FX-730P, FX-795P, etc.
 *
 *  Options:
 *    -b create BIN file to be converted by wave730 later (Default)
 *    -a create ASCII file (Piotr's format)
 *    -w create a WAV file
 *    -o allow old style FX-702P keywords 
 *    -n allow new style FX-850P keywords
 *    -l convert LGT/LOG to LOG/LN
 *    -1 try to make PB-100/FX-700P compatible 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"

#define LEAD_IN_TIME 20         /* tens of a second */
#define LEAD_IN_TIME_FIRST 20   /* tens of a second */
#define LEAD_IN_ASCII 64        /* Number of groups of six '1' bits */

#define HEADER_WITH_LINE_NO 1   /* Piotr's assumption */

/*
 *  Error messages
 */
const char *err_msg[] = {
    "",                                       /* OK */
    "No line numbers found",                  /* message #1 */
    "Write error",                            /* message #2 */
    "Line too long",                          /* message #3 */
    "Line number too large"                   /* message #4 */
    "Not implemented"                         /* message #5 */
};

/*
 *  Character codes 0x00 to 0x7F 
 */
const unsigned char characters[] = 
{
  0x00, 0x06, 0x07, 0x08, 0x09, 0x70, 0x74, 0x76,  /*   ! " # $ % & ' */
  0x1D, 0x1C, 0x03, 0x01, 0x5D, 0x02, 0x1A, 0x04,  /* ( ) * + , - . / */
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  /* 0 1 2 3 4 5 6 7 */
  0x18, 0x19, 0x5F, 0x5E, 0x0E, 0x0C, 0x0A, 0x5C,  /* 8 9 : ; < = > ? */
  0x64, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,  /* @ A B C D E F G */
  0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,  /* H I J K L M N O */
  0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,  /* P Q R S T U V W */
  0x37, 0x38, 0x39, 0x73, 0x7A, 0x78, 0x05, 0x75,  /* X Y Z [ \ ] ^ _ */
  0x62, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,  /* ` a b c d e f g */
  0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,  /* h i j k l m n o */
  0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,  /* p q r s t u v w */
  0x57, 0x58, 0x59, 0x68, 0x6E, 0x6F, 0x3D, 0x00,  /* x y z { | } ~   */
};

/*
 *  Replacement for some special characters
 *  Only outside strings and comments
 */
struct _specials {
    char *text;
    unsigned char token;
} specials[] = 
{
    { ">=", 0x0B },
    { "=>", 0x0B },
    { "<=", 0x0D },
    { "=<", 0x0D },
    { "!=", 0x0F },
    { "<>", 0x0F },
    { "PI", 0x1B },
    { NULL, 0    }
};

/*
 *  Escape codes with \, work everywhere
 */
struct _specials escapes[] = 
{
    { ">=", 0x0B },  /* >= */
    { "<=", 0x0D },  /* <= */
    { "<>", 0x0F },  /* <> */
    { "PI", 0x1B },  /* PI */
    { "E-", 0x1E },  /* E-exponent */
    { "E+", 0x1F },  /* E exponent */
    { "SD", 0x3A },  /* Small D */
    { "SL", 0x3B },  /* Small L */
    { "GA", 0x3C },  /* gamma */
    { "SI", 0x3E },  /* sigma */
    { "-1", 0x3F },  /* Small -1 */
    { "`",  0x5A },  /* Quote begin */
    { "'",  0x5B },  /* Quote end */
    { "@",  0x60 },  /* circle */
    { "SM", 0x61 },  /* Sigma, sum */
    { "DG", 0x62 },  /* degree */
    { "^",  0x63 },  /* triangle */
    { "*",  0x65 },  /* multiply */
    { ":",  0x66 },  /* divide */
    { "SP", 0x67 },  /* spade */
    { "HT", 0x69 },  /* heart */
    { "DI", 0x6A },  /* diamond */
    { "CL", 0x6B },  /* club */
    { "MU", 0x6C },  /* Mu */
    { "OM", 0x6D },  /* Omega */
    { "YN", 0x71 },  /* Yen */
    { "SQ", 0x72 },  /* sQuare */
    { ".",  0x77 },  /* dot */
    { "#",  0x79 },  /* block */
    { "\\", 0x7A },  /* backslash */
    { "]",  0x7B },  /* grey block */
    { "/",  0x7C },  /* thick slash */
    { "B>", 0x7D },  /* b> ? */
    { "TA", 0x7E },  /* tau */
    { NULL, 0    }
};


/*
 *  BASIC keywords 0x80 to 0xD1 
 */
const char *tokens[] = {
  "SIN",      "COS",      "TAN",      "ASN",     /* 80 - 83 */
  "ACS",      "ATN",      "LOG",      "LN",      /* 84 - 87 */
  "EXP",      "SQR",      "INT",      "FRAC",    /* 88 - 8B */
  "ABS",      "SGN",      "RND(",     "RAN#",    /* 8C - 8F */
  "LEN(",     "VAL(",     "MID(",     "KEY",     /* 90 - 93 */
  "CSR",      "TO",       "STEP",     "THEN",    /* 94 - 97 */
  "DEG(",     "HEX$(",    "STR$(",    "DM$(",    /* 98 - 9B */
  "MID$(",    "ALL",      "BEEP",     "LET",     /* 9C - 9F */
  "FOR",      "NEXT",     "GOTO",     "GOSUB",   /* A0 - A3 */
  "RETURN",   "IF",       "PRINT",    "INPUT",   /* A4 - A7 */
  "MODE",     "STOP",     "END",      "ON",      /* A8 - AB */
  "READ",     "RESTORE",  "DATA",     "REM",     /* AC - AF */
  "VAC",      "SET",      "PUT",      "GET",     /* B0 - B3 */
  "CLEAR",    "\\B5",     "\\B6",     "\\B7",    /* B4 - B7 */
  "POL(",     "HYP",      "REC(",     "FACT",    /* B8 - BB */
  "NPR(",     "EOX",      "NCR(",     "EOY",     /* BC - BF */
  "DEFM",     "SAVE",     "LOAD",     "VERIFY",  /* C0 - C3 */
  "LIST",     "RUN",      "NEW",      "PASS",    /* C4 - C7 */
  "SRAM",     "LRAM",     "WRITE#",   "STAT",    /* C8 - CB */
  "\\CC",     "\\CD",     "\\CE",     "\\CF",    /* CC - CF */
  "DIM",      "ERASE",    NULL                   /* D0 - D1 */
};

/*
 *  BASIC keywords 0x80 to 0xC6 for PB-100
 */
const char *tokens100[] = {
  "SIN",      "COS",      "TAN",      "ASN",     /* 80 - 83 */
  "ACS",      "ATN",      "LOG",      "LN",      /* 84 - 87 */
  "EXP",      "SQR",      "INT",      "FRAC",    /* 88 - 8B */
  "ABS",      "SGN",      "RND(",     "RAN#",    /* 8C - 8F */
  "LEN(",     "VAL(",     "MID(",     "KEY",     /* 90 - 93 */
  "CSR",      "TO",       "STEP",     "THEN",    /* 94 - 97 */
  "\\98",     "\\99",     "\\9A",     "\\9B",    /* 98 - 9B */
  "\\9C",     "\\9D",     "\\9E",     "\\9F",    /* 9C - 9F */
  "FOR",      "NEXT",     "GOTO",     "GOSUB",   /* A0 - A3 */
  "RETURN",   "IF",       "PRINT",    "INPUT",   /* A4 - A7 */
  "MODE",     "STOP",     "END",      "\\AB",    /* A8 - AB */
  "\\AC",     "\\AD",     "\\AE",     "\\AF",    /* 9C - 9F */
  "VAC",      "SET",      "PUT",      "GET",     /* B0 - B3 */
  "\\B4",     "\\B5",     "\\B6",     "\\B7",    /* B4 - B7 */
  "\\B8",     "\\B9",     "\\BA",     "\\BB",    /* B8 - BB */
  "\\BC",     "\\BD",     "\\BE",     "\\BF",    /* BC - BF */
  "DEFM",     "SAVE",     "LOAD",     "VER",     /* C0 - C3 */
  "LIST",     "RUN",      "CLEAR",    NULL,      /* C4 - C6 */
};

/*
 *  FX-702P translation table
 */
struct _translate {
    char *from;
    char *to;
} translation_702[] = 
{
    { "CLR",     "NEW"       },
    { "KEY$",    "KEY$"      },
    { "KEY",     "KEY$"      },
    { "MID$",    "MID$"      },
    { "MID",     "MID$"      },
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
    { "RPC",     "POL"       },
    { "PRC",     "REC"       },
    { NULL,      NULL        }
};

/*
 *  FX-850P translation table
 */
struct _translate translation_850[] = 
{
    { "TAB",    "CSR"    },
    { "INKEY$", "KEY$"   },
    { "ROUND",  "RND"    },
    { "ANGLE",  "MODE4+" },
    { NULL,     NULL     }
};

/*
 *  PB-100 translation table
 */
struct _translate translation_100[] = 
{
    { "NEW",    "CLEAR"  },
    { "CLEAR",  "VAC"    },
    { "KEY$",   "KEY"    },
    { "MID$",   "MID",   },
    { "THEN0",  "THEN0"  },
    { "THEN1",  "THEN1"  },
    { "THEN2",  "THEN2"  },
    { "THEN3",  "THEN3"  },
    { "THEN4",  "THEN4"  },
    { "THEN5",  "THEN5"  },
    { "THEN6",  "THEN6"  },
    { "THEN7",  "THEN7"  },
    { "THEN8",  "THEN8"  },
    { "THEN9",  "THEN9"  },
    { "THEN",   ";"      },
    { "VERIFY", "VER"    },
    { "DEFM",   ""       },
    { "DIM",    ""       },
    { "BEEP",   ""       },
    { "RESTORE",""       },
    { "READ",   "INPUT"  },
    { "STAT",   ""       },
    { "DATA",   NULL     },
    { "REM",    NULL     },
    { "'",      NULL     },
    { NULL,     NULL     }
};

/*
 *  Logarithm translation tables
 */
struct _translate translation_log[] = 
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
enum { MODE_BIN, MODE_ASCII, MODE_WAVE } OutputMode = MODE_BIN;
bool Translate702 = FALSE;
bool Translate850 = FALSE;
bool OutputPB100 = FALSE;
bool TranslateLog = FALSE;

/*
 *  Header bytes
 */
struct _header {
    unsigned char segment_id;
    unsigned char file_name[ 8 ];
    unsigned char line_no[ 2 ];
} Header;

#define HEADER_SIZE  11
#define TYPE_PROGRAM 0xD0
#define TYPE_ALL     0xF0
#define TYPE_DATA    0xE0

#define DATA_SIZE 64
/*
 *  Data segment
 */
static struct _data {
    char segment_id;
    char data[ DATA_SIZE ];
    char delimiter;
} Data;


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
    int line_number, l, i;
    bool in_string, transparent;
    struct _translate *tp;
    struct _specials *sp;
    
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

    /*
     *  Keyword translation needs a second pass
     *  Thus, store intermediate result in temp buffer if neccessary
     */
    q = Translate702 || Translate850 ? temp : buffer;

    for ( p = temp; *p != '\0'; *q++ = *p++ ) {

        /*
         *  Check for escape sequence (force uppercase!)
         */
        if ( *p == '\\' && isalpha( p[ 1 ] ) ) {
            p[ 1 ] = toupper( p[ 1 ] );
            p[ 2 ] = toupper( p[ 2 ] );
        }

        if ( !in_string && !transparent && 
             ( q[ -1 ] == '\'' 
               || strncmp( q - 4, "DATA", 4 ) == 0
               || strncmp( q - 3, "REM" , 3 ) == 0 ) )
        {
            transparent = TRUE;
            continue;
        }
        /*
         *  Remove spaces
         */
        while ( !in_string && !transparent && isspace( *p ) ) {
            ++p;
        }
        if ( *p == '"' ) {
            in_string = !in_string;
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

    if ( Translate702 || Translate850 ) {
        /*
         *  Translate old or new style keywords
         */
        /* printf( "%s\n", temp ); */

        transparent = FALSE;
        in_string = FALSE;

        /*
         *  Translate line
         */
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
                 || strncmp( p - 4, "DATA", 4 ) == 0
                 || strncmp( p - 3, "REM" , 3 ) == 0 )
            {
                transparent = TRUE;
                continue;
            }
            tp = Translate702 ? translation_702 : translation_850;

            for ( ; tp->from != NULL; ++tp ) {

                l = strlen( tp->from );
                if ( 0 == strncmp( tp->from, p, l ) ) {
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
            if ( Translate702 && *p == '!' && p != temp ) {
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
        *q = '\0';
    }

    if ( TranslateLog ) {
        /*
         *  Handle LGT/LOG to LOG/LN conversion
         *  Because the line connot get longer we do it in place
         */
        for ( p = q = buffer; *p != '\0'; *q++ = *p++ ) {

            for ( tp = translation_log; tp->from != NULL; ++tp ) {

                l = strlen( tp->from );
                if ( 0 == strncmp( tp->from, p, l ) ) {
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

    if ( OutputPB100 ) {
        /*
         *  Try to make PB-100 compliant
         */
        strcpy( temp, buffer );

        in_string = FALSE;
        transparent = FALSE;

        for ( p = temp, q = buffer; *p != '\0'; q++, p++ ) {
            /*
             *  Copy char, may be overwritten later
             */
            *q = *p;

            if ( *p == '"' ) {
                in_string = !in_string;
            }
            if ( transparent ) {
                /*
                 *  Look for colon outside string
                 */
                if ( !in_string && *p == ':' ) {
                    /*
                     *  End of statement detected
                     */
                    transparent = FALSE;
                    --q;
                    continue;
                }
                /*
                 *  Ignore character
                 */
                --q;
                continue;
            }
            if ( in_string ) {
                /*
                 *  Transparent copy within strings
                 */
                continue;
            }
            /*
             *  Check for keywords to translate
             */
            for ( tp = translation_100; tp->from != NULL; ++tp ) {
                l = strlen( tp->from );
                if ( 0 == strncmp( tp->from, p, l ) ) {
                    break;
                }
            }
            if ( tp->from != NULL ) {
                /*
                 *  Keyword found
                 */
                p += l - 1;

                if ( tp->to == NULL ) {
                    /*
                     *  Remove until end of line
                     */
                    if ( *(--q) != ':' ) {
                        /*
                         *  Remove line completely
                         */
                        return 0;
                    }
                    /*
                     *  Skip rest of line
                     */
                    break;
                }
                if ( *(tp->to) == '\0' ) {
                    /*
                     *  Skip until end of line or colon
                     */
                    transparent = TRUE;
                    --q;
                    continue;
                }
                /*
                 *  Replace keyword
                 */
                l = strlen( tp->to );
                memcpy( q, tp->to, l );
                q += l - 1;
            }
        }
        /*
         *  Removed statements might have left stray colons at end of line
         *  or might have left an empty line
         */
        while ( q[ -1 ] == ':' ) {
            --q;
        }
        *q = '\0';

        if ( *buffer == '\0' ) {
            return 0;
        }
    }
    printf( "%4d %s\n", line_number, buffer );

    return line_number;
}


/*
 *  Encode (tokenize) a program line
 */
int encode( long line_number, char *line, unsigned char *buffer )
{
    unsigned int i, l, tl;
    long ln = line_number;
    const char **tp;
    struct _specials *sp;
    bool transparent, in_string;
    unsigned char *buff = buffer;

    /*
     *  Convert line number to BCD
     */
    l = 0;
    for ( i = 0; i < 16; i += 4 ) {
        l |= (int)( ln % 10 ) << i;
        ln /= 10;
    }
    *buff++ = l & 0xff;
    *buff++ = l >> 8;
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

    while ( *line != '\0' && l < DATA_SIZE ) {
        /*
         *  Escape sequence \nn
         */
        if ( *line == '\\' ) {
            /*
             *  look for abreviation
             */
            ++line;
            for ( sp = escapes; sp->text != NULL; ++sp ) {
                tl = strlen( sp->text );
                if ( 0 == strncmp( line, sp->text, tl ) ) {
                    break;
                }
            }
            if ( sp->text != NULL ) {
                /*
                 *  Replace string by token value
                 */
                line += tl;
                *buff++ = sp->token;
            }
            else {
                /*
                 *  hex value
                 */
                sscanf( line, "%2x", &i );
                line += 2;
                *buff++ = i;
            }
            ++l;
            continue;
        }

        if ( l > 2 && buff[ -1 ] == 0x07 ) {
            /*
             *  '"'
             */
            in_string = !in_string;
        }
        if ( !in_string && !transparent ) {

            if ( *line == ':' ) {
                /*
                 *  Statement delimiter
                 */
                *buff++ = 0xFE;
                ++l;
                ++line;
                continue;
            }
            if ( *line == '\'' ) {
                /*
                 *  Replace ' by :REM
                 */
                if ( l > 2 && buff[ -1 ] != 0xFE ) {
                    *buff++ = 0xFE;
                    ++l;
                }
                *buff++ = 0xAF;
                ++l;
                ++line;
                transparent = TRUE;
                continue;
            }
            if ( isupper( *line ) ) {
                /*
                 *  Look if a token is matched
                 */
                tp = OutputPB100 ? tokens100 : tokens;
                for ( i = 0x80; *tp != NULL; ++tp, ++i ) {
                    if ( 0 == strncmp( line, *tp, tl = strlen( *tp ) ) ) {
                        break;
                    }
                }
                if ( *tp != NULL ) {
                    /*
                     *  Replace string by token value
                     */
                    if ( i == 0xAE || i == 0xAF ) {
                        /*
                         *  DATA or REM
                         */
                        transparent = TRUE;
                    }
                    line += tl;
                    *buff++ = (unsigned char) i;
                    ++l;
                    continue;
                }
                /*
                 *  E and E- in numbers
                 */
                if ( l > 2 && *line == 'E' 
                     && buff[ -1 ] <= 0x1A && buff[ -1 ] >= 0x10 ) 
                {
                    if ( line[ 1 ] == '-' ) {
                        *buff++ = 0x1E;
                        ++line;
                    }
                    else {
                        *buff++ = 0x1F;
                    }
                    ++l;
                    ++line;
                    continue;
                }
            }

            /*
             *  Replace special characters
             */
            for ( sp = specials; sp->text != NULL; ++sp ) {
                if ( 0 == strncmp( line, sp->text, tl = strlen( sp->text ) ) ) {
                    break;
                }
            }
            if ( sp->text != NULL ) {
                /*
                 *  Replace string by token value
                 */
                line += tl;
                *buff++ = sp->token;
                ++l;
                continue;
            }
        }
        /*
         *  Translate single character 
         */
        i = (unsigned char) *line++;
        if ( i >= 0x7F || i < 32 ) {
            switch ( i ) {
            case 0xB5: 
                *buff = 0x6C; break; /* mu */
            case 0xB0: case 0xBA:
                *buff = 0x62; break; /* degree */
            default:
                *buff = 0x77;
            }
        }
        else {
            *buff = characters[ i - 32 ];
        }
        ++buff;
        ++l;
    }
    /*
     *  End of line delimiter
     */
    *buff = 0xFF;
    ++l;
    return l;
}


/*
 *  Main program
 */
int main( int argc, char *argv[] )
{
    int err;
    int i, l, linecount;
    long line_number;
    char *p, *buff, c;
    char line[ 256 ];
    unsigned char buffer[ DATA_SIZE + 1 ];

    ++argv;
    --argc;

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
        if ( strncmp( *argv, "-w", 2 ) == 0 ) {
            OutputMode = MODE_WAVE;
        }
        if ( strncmp( *argv, "-1", 2 ) == 0 ) {
            OutputPB100 = TRUE;
        }
        if ( strncmp( *argv, "-o", 2 ) == 0 ) {
            Translate702 = TRUE;
            Translate850 = FALSE;
        }
        if ( strncmp( *argv, "-n", 2 ) == 0 ) {
            Translate850 = TRUE;
            Translate702 = FALSE;
        }
        if ( strncmp( *argv, "-l", 2 ) == 0 ) {
            TranslateLog = TRUE;
        }
        ++argv;
        --argc;
    }

    if ( argc < 1 ) {
        printf( "usage: bas730 <options> "
                        "infile outfile\n"
                "       -a create an ASCII encoded file for Piotr's interface\n"
                "       -b create a binary file\n"
                "       -w create a WAV file\n"
                "       -o allow old style FX-702P keywords\n" 
                "       -n allow new style FX-850P keywords\n" 
                "       -l convert LGT/LOG to LOG/LN\n"
                "       -1 target for PB-100/FX701P\n" 
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
        WaveOut = kcsOpen( *argv, "wb", 300, 8, 'E', 2 );
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
    }

    /*
     *  Create file header
     */
    Header.segment_id = TYPE_PROGRAM;

    /*
     *  Filename = basename of output file
     */
    memset( Header.file_name, 0, 8 );
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
        Header.file_name[ i ] = characters[ (unsigned char) *p - 32 ];
        ++p;
        ++i;
    }
    Header.segment_id |= i;

    /*
     *  Create the program
     */
    linecount = 0;
    
    Data.segment_id = 0x02;
    buff = Data.data;
    err = 0;

    /*
     *  Process the input file
     */
    while ( err == 0 ) {
        /*
         *  read the text input file line by line
         */
        l = line_number;
        line_number = read_line( line, &err );
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
        if ( line_number > 9999 ) {
            /*
             *  too large
             */
            err = 4;
            break;
        }
        /*
         *  encode the line
         */
        l = encode( line_number, line, buffer );
        if ( l == 0 ) {
            /*
             *  line removed by encoder
             */
            continue;
        }
        if ( l > DATA_SIZE ) {
            /*
             *  too long
             */
            err = 3;
            break;
        }
        if ( ++linecount == 1 ) {
#if HEADER_WITH_LINE_NO
            /*
             *  Dump the header with the line number of the first line
             *  This is Piotr's assumption which does not hold for FX-730P
             */
            memcpy( Header.line_no, buffer, 2 );
#else
            Header.line_no[ 0 ] = 0xD7;
            Header.line_no[ 1 ] = 0x01;
#endif
            err = output_segment( &Header, HEADER_SIZE );
            if ( err != 0 ) {
                break;
            }
        }
        /*
         *  check if encoded line fits in segment
         */
        i = buff - Data.data;
        if ( l + i > DATA_SIZE ) {
            /*
             *  Flush segment
             */
            *buff++ = 0xF1;
            err = output_segment( &Data, i + 2 );
            if ( err != 0 ) {
                break;
            }
            buff = Data.data;
        }
        /*
         *  Copy encoded line to end of buffer
         */
        memcpy( buff, buffer, l );
        buff += l;
    }
    if ( err == 0 ) {
        /*
         *  Flush last segment
         */
        *buff++ = 0xF0;
        i = buff - Data.data;
        err = output_segment( &Data, 1 + i );
    }
    if ( err == 0 ) {
        /*
         *  0x00 terminates the recording
         */
        err = output( 0x00 );
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

