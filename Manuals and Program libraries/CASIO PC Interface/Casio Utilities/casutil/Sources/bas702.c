/*
 *  bas702.c - Create Casio tape images from BASIC source
 *
 *  This program creates a binary tape image file for the Casio
 *  Casio calculator FX-702P
 *
 *  Options:
 *    -b create BIN file to be converted by wave730 later (Default)
 *    -a create ASCII file (Piotr's format)
 *    -w create a WAV file
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include"bool.h"
#include"wave.h"

#define LEAD_IN_TIME 15         /* tens of a second */
#define LEAD_IN_TIME_FIRST 15   /* tens of a second */
#define LEAD_IN_ASCII 64        /* Number of groups of six '1' bits */

/*
 *  Error messages
 */
const char *err_msg[] = {
    "",                                       /* OK */
    "No line numbers found",                  /* message #1 */
    "Write error",                            /* message #2 */
    "Line too long",                          /* message #3 */
    "Line number too large"                   /* message #4 */
};

/*
 *  Character codes 0x10 to 0x5F 
 */
const unsigned char characters[] = 
{
    0x0F, 0x26, 0x14, 0x15, 0x16, 0x12, 0x20, 0x13,  /*   ! " # $ % & ' */
    0x2C, 0x28, 0x22, 0x20, 0x19, 0x21, 0x3A, 0x23,  /* ( ) * + , - . / */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  /* 0 1 2 3 4 5 6 7 */
    0x38, 0x39, 0x18, 0x17, 0x1E, 0x1C, 0x1A, 0x11,  /* 8 9 : ; < = > ? */
    0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,  /* @ A B C D E F G */
    0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,  /* H I J K L M N O */
    0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,  /* P Q R S T U V W */
    0x57, 0x58, 0x59, 0x2C, 0x23, 0x28, 0x24, 0x5E,  /* X Y Z [ \ ] ^ _ */
    0x12, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,  /* ` a b c d e f g */
    0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,  /* h i j k l m n o */
    0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,  /* p q r s t u v w */
    0x57, 0x58, 0x59, 0x1D, 0x1F, 0x1B, 0x3B, 0x5E   /* x y z { | } ~   */
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
    { ">=", 0x1B },
    { "=>", 0x1B },
    { "<=", 0x1D },
    { "=<", 0x1D },
    { "!=", 0x1F },
    { "<>", 0x1F },
    { "PI", 0x3B },
    { NULL, 0    }
};

/*
 *  Escape codes with \, work everywhere
 */
struct _specials escapes[] = 
{
    { ">=", 0x1B },  /* >= */
    { "<=", 0x1D },  /* <= */
    { "<>", 0x1F },  /* <> */
    { "PI", 0x3B },  /* PI */
    { "E+", 0x3F },  /* E exponent */
    { "DG", 0x12 },  /* degree */
    { NULL, 0   }
};


/*
 *  BASIC keywords 0x60 to 0xAF
 */
const char *Tokens[] =
{
    "CNT",    "SX",     "SY",     "SX2",      /* 60 - 63 */
    "SY2",    "SXY",    "RAN#",   "MX",       /* 64 - 67 */
    "MY",     "SDX",    "SDXN",   "SDY",      /* 68 - 6B */
    "SDYN",   "LRA",    "LRB",    "COR",      /* 6C - 6F */
    "SIN",    "COS",    "TAN",    "ASN",      /* 70 - 73 */
    "ACS",    "ATN",    "HSN",    "HCS",      /* 74 - 77 */
    "HTN",    "AHS",    "AHC",    "AHT",      /* 78 - 7B */
    "LOG",    "LN",     "SQR",    "EXP",      /* 7C - 7F */
    "ABS",    "SGN",    "INT",    "FRAC",     /* 80 - 83 */
    "EOX",    "EOY",    "CSR",    "TO",       /* 84 - 87 */
    "STEP",   "THEN",   "KEY",    "MID(",     /* 88 - 8B */
    "LEN(",   "RND(",   "DEG(",   "???",      /* 8C - 8F */
    "FOR",    "NEXT",   "GOTO",   "GSB",      /* 90 - 93 */
    "RET",    "INP",    "PRT",    "WAIT",     /* 94 - 97 */
    "MODE",   "STOP",   "END",    "IF",       /* 98 - 9B */
    "STAT",   "DEL",    "SAC",    "ALL",      /* 9C - 9F */
    "ROM",    "DMS",    "RPC",    "PRC",      /* A0 - A3 */
    "SET",    "VAC",    "PUT",    "GET",      /* A4 - A7 */
    "LOAD",   "SAVE",   "VER",    "DEFM",     /* A8 - AB */
    "PASS",   "LIST",   "RUN",    "CLR",      /* AC - AF */
    NULL
};

/*
 *  New style keyword translation table
 */
struct _translate {
    char *from;
    char *to;
} translation_table[] = 
{
    { "NEW",       "CLR"    },
    { "CLEAR",     "VAC"    },
    { "INKEY$",    "KEY"    },
    { "KEY$",      "KEY"    },
    { "MID$",      "MID"    },
    { "RETURN",    "RET"    },
    { "GOSUB",     "GSB"    },
    { "PRINT",     "PRT"    },
    { "INPUT",     "INP"    },
    { "TAB",       "CSR"    },
    { "ROUND",     "RND"    },
    { "ANGLE",     "MODE4+" },
    { "PRINTDMS$", "DMS"    },
    { "HYPSIN",    "HSN"    },
    { "HYPCOS",    "HCS"    },
    { "HYPTAN",    "HTN"    },
    { "HYPASN",    "AHS"    },
    { "HYPACS",    "AHC"    },
    { "HYPATN",    "AHT"    },
    { "STATCLEAR", "SAC"    },
    { "SUMX",      "SX"     },
    { "SUMY",      "SY"     },
    { "SUMX2",     "SX2"    },
    { "SUMY2",     "SY2"    },
    { "SUMXY",     "SXY"    },
    { "POL",       "RPC"    },
    { "REC",       "PRC"    },
    { "THEN0",     "THEN0"  },
    { "THEN1",     "THEN1"  },
    { "THEN2",     "THEN2"  },
    { "THEN3",     "THEN3"  },
    { "THEN4",     "THEN4"  },
    { "THEN5",     "THEN5"  },
    { "THEN6",     "THEN6"  },
    { "THEN7",     "THEN7"  },
    { "THEN8",     "THEN8"  },
    { "THEN9",     "THEN9"  },
    { "THEN",      ";"      },
    { "VERIFY",    "VER"    },
    { "DEFM",      ""       },
    { "DIM",       ""       },
    { "Z(",        "A("     },
    { "BEEP",      ""       },
    { "RESTORE",   ""       },
    { "READ",      "INPUT"  },
    { "CLS",       ""       },
    { "DATA",      NULL     },
    { "REM",       NULL     },
    { "'",         NULL     },
    { "FACT",      "!"      },
    { NULL,        NULL     }
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
 *  Output mode and flags
 */
enum { MODE_BIN, MODE_ASCII, MODE_WAVE } OutputMode = MODE_BIN;
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
 *  Read a line from input file and do some preliminary syntax checks
 *  Returns the line number in binary or 0 (ignore) or EOF (error)
 */
long read_line( char *buffer, int *err )
{ 
    char temp_buffer[ 256 ];
    char *temp = temp_buffer;
    char *p, *q;
    int line_number, l, i;
    bool in_string, ignore;
    int plevel[ 5 ], fact;
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
     *  Uppercase everything
     *  Remove spaces
     */
    ignore = FALSE;
    in_string = FALSE;

    /*
     *  Keyword translation needs a second pass
     *  Thus, store intermediate result in temp buffer
     */
    for ( p = q = temp; *p != '\0'; *q++ = *p++ ) {

        if ( !in_string && !ignore && 
             ( q[ -1 ] == '\'' 
               || strncmp( q - 4, "DATA", 4 ) == 0
               || strncmp( q - 3, "REM" , 3 ) == 0 ) )
        {
            /*
             *  Both statements will be removed later 
             *  but handle them here anyway
             */
            ignore = TRUE;
            continue;
        }

        /*
         *  Remove spaces
         */
        while ( !in_string && !ignore && isspace( *p ) ) {
            ++p;
        }
        if ( *p == '"' ) {
            in_string = !in_string;
        }
        /*
         *  Make uppercase
         */
        *p = toupper( *p );
    }
    *q = '\0';

    /*
     *  Translate new style keywords
     */
    ignore = FALSE;
    in_string = FALSE;
    fact = 0;
    memset( plevel, 0, sizeof( plevel ) );
    
    for ( p = temp, q = buffer; *p != '\0'; q++, p++ ) {
        /*
         *  Copy char, may be overwritten later
         */
        *q = *p;

        if ( *p == '"' ) {
            in_string = !in_string;
        }
        if ( ignore ) {
            /*
             *  Look for colon outside string
             */
            if ( !in_string && *p == ':' ) {
                /*
                 *  End of statement detected
                 */
                ignore = FALSE;
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
        for ( tp = translation_table; tp->from != NULL; ++tp ) {
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
                ignore = TRUE;
                --q;
                continue;
            }
            if ( 0 == strcmp( tp->to, "!" ) ) {
                /*
                 *  Must be moved behind the expression!
                 */
                --q;
                fact += fact >= 5 ? 0 : 1;
                continue;
            }

            /*
             *  Replace keyword
             */
            l = strlen( tp->to );
            memcpy( q, tp->to, l );
            q += l - 1;
        }

        if ( fact != 0 ) {
            if ( *p == '(' ) {
                ++plevel[ fact - 1 ];
                continue;
            }
            if ( *p == ')' && plevel[ fact - 1 ] > 0 ) {
                if ( --plevel[ fact - 1 ] == 0 ) {
                    /*
                     *  Insert '!' after closing parenthesis
                     */
                    --fact;
                    *++q = '!';
                }
                continue;
            }
            if ( plevel[ fact - 1 ] > 0 || isalnum( *p ) ) {
                continue;
            }
            /*
             *  Insert '!' before current character
             */
            --fact;
            *q++ = '!';
            *q = *p;
            continue;
        }
    }
    while ( fact-- ) {
        *q++ = '!';
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
    bool in_string;
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

        if ( l > 2 && buff[ -1 ] == 0x14 ) {
            /*
             *  '"'
             */
            in_string = !in_string;
        }
        if ( !in_string ) {

            if ( *line == ':' ) {
                /*
                 *  Statement delimiter
                 */
                *buff++ = 0xFE;
                ++l;
                ++line;
                continue;
            }
            if ( isupper( *line ) ) {
                /*
                 *  Look if a token is matched
                 */
                tp = Tokens;
                for ( i = 0x60; *tp != NULL; ++tp, ++i ) {
                    if ( 0 == strncmp( line, *tp, tl = strlen( *tp ) ) ) {
                        break;
                    }
                }
                if ( *tp != NULL ) {
                    /*
                     *  Replace string by token value
                     */
                    line += tl;
                    *buff++ = (unsigned char) i;
                    ++l;
                    continue;
                }
                /*
                 *  E in numbers
                 */
                if ( l > 2 && *line == 'E' 
                     && buff[ -1 ] <= 0x3A && buff[ -1 ] >= 0x30 ) 
                {
                    *buff++ = 0x3F;
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
            case 0xB0:
            case 0xBA:
                *buff = 0x12; break; /* degree */
            default:
                *buff = 0x5E;
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
        if ( strncmp( *argv, "-l", 2 ) == 0 ) {
            TranslateLog = TRUE;
        }
        ++argv;
        --argc;
    }

    if ( argc < 1 ) {
        printf( "usage: bas702 [-a|-b|-w] "
                        "infile outfile\n"
                "       -a create an ASCII encoded file for Piotr's interface\n"
                "       -b create a binary file\n"
                "       -w create a WAV file\n"
                "       -l convert LGT/LOG to LOG/LN\n"
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
    
    Data.segment_id = 0x01;
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
            /*
             *  Dump the header with the line number of the first line
             */
            memcpy( Header.line_no, buffer, 2 );
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


