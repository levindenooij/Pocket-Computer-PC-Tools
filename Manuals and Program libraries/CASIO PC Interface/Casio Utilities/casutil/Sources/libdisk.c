/*
 *   md100.c - Handle Casio md100 disk images
 *
 *   libdisk.c - Direct access module using the LibDsk library by
 *               John Elliot (http://www.seasip.demon.co.uk/Unix/LibDsk)
 *
 *   Marcus von Cube
 *
 *   30.03.2006 1.0 created
 */

#include <libdsk.h>

/*
 *  MD-100 disks are 320KB, 80 tracks, SS, DD, 16 sectors per track,
 *  256 bytes per sector
 */
DSK_GEOMETRY DskGeometry =
{
    SIDES_ALT,             /* meaningless for single sided disk */
    80, 1, 16,             /* tracks, sides, sectors per track */
    1, 256,                /* sector base and size */
    RATE_SD,               /* Double(!) density disk in 3.5" drive */
    12, 23,                /* R/W and format gaps (???) */
    0, 0, 0                /* MFM, "skip" flags */
};

/*
 *  Handle
 */
DSK_PDRIVER DskHandle = NULL;


/*
 *  Check for errors
 */
int checkError( char *message, dsk_err_t err )
{
    if ( err != DSK_ERR_OK ) {
        fprintf( stderr, "%s: %s\n", message, dsk_strerror( err ) );
        return NOT_OK;
    }
    else {
        return OK;
    }
}


/*
 *  Get access to the device
 */
#ifdef __BORLANDC__
#pragma argsused
#endif
int openDirect( char *name, int create, int noUpdate )
{
    dsk_err_t err = dsk_open( &DskHandle, name, NULL, NULL );
#if DEBUG
    printf( "opened\n" );
#endif
    if ( err != DSK_ERR_OK ) {
        fprintf( stderr, "Error opening device \"%s\":%s\n",
                         name, dsk_strerror( err ) );
        return NOT_OK;
    }
    return OK;
}


/*
 *  Terminate access
 */
int closeDirect( void )
{
    dsk_err_t err = dsk_close( &DskHandle );
    return checkError( "Error closing device", err );
}


/*
 *  Read data
 */
int readDirect( void *dest, int number, int count )
{
    dsk_err_t err = DSK_ERR_OK;
    int sec  = 4 * number;
    int secs = 4 * count;
    char *buff = (char *) dest;

    while ( err == DSK_ERR_OK && secs-- > 0 ) {
        /*
         *  Read a single sector
         */
#if DEBUG
        printf( "dsk_lread(%d)\n", sec );
#endif
        err = dsk_lread( DskHandle, &DskGeometry, buff, sec );
        ++sec;
        buff += 256;
    }

    return checkError( "Read error", err );
}


/*
 *  Write data
 */
int writeDirect( void *source, int number, int count )
{
    dsk_err_t err = DSK_ERR_OK;
    int sec  = 4 * number;
    int secs = 4 * count;
    char *buff = (char *) source;

    while ( err == DSK_ERR_OK && secs-- > 0 ) {
        /*
         *  Read a single sector
         */
        err = dsk_lwrite( DskHandle, &DskGeometry, buff, sec );
        ++sec;
        buff += 256;
    }

    return checkError( "Write error", err );
}

