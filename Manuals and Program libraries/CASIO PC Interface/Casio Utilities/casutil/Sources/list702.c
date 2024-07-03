/*
 *  list702.c - List Casio FX-702P tape files
 *
 *  This program displays the contents of a tape image saved by the FX-702P 
 *  calculator through the sound card or with Piotr's serial interface.
 *  Only program files supported.
 *
 *  Options:
 *    -b<skip> read BIN file (Default); skip <skip> garbage bytes
 *    -a       read ASCII file (Piotr's format)
 *    -w       read WAV file
 *
 *  Inspired by Piotr Piatek
 *  Written by Marcus von Cube
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bool.h"
#include "wave.h"

/*
 *  error messages
 */
const char *ErrMsg[] = {
    "",                                       /* OK */
    "leader expected",                        /* message #1 */
    "premature end of a header/data segment", /* message #2 */
    "premature end of a file",                /* message #3 */
    "header segment expected",                /* message #4 */
    "unknown segment identifier",             /* message #5 */
    "unexpected program separator"            /* message #6 */
};

/*
 *  file types 0xB_ to 0xF_
 */
const char *FileType[] =
{
    "Program",
    "Variables",
    "All programs"
};

/*
 *  character codes 0x11 to 0x5E
 */
const char Characters[] = 
    "                "
    " ?\xF8'"
       "\"#$;:,>>=<<!"
    "+-*/^ ! )   (   "
    "0123456789.p   E"
    "ABCDEFGHIJKLMNOP"
    "QRSTUVWXYZ    _ ";

/*
 *  Replacement for some special characters
 *  Only outside strings and comments
 */
struct _specials {
    char *text;
    unsigned char token;
} specials[] = 
{
    { " ",  0x0F },
    { " ",  0x10 },
    { ">=", 0x1B },
    { "=>", 0x1B },
    { "<=", 0x1D },
    { "=<", 0x1D },
    { "!=", 0x1F },
    { "<>", 0x1F },
    { "PI", 0x3B },
    { ":",  0xFE },
    { "\n", 0xFF },
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
    "CNT",     "SX",      "SY",      "SX2",
    "SY2",     "SXY",     "RAN#",    "MX",
    "MY",      "SDX",     "SDXN",    "SDY",
    "SDYN",    "LRA",     "LRB",     "COR",
    "SIN ",    "COS ",    "TAN ",    "ASN ",
    "ACS ",    "ATN ",    "HSN ",    "HCS ",
    "HTN ",    "AHS ",    "AHC ",    "AHT ",
    "LOG ",    "LN ",     "SQR ",    "EXP ",
    "ABS ",    "SGN ",    "INT ",    "FRAC ",
    "EOX ",    "EOY ",    "CSR ",    " TO ",
    " STEP ",  " THEN ",  "KEY",     "MID(",
    "LEN(",    "RND(",    "DEG(",    "ABS ",
    "FOR ",    "NEXT ",   "GOTO ",   "GSB ",
    "RET",     "INP ",    "PRT ",    "WAIT ",
    "MODE ",   "STOP ",   "END ",    "IF ",
    "STAT ",   "DEL ",    "SAC ",    "ALL ",
    "ROM ",    "DMS ",    "RPC ",    "PRC ",
    "SET ",    "VAC",     "PUT ",    "GET ",
    "LOAD ",   "SAVE ",   "VER ",    "DEFM ",
    "PASS ",   "LIST ",   "RUN ",    "CLR "
};

/*
 *  BASIC keywords 0x60 to 0xAF (new syntax)
 */
const char *NewTokens[] =
{
    "CNT",     "SUMX",    "SUMY",    "SUMX2",
    "SUMY2",   "SUMXY",   "RAN#",    "MEANX",
    "MEANY",   "SDX",     "SDXN",    "SDY",
    "SDYN",    "LRA",     "LRB",     "COR",
    "SIN ",    "COS ",    "TAN ",    "ASN ",
    "ACS ",    "ATN ",    "HYPSIN ", "HYPCOS ",
    "HYPTAN ", "HYPASN ", "HYPACS ", "HYPATN ",
    "LOG ",    "LN ",     "SQR ",    "EXP ",
    "ABS ",    "SGN ",    "INT ",    "FRAC ",
    "EOX ",    "EOY ",    "CSR ",    " TO ",
    " STEP ",  " THEN ",  "KEY$",    "MID$(",
    "LEN(",    "RND(",    "DEG(",    "ABS ",
    "FOR ",    "NEXT ",   "GOTO ",   "GOSUB ",
    "RETURN",  "INPUT ",  "PRINT ",  "WAIT ",
    "MODE ",   "STOP",    "END",     "IF ",
    "STAT ",   "DEL ",    "STAT CLEAR", "ALL ",
    "ROM ",    "DMS ",    "POL ",    "REC ",
    "SET ",    "CLEAR",   "PUT ",    "GET ",
    "LOAD ",   "SAVE ",   "VER ",    "DEFM ",
    "PASS ",   "LIST ",   "RUN ",    "NEW "
};

/*
 *  Flag for binary mode
 */
bool BinMode = TRUE;

/*
 *  Flag for new syntax and escape generation
 */
bool NewSyntax = FALSE;
bool EscapeMode = TRUE;

/*
 *  Output a character
 */
void casioprint( int c )
{
    struct _specials *sp;
    static bool in_string = FALSE;
    static bool space_pending = FALSE;
    int l;
    const char *p;
    const char **tokens = NewSyntax ? NewTokens : Tokens;

    if ( c == 0x14 ) {
        in_string = !in_string;
        if ( in_string && space_pending ) {
            putchar( ' ' );
        }
    }
    if ( c == 0xFE || c == 0xFF ) {
        /*
         *  Statement or line end
         */
        in_string = FALSE;
        space_pending = FALSE;
    }
    if ( space_pending && !in_string ) {
        /*
         *  Print pending space before keywords, characters and numbers
         */
        if ( c >= 0x30 && c <= 0x3B 
          || c >= 0x40 && c <= 0x59
          || c == 0x5E
          || c >= 0x60 )
        {
            putchar( ' ' );
        }
    }
    space_pending = FALSE;

    if ( in_string && EscapeMode && c <= 0x5F ) {
        /*
         *  Print escape sequences like \pi for PI
         */
        for ( sp = escapes; sp->text != NULL; ++sp ) {
            if ( c == sp->token ) {
                printf( "\\%s", sp->text );
                return;
            }
        }
    }
        
    /*
     *  Print special characters
     */
    for ( sp = specials; sp->text != NULL; ++sp ) {
        if ( c == sp->token ) {
            printf( "%s", sp->text );
            return;
        }
    }

    if ( c <= 0x5F ) {
        /*
         *  Print other characters or tokens
         */
        if ( EscapeMode && Characters[ c ] == ' ' ) {
            printf( "\\%2.2X", c );
        }
        else {
            printf( "%c", Characters[ c ] );
        }
    }
    else if ( !in_string && c <= 0xAF ) {
        /*
         *  Token
         */
        l = strlen( p = tokens[ c - 0x60 ] );
        if ( l > 0 && p[ l - 1 ] == ' ' ) {
            --l;
            space_pending = TRUE;
        }
        printf( "%.*s", l, tokens[ c - 0x60 ] );
    }
    else {
        if ( EscapeMode ) {
            printf( "\\%2.2X", c );
        }
        else {
            putchar( '?' );
        }
    }
}


/*
 *  convert a BCD number for output
 */
int bcd2bin( int c )
{
    int x;
    x = c / 16;
    return c - 6 * x;
}


/*
 *  output the next item
 *  returns 0 when OK or the error code when invalid data encountered
 */
int list( int c )
{
    static int skipped = 5;
    static int idle_counter = 0;
    static int data_counter = 0;
    static int line_counter = 0;
    static int line_number = 0;
    static int prog_number = 0;
    static int segment_id = 0;
    static int file_id = 0;
    static bool leader_expected = TRUE;
    static bool dataseg_expected = FALSE;

    if ( !BinMode ) {

        /*
         *  skip first 5 bytes before the leader
         */
        if ( skipped > 0 ) {
            skipped--;
            return 0;
        }

        /*
         *  handle the leader
         */
        if ( c == KCS_LEAD_IN ) {
            /*
             *  idle string (leader)
             */
            if ( idle_counter == 0 && !leader_expected ) {
                /*
                 *  "Premature end of a header/data segment" error
                 */
                return 2;
            }
            /*
             *  leader should contain at least 12 idle strings
             */
            if ( idle_counter++ >= 12 ) {
                /*
                 *  valid leader detected
                 */
                leader_expected = FALSE;
            }
            data_counter = 0;
            return 0;
        }

        /*
         *  data byte received
         */
        idle_counter = 0;
        if ( leader_expected ) {
            /*
             *  "Leader expected" error
             */
            return 1;
        }
        c &= 0xff;
    }
    else {
        /*
         *  BinMode
         */
        if ( leader_expected ) {
            if ( !dataseg_expected
                 && ( c & 0xf0 ) >= 0xb0
                 && ( ( c & 0x0f ) <= 8 || ( c & 0x0f ) == 0x0F )
                 || c == 0x01 )
            {
                /*
                 *  valid header byte encountered
                 */
                data_counter = 0;
                leader_expected = FALSE;
            }
            else {
                /* printf( "Skipped\n", c ); */
                return 0;
            }
        }
    }

    /* printf( "[%02.2X]", c ); */

    if ( ++data_counter == 1 ) {
        /*
         *  handle the first segment byte
         */
        segment_id = c;

        if ( dataseg_expected ) {
            if ( c == 0x01 ) {
                /*
                 *  start of data segment
                 */
                line_counter = 0;
                return 0;
            }
            else {
                /*
                 *  "Premature end of a file" error
                 */
                return 3;
            }
        }

        /*
         *  header segment expected
         *  check for data segment
         */
        if ( c == 0x01 ) {
            /*
             *  "Header segment expected" error
             */
            return 4;
        }

        if ( ( file_id = c & 0xF0 ) >= 0xD0 ) {
            /*
             *  output the file type
             */
            prog_number = 0;
            printf( "%s: \"", FileType[ file_id / 16 - 0x0D ] );
            return 0;
        }

        /*
         *  "Unknown segment identifier" error
         */
        return 5;
    }

    /*
     *  handle subsequent segment bytes
     */
    if ( segment_id == 0x01 ) {
       /*
        *  handle the data segment
        */
        if ( c == 0xF0 || c == 0xF1 ) {
            /*
             *  End Marker
             */
            leader_expected = TRUE;
            skipped = 5;
            dataseg_expected = ( c == 0xF1 );
            return 0;
        }

        if ( data_counter == 2 && ( file_id == 0xF0 || file_id == 0xB0 ) ) {
            /*
             *  P0
             */
            printf( " P0\n\n" );
        }

        /*
         *  programs separator ?
         */
        if ( c == 0xE0 ) {
            prog_number++;
            line_counter = 0;
            if ( file_id != 0xF0 || prog_number > 10 ) {
                /*
                 *  "Unexpected program separator" error
                 */
                return 6;
            }
            if ( prog_number < 10 ) {
                printf( "\n P%d\n\n", prog_number );
            }
            else {
                /*
                 *  Last program listed, dont't list the variables
                 */
                printf( "\n END\n" );
                return -1;
            }
            return 0;
        }

        if ( file_id == 0xE0 ) {
            /*
             *  don't display data segments containing variables
             */
            return 0;
        }

        if ( ++line_counter == 1 ) {
            /*
             *  the least significant byte of the line number
             */
            line_number = bcd2bin( c );
            return 0;
        }

        if ( line_counter == 2 ) {
            /*
             *  the most significant byte of the line number
             */
            line_number += 100 * bcd2bin( c );
            if ( line_number == 0 ) {
                printf( "Password: " );
                line_number = -1;
            }
            else {
                printf( "%d ", line_number );
            }
            return 0;
        }

        /*
         *  print remaining bytes of the data segment
         */
        if ( c == 0xFF ) {
            if ( line_number == -1 ) {
                /*
                 *  End of password
                 */
                putchar( '\n' );
            }
            line_counter = 0;
        }
        casioprint( c );
        return 0;
    }

    /*
     *  handle the header segment
     */
    if ( ( segment_id & 0x0F ) == 0x0F ) {
        /*
         *  no name
         */
        if ( data_counter == 2 ) {
            putchar( '"' );
        }
    }
    else if ( data_counter <= 1 + ( segment_id & 0x0F ) ) {
        /*
         *  display the file name
         */
        casioprint( c );
        if ( data_counter == 1 + ( segment_id & 0x0F ) ) {
            putchar( '"' );
        }
        return 0;
    }

    /*
     *  skip remaining data
     */
    if ( data_counter < 11 )
    {
        return 0;
    }

    /*
     *  last byte of the header segment
     */
    printf( "\n\n" );
    leader_expected = TRUE;
    skipped = 5;
    dataseg_expected = TRUE;
    return 0;
}


/*
 *  main
 */
int main( int argc, char **argv )
{
    int c1;
    int counter;          /* character counter */
    long position = 0;    /* file position */
    int err_code;
    unsigned int x;       /* 12-bit word */
    FILE *infp = NULL;
    KCS_FILE *wfp = NULL;
    int skip;
    bool wavemode = FALSE;
    int ignore = FALSE;

    while ( argc > 1 && argv[ 1 ][ 0 ] == '-' ) {

        if ( strncmp( argv[ 1 ], "-a", 2 ) == 0 ) {
            BinMode = FALSE;
        }
        if ( strncmp( argv[ 1 ], "-b", 2 ) == 0 ) {
            BinMode = TRUE;
            skip = atoi( argv[ 1 ] + 2 );
        }
        if ( strncmp( argv[ 1 ], "-w", 2 ) == 0 ) {
            BinMode = FALSE;
            wavemode = TRUE;
        }
        if ( strncmp( argv[ 1 ], "-i", 2 ) == 0 ) {
            ignore = TRUE;
        }
        if ( strncmp( argv[ 1 ], "-n", 2 ) == 0 ) {
            NewSyntax = TRUE;
        }
        if ( strncmp( argv[ 1 ], "-e", 2 ) == 0 ) {
            EscapeMode = TRUE;
        }
        ++argv;
        --argc;
    }

    if ( argc <= 1 ) {
        printf( "usage: list702 <options> infile > outfile\n"
                "         -w reads a WAV file directly\n"
                "         -a reads an ASCII encoded file from Piotr's"
                           " interface\n"
                "         -b<skip> reads a binary file, "
                           "<skip> is an optional offset\n"
                "         -i ignores any errors on the input stream\n"
                "         -n new syntax listing (PRT->PRINT)\n" 
                "         -e generate backslash escapes for special codes\n" );
        return 2;
    }

    if ( wavemode ) {
        if ( ( wfp = kcsOpen( *++argv, "rb", 300, 8, 'E', 2 ) ) == NULL ) {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    else {
        if ( ( infp = fopen( *++argv, BinMode ? "rb" : "rt" ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }

    x = 0;
    counter = 0;

    if ( wavemode ) {
        while ( ( c1 = kcsReadByte( wfp ) ) >= 0 ) {

            /* printf( "[%02.2X]", c1 & 0xff ); */

            if ( ignore 
                 && c1 != KCS_LEAD_IN && c1 & ( KCS_FRAMING | KCS_PARITY ) ) 
            {
                continue;
            }
            if ( ( err_code = list( c1 ) ) != 0 ) {
                if ( err_code == -1 ) {
                    break;
                }
                printf( "\nInvalid data encountered - %s.\n",
                        ErrMsg[ err_code ] );
                break;
            }
        }
        if ( c1 != KCS_EOF ) {
            perror( "read" );
            return 2;
        }
        kcsClose( wfp );
    }
    else {
        while ( ( c1 = fgetc( infp ) ) != EOF ) {

            /* printf( "[%02.2X]", c1 ); */

            if ( BinMode ) {
                if ( skip > 0 ) {
                    --skip;
                    ++position;
                    continue;
                }
                if ( ( err_code = list( c1 ) ) != 0 ) {
                    if ( err_code == -1 ) {
                        break;
                    }
                    printf ("\nInvalid data @%04.4lX [%02.2X] - %s.\n",
                            position, c1, ErrMsg[err_code]);
                    break;
                }
                ++position;
                continue;
            }

            /*
             *  skip invalid characters
             */
            if ( c1 < 0x30 || c1 > 0x6F ) {
                counter = 0;
            }
            else {
                /*
                 *  shift the received 6-bit data into the 12-bit word
                 */
                x = ( x >> 6 ) & 0x003F;
                x |= ( ( (unsigned int) ( c1 - 0x30 ) ) << 6 );

                if ( ++counter == 2 ) {
                    /*
                     *  already 2 characters processed
                     */
                    counter = 0;

                    /*
                     *  Strip the start, stop and parity bits
                     */
                    c1 = (int) ( ( x >> 1 ) & 0xFF );

                    if ( ( x & 0x01 ) != 0 ) {
                        /*
                         *  idle string (leader)
                         */
                        c1 = KCS_LEAD_IN;
                    }

                    if ( ( err_code = list( c1 ) ) != 0 ) {
                        if ( err_code == -1 ) {
                            break;
                        }
                        printf( "\nInvalid data encountered - %s.\n",
                                ErrMsg[ err_code ] );
                        break;
                    }
                }
            }
        }
        fclose( infp );
    }

    return 0;
}

