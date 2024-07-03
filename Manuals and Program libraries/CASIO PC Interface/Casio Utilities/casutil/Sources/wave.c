/*
 *  wave.c
 *
 *  Library to create or read wave files
 *
 *  Kansas City Standard formatted files are created uncompressed, 8 bit mono,
 *  with a sample rate of 22050.
 *
 *  All functions return errors in errno. This is indicated by
 *  a return value of NULL (open) or a nonzero result.
 */
#define _DEBUG 0
#define _DEBUG_ 0

#include "wave.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MIN_GOOD 4

/*
 *  Open a wave file
 *
 *  openmode is "rb" or "wb"
 *  channels, bitsPerSample and samplesPerSec are used only for writing
 */
WFILE *wopen( char *filename,
              char *openmode,
              uint16_t channels,
              uint16_t bitsPerSample,
              uint32_t samplesPerSec )
{
    WFILE *file = NULL;
    FILE *f = NULL;
    FORMAT_CHUNK *fmt;
    DATA_CHUNK *data;
    uint16_t sampleSize;
    size_t len;

    /*
     *  Only 'r'ead or 'w'rite supported
     */
    if ( openmode[ 0 ] != 'r' && openmode[ 0 ] != 'w' ) {
        errno = EINVAL;
        return NULL;
    }
    if ( openmode[ 0 ] == 'w' ) {
        /*
         *  Output: Check parameters
         */
        if ( channels == 0
          || bitsPerSample == 0
          || bitsPerSample > 32
          || samplesPerSec == 0 )
        {
            errno = EINVAL;
            goto error;
        }

        /*
         *  compute sample size for buffer allocation
         */
        sampleSize = (uint16_t) ( channels * ( 7 + bitsPerSample ) / 8 );
    }
    else {
        /*
         *  Input: Set sample size to zero until we know better from the file
         *  Buffer will be reallocated later
         */
        sampleSize = 0;
    }

    /*
     *  Allocate control block
     */
    file = malloc( sizeof( WFILE ) + sampleSize - 4 );
    if ( file == NULL ) {
        return NULL;
    }
    fmt  = &( file->header.formatChunk );

    /*
     *  Open the file
     */
    f = file->file = fopen( filename, openmode );
    if ( f == NULL ) {
        free( file );
        return NULL;
    }

    /*
     *  save openmode
     */
    file->openmode = openmode[ 0 ];

    /*
     *  Input: Read Header
     */
    if ( file->openmode == 'r' ) {
        /*
         *  Read file type
         */
        len = fread( &( file->header ), 4, 3, f );
        if ( len != 3 ) {
            free( file );
            return NULL;
        }
        if ( 0 != memcmp( file->header.fileId, "RIFF", 4 )
          || 0 != memcmp( file->header.typeId, "WAVE", 4 ) )
        {
            goto badfile;
        }
        file->header.fileSize = GET_LSB_FIRST( file->header.fileSize );

        /*
         *  Look for format chunk
         */
        while ( 1 ) {
            /*
             *  Read chunkId and chunkSize
             */
            len = fread( fmt, 4, 2, f );
            if ( len != 2 ) {
                goto error;
            }

            if ( 0 == memcmp( fmt->chunkId, "fmt ", 4 ) ) {
                /*
                 *  format chunk found, leave loop
                 */
                break;
            }

            if ( 0 == memcmp( fmt->chunkId, "data", 4 ) ) {
                /*
                 *  early data chunk found
                 */
                goto badfile;
            }

            /*
             *  Skip unknown chunk
             */
            len = (size_t) GET_LSB_FIRST( fmt->chunkSize );
            if ( 0 != fseek( f, len, SEEK_CUR ) ) {
                goto error;
            }
        }

        /*
         *  read rest of format chunk
         */
        fmt->chunkSize = GET_LSB_FIRST( fmt->chunkSize );
        if ( fmt->chunkSize < sizeof( *fmt ) - 8 ) {
            goto badfile;
        }

        len = fread( &( fmt->formatTag ), sizeof( *fmt ) - 8, 1, f );
        if ( len != 1 ) {
            goto error;
        }
        len = fmt->chunkSize - sizeof( *fmt ) + 8;
        if ( len > 0 ) {
            fseek( f, len, SEEK_CUR );
        }

        /*
         *  reformat the header values
         */
        fmt->formatTag      = (uint16_t)
                                      GET_LSB_FIRST( fmt->formatTag );
        fmt->channels       = (short) GET_LSB_FIRST( fmt->channels );
        fmt->samplesPerSec  = (uint32_t)
                                      GET_LSB_FIRST( fmt->samplesPerSec );
        fmt->avgBytesPerSec = (uint32_t)
                                      GET_LSB_FIRST( fmt->avgBytesPerSec );
        fmt->blockAlign     = (uint16_t)
                                      GET_LSB_FIRST( fmt->blockAlign );
        fmt->bitsPerSample  = (uint16_t)
                                      GET_LSB_FIRST( fmt->bitsPerSample );
#if _DEBUG
        printf( "format tag = %d\nchannels = %d\nsamples/sec = %lu\n"
                "bytes/sec = %lu\nblock align = %u\nbits/sample = %u\n",
                fmt->formatTag,
                fmt->channels,
                fmt->samplesPerSec,
                fmt->avgBytesPerSec,
                fmt->blockAlign,
                fmt->bitsPerSample
              );
#endif        
        if ( fmt->formatTag != 1 ) {
            /*
             *  Can only handle uncompressed data
             */
            goto badfile;
        }
        channels = fmt->channels;
        bitsPerSample = fmt->bitsPerSample;
        samplesPerSec = fmt->samplesPerSec;

        if ( channels == 0 
          || bitsPerSample == 0
          || bitsPerSample > 32
          || samplesPerSec == 0 )
        {
            /*
             *  strange parameters in format chunk
             */
            goto badfile;
        }

        /*
         *  Reserve space for sample buffer
         */
        sampleSize = (uint16_t) ( channels * ( 7 + bitsPerSample ) / 8 );
        file = realloc( file, sizeof( WFILE ) + sampleSize - 4 );
        if ( file == NULL ) {
            goto error;
        }
        data = &( file->header.dataChunk );

        /*
         *  Look for data chunk
         */
        while ( 1 ) {
            /*
             *  Read chunkId and chunkSize
             */
            len = fread( data, 4, 2, f );
            if ( len != 2 ) {
                goto error;
            }

            if ( 0 == memcmp( data->chunkId, "data", 4 ) ) {
                /*
                 *  data chunk found, leave loop
                 */
                break;
            }

            if ( 0 == memcmp( data->chunkId, "fmt ", 4 ) ) {
                /*
                 *  another format chunk found
                 */
                goto badfile;
            }

            /*
             *  Skip unknown chunk
             */
            len = (size_t) GET_LSB_FIRST( data->chunkSize );
            fseek( f, len, SEEK_CUR );
        }
        data->chunkSize = GET_LSB_FIRST( data->chunkSize );
    }
    else {
        /*
         *  Output: fill header
         */
        memcpy( file->header.fileId, "RIFF", 4 );
        file->header.fileSize = sizeof( file->header ) - 8;
        memcpy( file->header.typeId, "WAVE", 4 );

        memcpy( fmt->chunkId, "fmt ", 4 );
        fmt->chunkSize = sizeof( FORMAT_CHUNK ) - 8;
        fmt->formatTag = 1;
        fmt->channels = channels;
        fmt->samplesPerSec = samplesPerSec;
        fmt->avgBytesPerSec = samplesPerSec * sampleSize;
        fmt->blockAlign = sampleSize;
        fmt->bitsPerSample = bitsPerSample;

        memcpy( file->header.dataChunk.chunkId, "data", 4 );
        file->header.dataChunk.chunkSize = 0;

        /*
         *  Write header to file
         *  This is preliminary since values are in native byte order and
         *  counters aren't updated yet
         */
        len = fwrite( &( file->header ), sizeof( file->header ), 1, f );
        if ( len != 1 ) {
            goto error;
        }
    }

    /*
     *  OK
     */
    return file;

    /*
     *  error exits
     */
badfile:
    errno = EBADF;

error:
    if ( f != NULL ) {
        fclose( f );
    }
    if ( file != NULL ) {
        free( file );
    }
    return NULL;
}


/*
 *  Close file after updating the header
 */
int wclose( WFILE *file )
{
    int rc = 0;
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );

    if ( file->openmode == 'w' ) {
        /*
         *  Make size of data chunk even
         */
        if ( ( file->header.fileSize & 1 ) != 0 ) {
            wwrite( file, file->sample, 1 );
        }

        /*
         *  Make all binary values LSB first
         */
        LSB_FIRST( file->header.fileSize );
        LSB_FIRST( file->header.dataChunk.chunkSize );
        LSB_FIRST( fmt->chunkSize );
        LSB_FIRST( fmt->formatTag );
        LSB_FIRST( fmt->channels );
        LSB_FIRST( fmt->samplesPerSec );
        LSB_FIRST( fmt->avgBytesPerSec );
        LSB_FIRST( fmt->blockAlign );
        LSB_FIRST( fmt->bitsPerSample );

        /*
         *  Rewrite header to file
         */
        rc = fseek( file->file, 0L, SEEK_SET );
        if ( rc == 0 ) {
            size_t len = fwrite( &( file->header ), sizeof( file->header ), 1,
                                 file->file );
            if ( len != 1 ) {
                rc = EOF;
            }
        }
    }

    /*
     *  Close file
     */
    if ( rc == 0 ) {
        rc = fclose( file->file );
    }
    else {
        fclose( file->file );
    }
    /*
     *  Free control block memory
     */
    free( file );

    return rc;
}

              
/*
 *  Read samples from the file
 */
int wread( WFILE *file, void *samples, uint32_t numberOfSamples )
{
    size_t len, block;

    block = file->header.formatChunk.blockAlign;

    len = fread( samples, block, numberOfSamples, file->file );
    if ( len != numberOfSamples ) {
        file->isEof = feof( file->file );
        return EOF;
    }
    file->position += numberOfSamples;
    return 0;
}


/*
 *  Read single sample from the file.
 *
 *  The full sample is written to file->sample in LSB-first byte order.
 *
 *  Returns the value of the first sample, first channel.
 *  The value is upscaled to 32 bits, signed.
 *
 *  Check errno for errors if result == -1!
 */
long readSample( WFILE *file )
{
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );
    int rc = wread( file, file->sample, 1 );
    long value = (long) getLsbFirst( file->sample,
                                     fmt->blockAlign / fmt->channels );
    long bias, result;
    int shift;

    if ( rc != 0 ) {
        return -1L;
    }
    errno = 0;

    if ( fmt->bitsPerSample <= 8 ) {
        bias = 128l;
        shift = 24;
    }
    else {
        bias = 0;
        shift = ( 4 - ( fmt->bitsPerSample + 7 ) / 8 ) * 8;
    }

    result = ( value - bias ) << shift;
#if _DEBUG_x
    printf( "[%d->%d]", value, result );
#endif
    return result;
}


/*
 *  Write samples to the file
 */
int wwrite( WFILE *file, void *samples, uint32_t numberOfSamples )
{
    size_t len, block;

    block = file->header.formatChunk.blockAlign;

    len = fwrite( samples, block, numberOfSamples, file->file );
    if ( len != numberOfSamples ) {
        return EOF;
    }

    /*
     *  Adjust header
     */
    len *= block;
    file->header.fileSize += len;
    file->header.dataChunk.chunkSize += len;
    file->position += numberOfSamples;

    return 0;
}


/*
 *  Write a single sample
 *  All channels have the same value
 */
int writeSample( WFILE *file, long value, uint32_t count )
{
    int result = 0;

    storeLsbFirst( (uint32_t) value, file->sample,
                   file->header.formatChunk.blockAlign );
    while ( count-- > 0 && result == 0 ) {
        result = wwrite( file, file->sample, 1 );
    }
    return result;
}


/*
 *  Write a wave with given frequency and number of periods
 *  Amplitude is in percent
 */
int writeWave( WFILE *file, uint32_t freq, uint32_t periods,
               long amplitude, int phase )
{
    /*
     *  Here are some different wave patterns for experimenting
     */
#if 0
    /* sin x + 1/6 * sin 3x */
    static long wave[] = { 0,  62,  100,  110,  115,  115,  110,  100,  62,
                           0, -62, -100, -110, -115, -115, -110, -100, -62 };
#elif 0
    /* sin x */
    static long wave[] = { 0,  43,   82,  110,  125,  125,  110,   82,  43,
                           0, -43,  -82, -110, -125, -125, -110,  -82, -43 };
#elif 1
    /* -sin x */
    static long wave[] = { 0, -43,  -82, -110, -125, -125, -110,  -82, -43,
                           0,  43,   82,  110,  125,  125,  110,   82,  43 };
#elif 0
    /* sin (x+pi/2) */
    static long wave[] = {  125,  110,   82,  43, 0, -43,  -82, -110, -125, 
                           -125, -110,  -82, -43, 0,  43,   82,  110,  125 };
#elif 0
    /* modified sin (x+pi/2) */
    static long wave[] = {  125,  120,   80,  40, 0, -40,  -80, -120, -125, 
                           -125, -120,  -80, -40, 0,  40,   80,  120,  125 };
#elif 0
    /* square */
    static long wave[] = { 125,  125,  125,  125,  125,  125,  125,  125,  125,
                          -125, -125, -125, -125, -125, -125, -125, -125, -125 };
#elif 0
    /* square with offset */
    static long wave[] = { -125, -125, -125, -125, -125,  125,  125,  125,  125,
                            125,  125,  125,  125,  125, -125, -125, -125, -125 };
#elif 0
    /* modified square with less volume */
    static long wave[] = { 96,  96,  96,  96,  96,  96,  96,  96,  0,
                          -96, -96, -96, -96, -96, -96, -96, -96,  0 };
#endif
    long i, j;
    int rc = 0;
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );
    long value, bias;
    int shift;
    int ptr;
    long sps = (long) file->header.formatChunk.samplesPerSec;
    long samples = sps / (long) freq;
    long w1, w2;
    long ampl = file->amplitude == 0 ? amplitude : file->amplitude;
    int sign = wave[ 0 ] < 0 ? -1 : 1;

    if ( fmt->bitsPerSample <= 8 ) {
        bias = 128l;
        shift = 0;
    }
    else {
        bias = 0;
        shift = ( ( ( fmt->bitsPerSample + 7 ) / 8 ) - 1 ) * 8;
    }
#if 0
    for ( j = 0; (uint32_t) j < periods; ++j ) {
        for ( i = 0; i < samples && rc == 0; ++i ) {
            ptr = ( ( i * freq * 18 + sps / 2 ) / sps ) % 18;
            value = ( wave[ ptr ] + bias ) << shift;
            rc = writeSample( file, value, 1 );
        }
    }
#else
    for ( i = 0, ptr = 0; 
          i < (long) (samples * periods) || ptr % 18 < 13; 
          ++i ) 
    {
        j = ( i % sps ) * freq * 18;
        ptr = (int) ( j / sps );
        j -= ptr * sps;
        w1 = wave[ ptr % 18 ] * ( sps - j );
        w2 = wave[ ( ptr + 1 ) % 18 ] * j;
        if ( phase ) {
            w1 = -w1;
            w2 = -w2;
        }
        if ( ( w1 < 0 && sign > 0 )  || ( w1 >= 0 && sign < 0 ) ) {
            ampl = amplitude;
        }
        value = ( -ampl * ( w1 + w2 ) / sps / 100L + bias ) << shift;
        rc = writeSample( file, value, 1 );
        if ( rc != 0 ) break;
    }
#endif
    return rc;
}


/*
 *  Write half a wave with given frequency and phase
 */
int writeHalfWave( WFILE *file, uint32_t freq, int phase )
{
    /*
     *  Here are some different wave patterns for experimenting
     */
#if 0
    /* sin x */
    static long wave[] = { 0,  43,   82,  110,  125,  125,  110,   82,  43 };
#elif 0
    /* sin (x+pi/2) */
    static long wave[] = {  125,  110,   82,  43, 0, -43,  -82, -110, -125 };
#elif 0
    /* modified sin (x+pi/2) */
    static long wave[] = {  125,  120,   80,  40, 0, -40,  -80, -120, -125 };
#elif 0
    /* square */
    static long wave[] = { 125,  125,  125,  125,  125,  125,  125,  125,  125 };
#elif 0
    /* square with offset */
    static long wave[] = { -125, -125, -125, -125, -125,  125,  125,  125,  125 };
#elif 1
    /* modified square with less volume */
    static long wave[] = { 96,  96,  96,  96,  96,  96,  96,  96,  0 };
#endif
    long i, j;
    int rc = 0;
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );
    long value, bias;
    int shift;
    int ptr;
    long sps = (long) file->header.formatChunk.samplesPerSec;
    long samples = sps / (long) freq / 2;
    long w1, w2;

    if ( fmt->bitsPerSample <= 8 ) {
        bias = 128l;
        shift = 0;
    }
    else {
        bias = 0;
        shift = ( ( ( fmt->bitsPerSample + 7 ) / 8 ) - 1 ) * 8;
    }
    for ( i = 0, ptr = 0; 
          i < samples || ptr % 9 < 8; 
          ++i ) 
    {
        j = i * freq * 9 * 2;
        ptr = (int) ( j / sps );
        j -= ptr * sps;
        w1 = wave[ ptr % 9 ] * ( sps - j );
        w2 = wave[ ( ptr + 1 ) % 9 ] * j;
        if ( phase ) {
            w1 = -w1;
            w2 = -w2;
        }
        value = ( ( w1 + w2 ) / sps + bias ) << shift;
        rc = writeSample( file, value, 1 );
        if ( rc != 0 ) break;
    }
    return rc;
}


/*
 *  Write the "turn off" pattern
 */
int writeOff( WFILE *file )
{
    static long wave[] = { 120,  80,  40,   0, -20, -28, -32, -32, -30, 
                           -27, -23, -18, -12,  -8,  -5,  -3,  -2, -1  };
    int i;
    int rc = 0;
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );
    long value, bias;
    int shift;
    int ptr;
    int sps = file->header.formatChunk.samplesPerSec;
    int samples = sps / 600;

    if ( fmt->bitsPerSample <= 8 ) {
        bias = 128l;
        shift = 0;
    }
    else {
        bias = 0;
        shift = ( ( ( fmt->bitsPerSample + 7 ) / 8 ) - 1 ) * 8;
    }

    for ( i = 0; i < samples && rc == 0; ++i ) {
        ptr = ( ( i * 600 * 18 + sps / 2 ) / sps ) % 18;
        value = ( - wave[ ptr ] + bias ) << shift;
        rc = writeSample( file, value, 1 );
    }
    return rc;
}


/*
 *  Write silence
 */
int writeSilence( WFILE *file, uint32_t samples )
{
    unsigned int i;
    int rc = 0;
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );
    long value;

    if ( fmt->bitsPerSample <= 8 ) {
        value = 128l;
    }
    else {
        value = 0;
    }

    for ( i = 0; i < samples && rc == 0; ++i ) {
        rc = writeSample( file, value, 1 );
    }
    return rc;
}


/*
 *  Return file position in milliseconds
 */
uint32_t wtell( WFILE *file )
{
    FORMAT_CHUNK *fmt = &( file->header.formatChunk );
    double seconds = (double) file->position / (double) fmt->samplesPerSec;
    return (uint32_t) ( 1000. * seconds );
}


/*
 *  Threshold for low/high detection
 *  Can be set with environment variable WAVE_THRESHOLD
 */
static int Threshold = 10;

/*
 *  Automatic bias detection (experimental)
 *  Can be enabled with environment variable WAVE_AUTOBIAS
 */
static int AutoBias = 0;


/*
 *  Open a wave file for "Kansas City Standard" (KCS) encoded data
 *
 *  baudrate is 300, 600 or 1200, negative means inverted phase
 *  bits is 7 or 8
 *  parity is one of 'E', 'O' or 'N'
 *  stopbits is 1 or 2
 */
KCS_FILE *kcsOpen( char *filename,
                   char *openmode,
                   int baudrate,
                   int bits,
                   char parity,
                   int stopbits )
{
    KCS_FILE *file;
    WFILE *wfile;
    char *p;
    int phase;

    /*
     *  get threshold from environment
     */
    p = getenv( "WAVE_THRESHOLD" );
    if ( p != NULL ) {
        Threshold = atoi( p );
        if ( Threshold == 0 ) {
            Threshold = 10;
        }
    }

    /*
     *  get autobias flag from environment
     */
    p = getenv( "WAVE_AUTOBIAS" );
    if ( p != NULL ) {
        AutoBias = ( 0 != atoi( p ) );
    }

    /*
     *  get phase from baudrate or environment
     *  0: high-low-high
     *  1: low-high-low
     */
    if ( baudrate < 0 ) {
        phase = 1;
        baudrate = -baudrate;
    }
    else if ( NULL != ( p = getenv( "WAVE_PHASE" ) ) ) {
        phase = ( 0 != atoi( p ) );
    }
    else {
        phase = 0;
    }

    if ( (  baudrate != 300 && baudrate != 600
         && baudrate != 1200 && baudrate != 2400
         && baudrate != 500 && baudrate != 1500
         && baudrate != 1400 )
      || ( bits != 7 && bits != 8 && bits != 4 )
      || ( parity != 'E' && parity != 'O' && parity != 'N' )
      || ( stopbits < 0 || stopbits > 4 ) )
    {
        errno = EINVAL;
        return NULL;
    }

    wfile = wopen( filename, openmode, 1, 8,
                   baudrate ==  500 ? 16000
                 : baudrate == 2400 ? 44100 
                                    : 22050 );
    if ( wfile == NULL ) {
        return NULL;
    }

    /*
     *  Fill control block (add some space for sampleBuffer )
     */
    file = malloc( sizeof( KCS_FILE ) + 400 );
    if ( file == NULL ) {
        wclose( wfile );
        return NULL;
    }
    file->file               = wfile;
    file->baudrate           = baudrate;
    file->bits               = bits;
    file->parity             = parity;
    file->stopbits           = stopbits;
    file->state              = KCS_UNKNOWN;
    file->carrier            = baudrate ==  500 ? 4000    /* Sharp Basic */
                             : baudrate == 1500 ? 2100    /* Sharp el-9x00 */
                             : baudrate == 1400 ? 1400    /* TI-74 */
                             : baudrate == 2400 ? 4800    /* high speed */
                                                : 2400;   /* normal or fast */
    file->bitLength          = wfile->header.formatChunk.samplesPerSec
                               / baudrate;
    if ( file->carrier == baudrate ) {
        /*
         *  TI: only half of a wave used on low frequency
         */
        file->wavesPerZero = 1;
        file->wavesPerOne  = 2;
        file->halfWave = 1;
        file->singleWave = 0;
        file->syncOnZero = 1;
    }
    else if ( file->carrier < 2 * baudrate ) {
        /*
         *  Sharp el-9x00: one wave per bit, uneven bit length
         */
        file->bitLength          = wfile->header.formatChunk.samplesPerSec
                                 / ( file->carrier / 2 );
        file->wavesPerZero = 1;
        file->wavesPerOne  = 1;
        file->halfWave = 0;
        file->singleWave = 1;
        file->syncOnZero = 0;
    }
    else {
        file->wavesPerZero       = file->carrier / 2 / baudrate;
        file->wavesPerOne        = file->carrier / baudrate;
        file->halfWave = 0;
        file->singleWave = 0;
        file->syncOnZero = 0;
    }
    file->zeroWaveLength     = file->bitLength / file->wavesPerZero;
    file->syncWaves          = 0;
    file->raw                = 0;
    file->lastWave           = 0;
    file->phase              = phase;
    file->sampleBufferLength = AutoBias ? file->zeroWaveLength * MIN_GOOD : 1;
    file->samplePointer      = 0;
    file->bias               = 0L;
    file->good_count         = 0;

    /*
     *  Readjust size of control block
     */
    file = realloc( file, sizeof( KCS_FILE ) 
                        + sizeof( long ) * ( file->sampleBufferLength - 1 ) );
    if ( file == NULL ) {
        wclose( wfile );
        return NULL;
    }
#if _DEBUG
    printf( "kcsOpen: samples/sec=%lu, bitLength=%d, zeroLength=%d,"
            " syncOnZero=%d\n",
             file->file->header.formatChunk.samplesPerSec, 
             file->bitLength, file->zeroWaveLength,
             file->syncOnZero );
#endif
    return file;
}


/*
 *  Close KCS file
 */
int kcsClose( KCS_FILE *file )
{
    int rc = 0;

    if ( file->file->openmode == 'w' ) {
        /*
         *  Add lead-out and silence
         */
        if ( !file->raw ) {
            rc = kcsLeadOut( file, 4 );
        }
        if ( rc == 0 ) {
            rc = writeOff( file->file );
        }
        if ( rc == 0 ) {
            rc = writeSample( file->file, 128, 256 );
        }
    }

    /*
     *  Close wave file
     */
    if ( rc == 0 ) {
        rc = wclose( file->file );
    }
    else {
        wclose( file->file );
    }

    /*
     *  Free control block memory
     */
    free( file );

    return rc;
}


/*
 *  Read single sample from the file and normalize it.
 *
 *  Returns
 *   0 - value is < -1/Threshold
 *   1 - value is in [-1/Threshold, +1/Threshold)
 *   2 - value is >= +1/Threshold
 *  KCS_ERROR (-1) - error, check errno
 *  KCS_EOF   (-2) - end of file
 */
static int kcsReadSample( KCS_FILE *file )
{
    long sample = readSample( file->file );

    if ( sample == -1L ) {
        if ( file->file->isEof ) {
            return KCS_EOF;
        }
        if ( errno != 0 ) {
            return KCS_ERROR;
        }
    }
    sample /= 2;
    if ( file->samplePointer < file->sampleBufferLength ) {
        file->sampleBuffer[ file->samplePointer++ ] = sample;
    }

    /*
     *  map sample to 0/1/2
     */
    sample -= file->bias;
    if ( sample < - ( 0x3fffffffl / Threshold ) ) {
        return file->phase ? 2 : 0;
    }
    if ( sample < 0x3fffffffl / Threshold ) {
        return 1;
    }
    return file->phase ? 0 : 2;
}


/*
 * Update the bias from stored wave pattern
 */
static void kcsUpdateBias( KCS_FILE *file )
{
    int i;

    if ( !AutoBias ) return;

    file->bias = 0;
    for ( i = 0; i < file->samplePointer; ++i ) {
        file->bias += file->sampleBuffer[ i ] / file->samplePointer;
        file->sampleBuffer[ i ] = 0;
    }
    file->samplePointer = 0;
    file->good_count = 0;
#if _DEBUG_
    printf( "<bias=%ld>", file->bias );
#endif
}


/*
 *  Read half of a wave and return the length in samples
 *
 *  Returns
 *
 *  n > 0 : half wavelength
 *  KCS_ERROR    (-1) - error, check errno
 *  KCS_EOF      (-2) - end of file
 */
static int kcsReadHalfWave( KCS_FILE *file )
{
    int result = 0;
    int sample, lastSample;
    int direction = 0;

    /*
     *  Look for sign change
     */
    while ( 1 ) {
        sample = kcsReadSample( file );
        lastSample = file->lastSample;
        file->lastSample = sample;
        ++result;
#if _DEBUG_
        printf( "%d", sample );
#endif
        if ( sample < 0 ) {
            /*
             *  error or eof
             */
            if ( result > 1 ) {
                return result;
            }
            return sample;
        }

        if ( direction == 0 ) {
            /*
             *  determine the direction of the wave
             */
            if ( lastSample > sample || sample == 0 ) {
                direction = -1;
            }
            else if ( lastSample < sample || sample == 2 ) {
                direction = 1;
            }
        }
        else if ( direction > 0 ) {
            /*
             *  direction positive, look for negative sample
             */
            if ( sample == 0 ) {
                break;
            }
        }
        else {
            /*
             *  direction negative, look for positive sample
             */
            if ( sample == 2 ) {
                break;
            }
        }
    }
#if _DEBUG_
    printf( " kcsReadHalfWave: %d at %d\n", result, wtell( file->file ) );
#endif
    return result;
}


/*
 *  Read a single wave and return the wavelength in samples
 *
 *  Returns
 *
 *  n > 0 : wavelength
 *  KCS_ERROR    (-1) - error, check errno
 *  KCS_EOF      (-2) - end of file
 */
static int kcsReadWave( KCS_FILE *file )
{
    int result = 0;
    int sample;
    int lowDetected = 0;
    int highDetected = 0;

    if ( file->halfWave ) {
        /*
         *  Need a special algorithm for the TI
         */
        return kcsReadHalfWave( file );
    }
    /*
     *  Look for sign change
     */
    while ( 1 ) {
        if ( !highDetected && file->lastSample == 2 ) {
            /*
             *  On high frequency files we may miss a high sample!
             */
            sample = 2;
            file->lastSample = 0;
            result = -1;
        }
        else {
            sample = kcsReadSample( file );
            file->lastSample = sample;
        }
#if _DEBUG_
        printf( "%d", sample );
#endif
        if ( sample < 0 ) {
            /*
             *  error or eof
             */
            if ( result > 1 ) {
                return result;
            }
            return sample;
        }

        if ( sample != 2 && highDetected == 0 ) {
            /*
             *  skip all low bytes at the beginning of the wave
             */
            continue;
        }
        ++result;

        if ( sample == 2 ) {
            /*
             *  at least one high value detected
             */
            highDetected = 1 + lowDetected;
        }
        else if ( sample == 0 ) {
            /*
             *  at least one low value detected
             */
            lowDetected = 1;
        }

        if ( lowDetected == 1 && highDetected == 2 ) {
            /*
             *  high low high sequence detected
             */
            break;
        }
#if _DEBUG_x
        printf( "<l=%d,h=%d>", lowDetected, highDetected );
#endif
    }
#if _DEBUG_
    printf( " kcsReadWave: %d at %d\n", result, wtell( file->file ) );
#endif
    return result;
}


/*
 *  Read a KCS coded bit
 *
 *  Returns
 *     0, 1 - bit successfully decoded
 *     KCS_ERROR    (-1) - file error, check errno
 *     KCS_EOF      (-2) - end of file
 *     KCS_BAD_DATA (-3) - no appropriate wave pattern found
 */
int kcsReadBit( KCS_FILE *file )
{
    int waveLength;
    int sampleCount = 0;
    int isZero;
    int isOne;
    int isBetween;
    int zeroMin = file->zeroWaveLength - 4;
    int zeroMax = file->zeroWaveLength + 4;
    int oneMin = ( file->zeroWaveLength + 1 ) / 2 - 2;
    int oneMax = ( file->zeroWaveLength + 1 ) / 2 + 2;
    int countZero = 0;
    int countOne = 0;

    while ( oneMax >= zeroMin ) {
        /*
         *  Keep them farther apart
         */
        ++zeroMin;
    }

    while ( file->state == KCS_UNKNOWN || sampleCount < file->bitLength )
    {
        /*
         *  Read a single wave
         */
        if ( file->lastWave > 0 ) {
            /*
             *  we have pushed back the last wave length
             */
            waveLength = file->lastWave;
            file->lastWave = 0;
        }
        else {
            /*
             *  read from file
             */
            waveLength = kcsReadWave( file );
            if ( waveLength < 0 ) {
                /*
                 *  error or eof
                 */
                if ( sampleCount == 0 ) {
                    file->state = KCS_UNKNOWN;
                    file->syncWaves = 0;
                    return waveLength;
                }
                else {
                    break;
                }
            }
        }

        /*
         *  find out what it is
         */
        isOne     = waveLength >= oneMin  && waveLength <= oneMax;
        isBetween = waveLength >  oneMax  && waveLength <  zeroMin;
        isZero    = waveLength >= zeroMin && waveLength <= zeroMax;
#if _DEBUG
        printf( "\tkcsReadBit: waveLength=%2d, "
                "isZero=%d, isBetween=%d, isOne=%d, good=%d\n",
                 waveLength, isZero, isBetween, isOne, file->good_count );
#endif
        if ( isZero || isOne || isBetween ) {
            if ( ++(file->good_count) >= MIN_GOOD ) {
                kcsUpdateBias( file );
            }
        }
        else {
            file->bias = 0;
            file->samplePointer = 0;
            file->good_count = 0;
        }

        if ( isBetween ) {
            /*
             *  Looks like we are out of phase
             */
            if ( file->lastFreq == FRQ_LOW ) {
                /*
                 *  Frequency shift low->high
                 *  Treat as low frequency (logical zero)
                 */
                isZero = 1;
            }
            else if ( file->lastFreq == FRQ_HIGH ) {
                /*
                 *  Frequency shift high->low
                 *  Treat as high frequency (logical one)
                 */
                isOne = 1;
            }
            else {
                /*
                 *  Value can't be used
                 */
                file->lastFreq = FRQ_UNKNOWN;
            }
        }

        if ( isZero ) {
            /*
             *  Store as a compare value for next wave
             */
            file->lastFreq = FRQ_LOW;

            if ( file->syncOnZero ) {
                /*
                 *  Check if lead-in
                 */
                if ( file->state == KCS_UNKNOWN 
                    && ++file->syncWaves >= 100 * file->wavesPerZero )
                {
                    /*
                     *  Lead-in found
                     *  Start over with this bit
                     */
                    file->state = KCS_SYNCHING;
#if _DEBUG
                    printf( "\nSynching at %lu\n", wtell( file->file ) );
#endif
                    return 0;
                }
#if _DEBUG_
                printf( "\tkcsReadBit: state=%d, syncWaves=%ld ?> %ld\n",
                         file->state, file->syncWaves, 
                         100 * file->wavesPerZero );
#endif
            }
            else if ( file->state == KCS_UNKNOWN ) {
                /*
                 *  Ignore
                 */
                continue;
            }
            else if ( file->state == KCS_SYNCHING && countZero == 0 ) {
                /*
                 *  Begin of first startbit found
                 *  Start over with this bit
                 */
                file->state = KCS_SYNCHED;
                sampleCount = 0;
                countOne = 0;
#if _DEBUG
                printf( "\nSynched at %lu\n", wtell( file->file ) );
#endif
            }
            if ( !file->syncOnZero ) {
                file->syncWaves = 0;
            }
            ++countZero;
            if ( countZero == 1 ) {

                if ( countOne == 1 ) {
                    /*
                     *  Discard a stray "one" wave and start over
                     */
#if _DEBUG
                    printf( "\nDeleted 1 at %lu\n", wtell( file->file ) );
#endif
                    sampleCount = 0;
                    countOne = 0;
                }
                else if ( countZero > 1 ) {
                    /*
                     *  We have overshot
                     */
                    file->lastWave = waveLength;
                    break;
                }
            }
        }
        else if ( isOne ) {
            /*
             *  Store as a compare value for next wave
             */
            file->lastFreq = FRQ_HIGH;

            /*
             *  Check if lead-in
             */
            if ( file->syncOnZero ) {
                /*
                 *  TI mode with a burst of ones after a zero lead-in
                 */
                if ( file->state == KCS_SYNCHING && countOne == 0 ) {
                    /*
                     *  Begin of first nonzero bit found
                     *  Start over with this bit
                     */
                    file->state = KCS_SYNCHED;
                    sampleCount = 0;
                    countZero = 0;
#if _DEBUG
                    printf( "\nSynched at %lu\n", wtell( file->file ) );
#endif
                }
            }
            else if ( file->state == KCS_UNKNOWN || file->state == KCS_SYNCHED ) 
            {
                /*
                 *  Standard KCS mode
                 */
                if ( ++file->syncWaves >= 12 * file->wavesPerOne * 2 ) {
                    /*
                     *  Lead-in found
                     *  Start over with this bit
                     */
                    file->state = KCS_SYNCHING;
#if _DEBUG
                    printf( "\nSynching at %lu\n", wtell( file->file ) );
#endif
                    return 1;
                }
            }
            ++countOne;
            if ( countOne == 1 ) {

                if ( countZero == 1 ) {
                    /*
                     *  Discard a stray "zero" wave and start over
                     */
#if _DEBUG
                    printf( "\nDeleted 0 at %lu\n", wtell( file->file ) );
#endif
                    sampleCount = 0;
                    countZero = 0;
                }
                else if ( countZero > 1 ) {
                    /*
                     *  We have overshot
                     */
                    file->lastWave = waveLength;
                    break;
                }
            }
        }
        else if ( waveLength > zeroMax ) {
            /*
             *  Synchronization lost
             */
#if _DEBUG
            if ( file->state != KCS_UNKNOWN ) {
                printf( "\nSynch lost at %lu\n", wtell( file->file ) );
            }
#endif
            file->state = KCS_UNKNOWN;
            sampleCount = 0;
            file->syncWaves = 0;
            countZero = countOne = 0;
            continue;
        }

        /*
         *  Compute total length
         */
        sampleCount += waveLength;

        /*
         *  check if it's a zero (exactly)
         */
        if ( countOne == 0 && countZero == file->wavesPerZero ) {
            return 0;
        }

        /*
         *  check if it's a one (exactly)
         */
        if ( countZero == 0 && countOne == file->wavesPerOne ) {
            return 1;
        }
    }

#if _DEBUG
    printf( "kcsReadBit: sampleCount=%d (%d) countOne=%d, countZero=%d\n",
             sampleCount, file->bitLength * 125 / 100, countOne, countZero );
#endif
    /*
     *  check if maximum length is exeeded
     */
    if ( sampleCount > file->bitLength * 125 / 100 ) {
        file->state = KCS_UNKNOWN;
        file->syncWaves = 0;
        return KCS_BAD_DATA;
    }

    /*
     *  check if it's a zero (with jitter)
     */
    if ( countOne <= 3
         && ( countZero >= file->wavesPerZero - 1
           || countZero <= file->wavesPerZero + 1 ) )
    {
        return 0;
    }

    /*
     *  check if it's a one (with jitter)
     */
    if ( countZero <= 2
         && ( countOne >= file->wavesPerOne - 2
           || countOne <= file->wavesPerOne + 2 ) )
    {
        return 1;
    }
    return KCS_BAD_DATA;
}


/*
 *  Read KCS coded data in uncooked form
 *
 *  Returns
 *     Positive 16 bit word - data successfully read
 *     KCS_ERROR    (-1)    - file error, check errno
 *     KCS_EOF      (-2)    - end of file
 *     KCS_BAD_DATA (-3)    - no appropriate wave pattern found
 */
short kcsReadRaw( KCS_FILE *file )
{
    short result = 0;
    int bits = ( file->stopbits != 0 ) + file->bits 
             + ( file->parity != 'N' ) + file->stopbits;
    int i;

#if _DEBUG
    printf( "\nkcsReadRaw(%d): ", bits );
#endif

    for ( i = 0; i < bits; ++i ) {
        /*
         *  Read next bit
         */
        int oldState = file->state;
        int bit = kcsReadBit( file );
        if ( bit < 0 ) {
            return bit;
        }
#if _DEBUG
        putchar( bit + '0' ); putchar( '.' );
#endif
        if ( file->state == KCS_SYNCHED ) {
            /*
             *  Within valid data
             */
            if ( file->stopbits != 0 ) {
                /*
                 *  Asynchronous format
                 */
                if ( i == 0 && bit == 1 ) {
                    /*
                     *  wait for start bit
                     */
                    i = -1;
                    continue;
                }
            }
            if ( oldState == KCS_SYNCHING ) {
                /*
                 *  First valid bit after lead-in
                 */
                i = 0;
                result = 0;
            }
        }

        /*
         *  Insert bit into result
         */
        result |= (short) ( bit << i );
    }

    return result;
}


/*
 *  Read KCS coded data in uncooked ASCII armoured form
 *  - The lower 6 bits are transfered to the lower byte and 0x30 is added
 *  - The remaining bits are transfered to the upper byte and 0x30 is added
 *
 *  Returns
 *     Positive 16 bit word - data successfully read
 *     KCS_ERROR    (-1)    - file error, check errno
 *     KCS_EOF      (-2)    - end of file
 *     KCS_BAD_DATA (-3)    - no appropriate wave pattern found
 */
short kcsReadAscii( KCS_FILE *file )
{
    short raw;

    /*
     *  read raw data
     */
    raw = kcsReadRaw( file );
    if ( raw < 0 ) {
        return raw;
    }

    /*
     *  reformat result
     */
    return (short) ( ( raw & 0x3f ) + 0x30
                   | ( ( raw << 2 ) & 0x3f00 ) + 0x3000
                   );
}


/*
 *  Read a KCS coded byte with parity and framing info
 *
 *  Returns
 *     Positive 16 bit word - data successfully decoded
 *     Parity is always moved to bit 8
 *     KCS_PARITY  (Bit 14) is set on parity error
 *     KCS_FRAMING (Bit 13) is set on framing error
 *     KCS_LEAD_IN (0x7fff) - lead-in character
 *     KCS_ERROR    (-1)    - file error, check errno
 *     KCS_EOF      (-2)    - end of file
 *     KCS_BAD_DATA (-3)    - no appropriate wave pattern found
 */
short kcsReadByte( KCS_FILE *file )
{
    short result = 0;
    short raw;
    int bits = ( file->stopbits != 0 ) + file->bits 
             + ( file->parity != 'N' ) + file->stopbits;
    int i;
    int parity = 0;
    int framing = 0;
    int shift = 0;

    /*
     *  read raw data
     */
    raw = kcsReadRaw( file );
    if ( raw < 0 ) {
        return raw;
    }

    /*
     *  reformat result
     */
    if ( file->state == KCS_SYNCHED ) {
        /*
         *  Valid data
         */
        for ( i = 0; i < bits; ++i ) {
            /*
             *  Get next bit
             */
            int bit = raw & 1;
            raw >>= 1;

            if ( file->stopbits == 0 ) {
                /*
                 *  Insert bit into result - synchronous, high to low
                 */
                result |= (short) ( bit << ( 8 - ++shift ) );
            }
            else if ( i > 0 ) {
                /*
                 *  Insert bit into result - asynch, low to high
                 */
                result |= (short) ( bit << shift );
                ++shift;
                if ( shift >= file->bits && shift < 8 ) {
                    /*
                     *  Parity and stopbits go in msb
                     */
                    shift = 8;
                }
            }
            /*
             *  Check data format
             */
            if ( i >= bits - file->stopbits ) {
                if ( bit != 1 ) {
                    framing = 1;
                }
            }
            else {
                parity ^= bit;
            }
        }

        /*
         *  check parity
         */
        if ( ( file->parity == 'E' && parity == 1 )
          || ( file->parity == 'O' && parity == 0 ) )
        {
            result |= KCS_PARITY;
#if _DEBUG
            printf( " Parity error " );
#endif
        }
        if ( framing ) {
            result |= KCS_FRAMING;
#if _DEBUG
            printf( " Framing error " );
#endif
        }
    }
    else {
        /*
         *  lead-in character
         */
        result = KCS_LEAD_IN;
    }
    return result;
}


/*
 *  Write a KCS lead-in sequence of given length (in tens of seconds)
 */
int kcsLeadIn( KCS_FILE *file, unsigned int tensOfSecs )
{
    uint32_t freq = file->carrier;
    uint32_t periods = tensOfSecs * ( freq / 10ul );
    int result = 0;
    uint32_t i;

    if ( file->halfWave ) {
        /*
         *  TI-Mode
         */
        for ( i = 0; i < periods && result == 0; ++i ) {
            result = writeHalfWave( file->file, freq / 2, file->phase );
            file->phase = !file->phase;
        }
    }
    else {
        result = writeWave( file->file, freq, periods, 80, file->phase );
    }
    return result;
}


/*
 *  Write a KCS lead-out sequence of given length (in periods)
 */
int kcsLeadOut( KCS_FILE *file, unsigned int periods )
{
    uint32_t freq = file->carrier / 2;

    return writeWave( file->file, freq, periods, 100, file->phase );
}


/*
 *  Write a "Kansas City Standard" (KCS) encoded bit to the file
 */
int kcsWriteBit( KCS_FILE *file, int bit )
{
    uint32_t freq    = bit ? file->carrier : file->carrier / 2;
    uint32_t periods = file->singleWave ? 1 : freq / file->baudrate;
    uint32_t i;
    int amplitude = bit ? 80 : 100;
    int result = 0;

    if ( file->halfWave ) {
        /*
         *  TI-Mode
         */
        periods = bit ? 2 : 1;
        for ( i = 0; i < periods && result == 0; ++i ) {
#if _DEBUG_
            printf( "frequency=%lu\n", freq );
#endif
            result = writeHalfWave( file->file, freq, file->phase );
            file->phase = !file->phase;
        }
    }
    else {
        result = writeWave( file->file, freq, periods, amplitude, file->phase );
    }
    return result;
}


/*
 *  Write a KCS encoded byte to the file
 */
int kcsWriteByte( KCS_FILE *file, unsigned char byte )
{
    int i;
    int rc;
    int parity = file->parity == 'E' ? 0 : 1;
    int bit;

    /*
     *  startbit
     */
    rc = kcsWriteBit( file, 0 );
    
    /*
     *  databits, lsb first
     */
    for ( i = 0; i < file->bits && rc == 0; ++i, byte >>= 1 ) {
        bit = byte & 1;
        parity ^= bit;
        rc = kcsWriteBit( file, bit );
    }

    /*
     *  parity
     */
    if ( file->parity != 'N' && rc == 0 ) {
        rc = kcsWriteBit( file, parity );
    }

    /*
     *  stopbits, lsb first
     */
    for ( i = 0; i < file->stopbits && rc == 0; ++i ) {
        rc = kcsWriteBit( file, 1 );
    }

    return rc;
}


/*
 *  Make sure that a value is stored LSB first
 */
void storeLsbFirst( uint32_t val, void *dest, int length )
{
    unsigned char *ptr = (unsigned char *) dest;

    while ( length-- > 0 ) {
        *ptr++ = (unsigned char) ( val & 0xff );
        val >>= 8;
    }
}


/*
 *  Read a value which is stored LSB first
 */
uint32_t getLsbFirst( void *src, int length )
{
    uint32_t val = 0;
    unsigned char *ptr = (unsigned char *) src;
    int shift = 0;

    while ( length-- > 0 ) {
        val |= ( (uint32_t) *ptr++ << shift );
        shift += 8;
    }
    return val;
}


