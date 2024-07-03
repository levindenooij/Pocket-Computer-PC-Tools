/*
 *  wav2wav.c - Copy a Casio tape WAV file bit for bit
 *
 *  Options:
 *    -f  fast (1200 baud) mode for both files!
 *    -if fast (1200 baud) mode for input only!
 *    -of fast (1200 baud) mode for output only!
 *    -5  500 baud (Sharp)
 *    -1 1500 baud (Sharp el-9x00)
 *    -d debug
 *    -s preserve silence
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wave.h"

int main( int argc, char **argv )
{
    KCS_FILE *in, *out;
    int bit, rc;
    long count = 0L;
    long bad = 0L;
    int debug = 0;
    int preserve_silence = 0;
    int in_baud = 300;
    int out_baud = 300;
    unsigned long pos, len;

    while ( argc > 1 && *argv[ 1 ] == '-' ) {

        if ( 0 == strcmp( "-d", argv[ 1 ] ) ) {
            debug = 1;
        }
        if ( 0 == strcmp( "-D", argv[ 1 ] ) ) {
            debug = 2;
        }
        if ( 0 == strcmp( "-s", argv[ 1 ] ) ) {
            preserve_silence = 1;
        }
        if ( 0 == strcmp( "-f", argv[ 1 ] ) ) {
            in_baud = 1200;
            out_baud = 1200;
        }
        if ( 0 == strcmp( "-h", argv[ 1 ] ) ) {
            in_baud = 2400;
            out_baud = 2400;
        }
        if ( 0 == strcmp( "-if", argv[ 1 ] ) ) {
            in_baud = 1200;
        }
        if ( 0 == strcmp( "-of", argv[ 1 ] ) ) {
            out_baud = 1200;
        }
        if ( 0 == strcmp( "-ih", argv[ 1 ] ) ) {
            in_baud = 2400;
        }
        if ( 0 == strcmp( "-oh", argv[ 1 ] ) ) {
            out_baud = 2400;
        }
        if ( 0 == strcmp( "-5", argv[ 1 ] ) ) {
            in_baud = 500;
            out_baud = 500;
        }
        if ( 0 == strcmp( "-15", argv[ 1 ] ) ) {
            in_baud = 1500;
            out_baud = 1500;
        }
        if ( 0 == strcmp( "-t", argv[ 1 ] ) ) {
            in_baud = 1400;
            out_baud = 1400;
        }
        ++argv;
        --argc;
    }

    if ( --argc < 2 ) {
        printf( "usage: wav2wav <options> <infile> <outfile>\n"
                "         -d or -D debug modes\n"
                "         -s preserve silent areas in recording\n"
                "         -f fast (1200 baud) input and output\n"
                "         -if fast input file only\n"
                "         -of fast output file only\n"
                "         -h high speed (2400 baud) input and output\n"
                "         -ih high speed input file only\n"
                "         -oh high speed output file only\n"
                "         -5 500 baud (Sharp pocket computers) in/out\n"
                "         -15 1500 baud (Sharp graphics calculators) in/out\n"
                "         -t 1400 baud (Texas Instruments TI-74/95) in/out\n"
                "         Fast mode works for FX-850P, PB-1000 and Canon X-07\n"
                "         High speed mode works for the PB-1000 and the "
                          "fx-8000G series.\n"
              );
        return 2;
    }
    in = kcsOpen( *++argv, "rb", in_baud, 8, 'N', 1 );
    if ( in == NULL ) {
        perror( "in" );
        return 2;
    }
    out = kcsOpen( *++argv, "wb", out_baud, 8, 'N', 1 );
    if ( out == NULL ) {
        perror( "out" );
        return 2;
    }
    out->raw = 1;

    do {
        pos = in->file->position;
        bit = kcsReadBit( in );
        len = in->file->position - pos;
        if ( preserve_silence && in->state != KCS_SYNCHED 
             && len >= (unsigned long) ( 2 * in->bitLength ) )
        {
            /*
             *  Write out silence
             */
            if ( in->file->header.formatChunk.samplesPerSec !=
                out->file->header.formatChunk.samplesPerSec ) 
            {
                double f = (double) out->file->header.formatChunk.samplesPerSec 
                         / (double) in->file->header.formatChunk.samplesPerSec;
                len = (unsigned long) ( (double) len * f ); 
            }
            rc = writeSilence( out->file, len );
            if ( rc < 0 ) {
                perror( "out" );
                return 2;
            }
        }
        if ( bit >= 0 ) {
            if ( debug ) {
                printf( "%d", in_baud == 1500 ? 1 - bit : bit );
            }
            rc = kcsWriteBit( out, bit );
            if ( rc < 0 ) {
                perror( "out" );
                return 2;
            }
            ++count;
            if ( debug == 1 && count % 64 == 0 ) {
                putchar( '\n' );
            }
        }
        else {
            if ( debug ) {
                printf( " %d@%ld\n", bit, wtell( in->file ) );
            }

            switch ( bit ) {
                
            case KCS_BAD_DATA:
                bit = 0;
                ++bad;
                continue;

            case KCS_ERROR:
                perror( "in" );
                break;

            case KCS_EOF:
                break;
            }
        }
    } while ( bit >= 0 );

    kcsClose( out );
    kcsClose( in );

    printf( "\nBits: %ld, bad: %ld\n", count, bad );
    return 0;
}
