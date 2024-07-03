/*
 *  waveX07.c - Create Canon X-07 tape image from binary file
 *
 *  This program creates a binary tape image file for the Canon X-07 computer.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"

#define LEAD_IN_TIME 6        /* tens of a second */
#define LEAD_IN_TIME_FIRST 20 /* tens of a second */

/*
 *  Error messages
 */
const char *err_msg[] = {
    "",                                       /* OK */
    "Write error",                            /* message #1 */
};


/*
 *  File handling
 */
KCS_FILE *WaveOut = NULL;
FILE *FileIn = NULL;

#define HEADER_SIZE  16

/*
 *  Output a byte to the WAV file
 *  Returns 0 when OK or the error code 
 */
int output( int c )
{
    static int lead_in_time = LEAD_IN_TIME_FIRST;

    errno = 0;

    switch ( c ) {

    case KCS_LEAD_IN:
        /*
         *  Write a sequence of '1' bits
         */
        if ( 0 != kcsLeadIn( WaveOut, lead_in_time ) ) {
            /*
             *  I/O error
             */
            return 1;
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
            return 1;
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
    long position = 0;    /* file position */
    int err_code;
    int skip = 0;
    int count = 0;

    ++argv;
    --argc;

    while ( argc > 1 && **argv == '-' ) {

        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            skip = atoi( *argv + 2 );
        }
        ++argv;
        --argc;
    }

    if ( argc < 2 ) {
        printf( "usage: waveX07 -b<skip> infile outfile\n"
                "               <skip> is an optional offset into the file\n" );
        return 2;
    }

    /*
     *  Open binary file
     */
    if ( ( FileIn = fopen( argv[ 0 ], "rb" ) ) == NULL ) {
        fprintf( stderr, "\nCannot open input file %s\n", argv[ 0 ] );
        perror( "error" );
        return 2;
    }

    /*
     *  Open WAV file
     */
    if ( ( WaveOut = kcsOpen( argv[ 1 ], "wb", 1200, 8, 'N', 3 ) ) == NULL )
    {
        fprintf( stderr, "\nCannot open output file %s\n", argv[ 1 ] );
        perror( "error" );
        return 2;
    }

    /*
     *  Process binary file
     */
    while ( ( c1 = fgetc( FileIn ) ) != EOF ) {
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

        if ( count == 0 || count == HEADER_SIZE ) {
            /*
             *  Lead in sequence at start and after header
             */
            err_code = output( KCS_LEAD_IN );
        }
        if ( err_code != 0 || ( err_code = output( c1 ) ) != 0 ) {
            /*
             *  Error
             */
            printf( "\nError @%04.4lX \\%02.2X - %s.\n",
                        position, c1, err_msg[ err_code ] );
            break;
        }
        ++position;
        ++count;
    }
    fclose( FileIn );
    kcsClose( WaveOut );
    
    return 0;
}

