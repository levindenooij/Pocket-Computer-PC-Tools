/*
 *  wave850.c - Create Casio tape WAV files
 *
 *  This program creates a WAV file from a binary image
 *  from the Casio calculators PB-700, FX-750P, FX-850P or PB-1000.
 *
 *  Options:
 *    -b<skip> read BIN file (Default); skip <skip> garbage bytes
 *    -a       read ASCII file (Piotr's format)
 *    -s       create a 300  baud WAV file (slow: PB-700, FX-750P, FP-200)
 *    -f       create a 1200 baud WAV file (fast: FX-850P, PB-1000)
 *    -2       handle FP-200 specifics
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bool.h"
#include "wave.h"

#define LEAD_IN_TIME 20       /* tens of a second */
#define LEAD_IN_TIME_FIRST 20 /* tens of a second */
#define LEAD_IN_TIME_FP200 30 /* tens of a second */

/*
 *  Error messages
 */
const char *err_msg[] = {
    "",                                       /* OK */
    "Leader expected",                        /* message #1 */
    "Premature end of a header/data segment", /* message #2 */
    "Premature end of a file",                /* message #3 */
    "Header segment expected",                /* message #4 */
    "Unknown segment identifier",             /* message #5 */
    "Unexpected program separator",           /* message #6 */
    "Invalid file type in data segment",      /* message #7 */
    "No CR at end of line",                   /* message #8 */
    "Program table is last segment",          /* message #9 */
    "Program table is missing",               /* message #10 */
    "Program table doesn't have 10 entries",  /* message #11 */
    "Write error"                             /* message #12 */
};

/*
 *  Wave file handling
 */
KCS_FILE *Out = NULL;
char *OutName;
int Baudrate = 0;

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

void printHeader( void )
{
    int i;

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
        printf( "Program (FX-850P/PB-1000): " );
        break;

    case TYPE_ALL850:
        printf( "All Programs (FX-850P/PB-1000): " );
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
 *  Output a byte to the WAV file
 *  Returns 0 when OK or the error code when invalid data encountered
 */
int output( int c )
{
    static int lead_in_time;
    static int skipped = 5;
    static int idle_counter = 0;
    static int data_counter = 0;
    static int segment_id = 0;
    static int ignore_counter = 0;
    static int transparent_counter = 0;
    static int length_counter = 0;
    static int ptlength_counter = 0;
    static int progtab_length = 0;
    static int progtab_counter = 0;
    static int program_length = 0;
    static int prog_number = 0;

    static bool lead_in_expected = TRUE;
    static bool dataseg_expected = FALSE;
    static bool textline_expected = FALSE;
    static bool progtab_expected = FALSE;
    static bool last_segment = FALSE;

    static int progtab[ 10 ];

    if ( !BinMode ) {
        /*
         *  Piotr's format
         */
        if ( lead_in_expected && c != KCS_LEAD_IN ) {
            /*
             *  Data byte where an idle string is expected.
             *  This can be a trailing byte in a block of unknown
             *  meaning or just garbage.
             *  Write it to the file but don't interprete it.
             */
            if ( Out != NULL && 0 != kcsWriteByte( Out, c ) ) {
                /*
                 *  I/O error
                 */
                return 12;
            }
            if ( ++skipped >= 20 ) {
                /*
                 *  "Leader expected" error
                 */
                return 1;
            }
            return 0;
        }

        /*
         *  Handle the leader
         */
        if ( c == KCS_LEAD_IN ) {
            /*
             *  Idle string (leader)
             */
            if ( idle_counter == 0 && !lead_in_expected ) {
                /*
                 *  "Premature end of a header/data segment" error
                 */
                return 2;
            }

            if ( idle_counter++ >= 12 ) {
                /*
                 *  Lead-in contains at least 12 idle strings
                 */
                lead_in_expected = FALSE;

                /*
                 *  Write a lead in to the file
                 */
                if ( Out != NULL && lead_in_time > 0
                     && 0 != kcsLeadIn( Out, lead_in_time ) )
                {
                    /*
                     *  I/O error
                     */
                    return 12;
                }

                /*
                 *  Don't write another lead-in in this block
                 */
                lead_in_time = 0;
            }
            skipped = 0;
            data_counter = 0;
            return 0;
        }
        else {
            /*
             *  Data byte received
             */
            idle_counter = 0;

            if ( lead_in_expected ) {
                /*
                 *  "Leader expected" error
                 */
                return 1;
            }
            /*
             *  Strip framing information
             */
            c &= 0xff;

            /*
             *  For the next block to come
             */
            lead_in_time = Fp200 ? LEAD_IN_TIME_FP200 : LEAD_IN_TIME;
        }
#if DEBUG
        printf( "<%02.2X>", c );
#endif
    }
    else {
        /*
         *  Binary mode
         */
        if ( lead_in_expected ) {
            /*
             *  Check for 'H'eader or 'D'ata segment
             */
            if ( !dataseg_expected && c == 'H' || c == 'D' ) {
                /*
                 *  Valid header byte encountered
                 */
                data_counter = 0;
                lead_in_expected = FALSE;
#if DEBUG
                printf( "Segment: %c\n", c );
#endif
                /*
                 *  Create a lead-in for this block
                 */
                if ( Out != NULL && 0 != kcsLeadIn( Out, lead_in_time ) ) {
                    /*
                     *  I/O error
                     */
                    return 12;
                }
                lead_in_time = Fp200 ? LEAD_IN_TIME_FP200 : LEAD_IN_TIME;
            }
            else {
                /*
                 *  Write all bytes, even if ignored
                 */
                if ( Out != NULL && 0 != kcsWriteByte( Out, c ) ) {
                    /*
                     *  I/O error
                     */
                    return 12;
                }    
                return 0;
            }
        }
    }

    /*
     *  Output the byte
     */
    if ( Out != NULL && 0 != kcsWriteByte( Out, c ) ) {
        /*
         *  I/O error
         */
        return 12;
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
            lead_in_expected = TRUE;
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
                 *  End of line
                 */
                if ( c != 0x0d && c != 0x0a ) {
                    /*
                     *  "Missing line terminator" error
                     */
                    return 8;
                }

                /*
                 *  Skip checksum byte
                 */
                if ( last_segment ) {
                    dataseg_expected = FALSE;
                }
                ignore_counter = Fp200 ? 2 : 1;
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
                    ignore_counter = 1;
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
            lead_in_expected = TRUE;
            dataseg_expected = ( c == 0xF1 );
            return 0;
        }

        if ( data_counter == 2 ) {
            /*
             *  Special handling of first data byte of segment
             */
            switch ( Header.file_type ) {

            case TYPE_ALL:
            case TYPE_PROGRAM:  
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
#if DEBUG
                printf( "program_length = %d\n", program_length );
#endif
                return 0;

            case TYPE_PROG850:
                program_length = Header.parameters[ 6 ]
                               + Header.parameters[ 7 ] * 256;
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

        if ( Fx850P || Fp200 ) {
            /*
             *  FX-850P family or FP-200
             */
            if ( data_counter == 3 ) {
                /*
                 *  "Last segment" Flag
                 */
                last_segment = ( c != 0 );
#if DEBUG
                printf( "last_segment = %d\n", last_segment );
#endif
                return 0;
            }
            if ( --program_length == 0 ) {
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
#if DEBUG
                        printf( "program_length = %d\n", program_length );
#endif
                    }
                    else {
                        /*
                         *  End of segment: skip checksum
                         */
                        ignore_counter = 1;
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
                    ignore_counter = Fp200 ? 2 : 1;
                }
            }
        }
        return 0;
    }

    /*
     *  Handle the header segment
     */
    ((char *) &Header)[ data_counter - 1 ] = c;

    if ( data_counter == HEADER_SIZE ) {
        /*
         *  End of header
         */
        int i, err;
        Fx850P = Header.file_type == TYPE_PROG850
              || Header.file_type == TYPE_ALL850;
        if ( Fp200 ) {
            Fx850P = FALSE;
        }
        ignore_counter = Fp200 ? 2 : 1;
        dataseg_expected = TRUE;

        Header.segment_id = segment_id;

        /*
         *  Display header information
         */
        printHeader();

        /*
         *  Open the output WAV file
         */
        if ( Baudrate == 0 ) {
            /*
             *  Automatic baudrate selection
             */
            Baudrate = Fx850P ? 1200 : 300;
        }
        Out = kcsOpen( OutName, "wb", Baudrate, 8, 'E', 2 );
        if ( NULL == Out ) {
            /*
             *  I/O error
             */
            fprintf( stderr, "Cannot open output file %s\n", OutName );
            return 12;
        }
        printf( "Output file %s at %d baud opened\n", OutName, Baudrate );

        /*
         *  Dump header to WAV file
         */
        err = kcsLeadIn( Out, LEAD_IN_TIME_FIRST );

        for ( i = 0; i < HEADER_SIZE && err == 0; ++i ) {
            /*
             *  Write each header byte
             */
            c = ((char *) &Header)[ i ];
            err = kcsWriteByte( Out, c );
        }
        if ( err != 0 ) {
            /*
             *  I/O error
             */
            return 12;
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
    FILE *infp;
    int skip = 0;

    ++argv;
    --argc;

    while ( argc > 0 && **argv == '-' ) {
        /*
         *  Options
         */
        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            BinMode = TRUE;
            skip = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-a", 2 ) == 0 ) {
            BinMode = FALSE;
            skip = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-f", 2 ) == 0 ) {
            Baudrate = 1200;
        }
        if ( strncmp( *argv, "-s", 2 ) == 0 ) {
            Baudrate = 300;
        }
        if ( strncmp( *argv, "-2", 2 ) == 0 ) {
            Baudrate = 300;
            Fp200 = TRUE;
        }
        ++argv;
        --argc;
    }

    /*
     *  Open input file
     */
    if ( argc < 1 ) {
        printf( "usage: wave850 [-a|-b<skip>] infile outfile\n"
                "       -a read an ASCII encoded file from Piotr's interface\n"
                "       -b<skip> read a binary file, "
                        "<skip> is an optional offset\n"
                "       -s create a  300 baud WAV file "
                        "(slow: PB-700, FX-750P)\n"
                "       -f create a 1200 baud WAV file "
                        "(fast: FX-850P, PB-1000)\n"
                "       -2 handle FP-200 specifics\n"
              );
        return 2;
    }
    if ( ( infp = fopen( *argv, BinMode ? "rb" : "rt" ) ) == NULL ) {
        fprintf( stderr, "Cannot open input file\n" );
        perror( *argv );
        return 2;
    }
    ++argv;
    --argc;

    /*
     *  WAV file
     */
    if ( argc < 1 ) {
        fprintf( stderr, "Missing output file name\n" );
        return 2;
    }
    OutName = *argv;

    x = 0;
    counter = 0;

    /*
     *  Process the input file
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
            if ( ( err_code = output( c1 ) ) != 0 ) {
                /*
                 *  Error
                 */
                if ( err_code == 12 ) {
                    perror( OutName );
                }
                else {
                    printf( "\nInvalid data @%04.4lX [%02.2X] - %s.\n",
                             position, c1, err_msg[ err_code ] );
                }
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

                if ( ( err_code = output( c1 ) ) != 0 ) {
                    /*
                     *  Error
                     */
                    if ( err_code == 12 ) {
                        perror( OutName );
                    }
                    else {
                        printf( "Invalid data encountered [%02.2X] - %s.\n",
                                c1, err_msg[ err_code ] );
                    }
                    break;
                }
            }
        }
    }
    if ( Fp200 ) {
        Out->raw = TRUE; /* No lead out sequence */
        kcsLeadIn( Out, 1 ); /* short leader for termination */
    }

    kcsClose( Out );
    fclose( infp );

    return 0;
}
