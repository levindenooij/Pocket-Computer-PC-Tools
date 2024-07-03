/*
 *  wav2raw.c - Cretate a binary file from a Casio tape image
 *
 *  This program translates a WAV file recorded through the sound card from
 *  a Casio calculator (FX or PB series) to a binary image in various formats.
 *
 *  Options:
 *    -w words (with framing information)
 *    -b bytes (only decoded data bytes)
 *    -a ascii (ASCII encoded raw data, Piotr's format)
 *    -r raw (raw bits in 16 bit words)
 *    -s slow (300 baud) wave file
 *    -f fast (1200 baud) wave file
 *    -h high speed (2400 baud) wave file
 *    -5 500 baud (Sharp)
 *    -t TI-74 mode
 *    -p<parity> select parity (and stopbits)
 *    -i ignore parity or framing errors in format -b
 *    -d debug
 *
 *  Written by Marcus von Cube
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "wave.h"

int main( int argc, char **argv )
{
    KCS_FILE *in;
    FILE *out;
    short w = 0;
    short lastw;
    unsigned char b;
    int blocks = 0;
    long cframing = 0;
    long cparity = 0;
    long count = 0;
    long position = 0;
    long where;
    char format = 'w';
    int debug = 0;
    int baud = 300;
    int bits = 8;
    int parity = 'E';
    int stop = 2;
    int ignore = 0;

    while ( argc > 1 && *argv[ 1 ] == '-' ) {
        ++argv;
        --argc;
        if ( (*argv)[ 1 ] == 'd' ) {
            debug = 1;
            continue;
        }
        if ( (*argv)[ 1 ] == 's' || (*argv)[ 1 ] == 'S' ) {
            baud = 300;
            if ( (*argv)[ 2 ] == '-' ) baud = -baud;
            continue;
        }
        if ( (*argv)[ 1 ] == 'f' || (*argv)[ 1 ] == 'F' ) {
            baud = 1200;
            if ( (*argv)[ 2 ] == '-' ) baud = -baud;
            continue;
        }
        if ( (*argv)[ 1 ] == 'h' || (*argv)[ 1 ] == 'H' ) {
            baud = 2400;
            if ( (*argv)[ 2 ] == '-' ) baud = -baud;
            continue;
        }
        if ( (*argv)[ 1 ] == 'p' || (*argv)[ 1 ] == 'P' ) {
            parity = toupper( (*argv)[2] );
            if ( (*argv)[3] == '1' || (*argv)[3] == '2' ) {
                stop = (*argv)[3] & 0x03;
            }
            continue;
        }
        if ( (*argv)[ 1 ] == '5' ) {
            baud = 500;
            bits = 4;
            parity = 'N';
            stop = 1;
            continue;
        }
        if ( (*argv)[ 1 ] == 't' || (*argv)[ 1 ] == 'T' ) {
            baud = 1400;
            bits = 8;
            parity = 'N';
            stop = 0;
            format = 'b';
            continue;
        }
        if ( (*argv)[ 1 ] == 'i' ) {
            ignore = 1;
            continue;
        }
        format = (*argv)[ 1 ];
    }

    if ( --argc < 2 || NULL == strchr( "wbar", format ) ) {
        fprintf( stderr,
                "usage: wav2raw [options] [format] <infile> <outfile>\n"
                "         format is one of:\n"
                "           -w words (with framing information)\n"
                "           -b bytes (only decoded data bytes)\n"
                "           -a ascii (ASCII encoded raw data)\n"
                "           -r raw   (raw bits in 16 bit words)\n"
                "         options are:\n"
                "           -s slow (300 baud) wave file (default)\n"
                "           -f fast (1200 baud) wave file\n"
                "           -h high speed (2400 baud) wave file\n"
                "              Try -s-, -f- or -h- in case of read errors\n"
                "           -5 Sharp (500 baud) wave file\n"
                "           -t TI-74 (1400 baud synchronous) wave file\n"
                "           -p{E|O|N}[1|2] select parity and stopbits\n"
                "           -i ignore parity or framing errors in format -b\n"
                "           -d debug\n" );
        return 2;
    }
    if ( debug ) {
        printf( "baud=%d,bits=%d,parity=\'%c\',stopbits=%d\n",
                baud, bits, parity, stop );
    }
    in = kcsOpen( *++argv, "rb", baud, bits, parity, stop );
    if ( in == NULL ) {
        perror( "in" );
        return 2;
    }
    out = fopen( *++argv, format == 'a' ? "wt" : "wb" );
    if ( out == NULL ) {
        perror( "out" );
        return 2;
    }

    while ( 1 ) {

        lastw = w;

        switch ( format ) {

        case 'w':
        case 'b':
            w = kcsReadByte( in );
            break;

        case 'a':
            w = kcsReadAscii( in );
            break;

        case 'r':
            w = kcsReadRaw( in );
        }

        if ( w < 0 ) {
            if ( w != KCS_EOF ) {
                fprintf( stderr, "result = %d\n", w );
                if ( errno != 0 ) {
                    perror( "read" );
                }
            }
            break;
        }

        switch ( format ) {

        case 'w':
        case 'b':
            if ( w == KCS_LEAD_IN ) {
                if ( lastw != w ) {
                    ++blocks;
                }
                if ( format == 'b' ) {
                    continue;
                }
            }
            else {
                where = wtell( in->file );
                if ( w & KCS_FRAMING ) {
                    ++cframing;
                    if ( debug ) {
                        printf( "framing error @ %ld, 0x%06.6lX\n", 
                                where, position );
                    }
                }
                if ( w & KCS_PARITY ) {
                    ++cparity;
                    if ( debug ) {
                        printf( "parity error @ %ld, 0x%06.6lX\n", 
                                where, position );
                    }
                }
                if ( format == 'b' && !ignore 
                     && w & ( KCS_FRAMING | KCS_PARITY ) ) 
                {
                    continue;
                }
                ++count;
            }
            break;

        case 'a':
        case 'r':
            if ( ( w & 1 ) == 1 ) {
                if ( ( lastw & 1 ) == 0 ) {
                    ++blocks;
                }
            }
            else {
                ++count;
            }
            break;
        }

        /*
         *  write data to file
         */
        b = (unsigned char) w;
        if ( 1 != fwrite( &b, 1, 1, out ) ) {
            perror( "write" );
            break;
        }
        ++position;
        if ( format == 'b' ) {
            continue;
        }
        b = (unsigned char) ( w >> 8 );
        if ( 1 != fwrite( &b, 1, 1, out ) ) {
            perror( "write" );
            break;
        }
        ++position;
        if ( format == 'a' && position % 64 == 0 ) {
            fprintf( out, "\n" );
        }
    }
    kcsClose( in );
    fclose( out );

    printf( "Blocks: %d, Chars: %ld", blocks, count );
    if ( format == 'w' || format == 'b' ) {
        printf( ", parity errors: %ld, framing errors %ld",
                cparity, cframing );
    }
    puts( "" );
    return 0;
}
