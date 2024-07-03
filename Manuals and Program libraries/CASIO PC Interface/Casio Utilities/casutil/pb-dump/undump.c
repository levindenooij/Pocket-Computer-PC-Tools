/*
 *  undump.c
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main( int argc, char**argv )
{
    FILE *in, *out;
    char line[ 72 ];
    unsigned char buff[ 32 ];
    int i, b, a;
    char *p;
    char *infile = "dump.txt", *outfile = "dump.bin";

    if ( argc > 1 ) {
        infile = argv[ 1 ];
    }
    if ( argc > 2 ) {
        outfile = argv[ 2 ];
    }

    printf( "Reading %s, updating %s\n", infile, outfile );

    in = fopen( infile, "rt" );
    out = fopen( outfile, "r+b" );
    if ( errno ) {
        errno = 0;
        out = fopen( outfile, "w+b" );
    }

    while ( !errno && !feof( in ) ) {
        
        if ( NULL == fgets( line, 72, in ) ) {
            break;
        }
        strtok( line, "\r\n" );
        if ( strlen( line ) != 70 ) {
            continue;
        }
        printf( "%s\n", line );
        sscanf( line, "%4X", &a );
        for ( i = 0, p = line + 6; i < 32; ++i, p += 2 ) {
            sscanf( p, "%2X", &b );
            buff[ i ] = (unsigned char) b;
        }
        fseek( out, a, SEEK_SET );
        if ( !errno ) {
            fwrite( buff, 32, 1, out );
        } 
        if ( errno ) {
            break;
        }
    }
    if ( errno ) {
        perror( "Error" );
        return 2;
    }
    fclose( out );
    return 0;
}

