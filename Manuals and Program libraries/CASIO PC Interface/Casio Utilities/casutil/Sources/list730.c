/*
 *  list730.c - List Casio tape files
 *
 *  This program displays the contents of a tape image saved by the PB-100, 
 *  PB-220, FX-700P up to FX-730P calculators through the sound card or with
 *  Piotr's serial interface.
 *  Only program and memo files supported.
 *
 *  Options:
 *    -b<skip> read BIN file (Default); skip <skip> garbage bytes
 *    -a       read ASCII file (Piotr's format)
 *    -w       read WAV file
 *
 *  Original version by Piotr Piatek
 *  Enhancements and wave file support by Marcus von Cube
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bool.h"
#include "wave.h"

/*
 *  Error messages 
 */
const char *err_msg[] = 
{
    "",                                       /* OK */
    "leader expected",                        /* message #1 */
    "premature end of a header/data segment", /* message #2 */
    "premature end of a file",                /* message #3 */
    "header segment expected",                /* message #4 */
    "unknown segment identifier",             /* message #5 */
    "unexpected program separator"            /* message #6 */
};

/*
 *  File types 0x9_ to 0xF_ 
 */
const char *file_type[] = 
{
    "Memo",
    "Memo",         /* password protected */
    "All programs", /* password protected */
    "Program",      /* password protected */
    "Program",
    "Variables",
    "All programs"
};

/*
 *  Character codes 0x00 to 0x7F 
 */
const char characters[] =
    " +-*/^!\""
            "#$> = < 0123456789. )( E"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZDLg~s "
    "abcdefghijklmnopqrstuvwxyz`'?,;:"
    "oS`^@x:S{HDCuO|}%Y0[&_'.]#\\"
                                "#/bt";

/*
 *  Replacement for some special characters
 *  Only outside strings and comments
 */
struct _specials {
    char *text;
    unsigned char token;
} specials[] = 
{
    { " ",  0x00 },
    { ">=", 0x0B },
    { "<=", 0x0D },
    { "<>", 0x0F },
    { "PI", 0x1B },
    { "E-", 0x1E },
    { "-1", 0x3F },
    { ":",  0xFE },
    { "\n", 0xFF },
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
const char *tokens[] = 
{
    "SIN ",     "COS ",     "TAN ",     "ASN ",    /* 80 - 83 */
    "ACS ",     "ATN ",     "LOG ",     "LN ",     /* 84 - 87 */
    "EXP ",     "SQR ",     "INT ",     "FRAC ",   /* 88 - 8B */
    "ABS ",     "SGN ",     "RND(",     "RAN#",    /* 8C - 8F */
    "LEN(",     "VAL(",     "MID(",     "KEY ",    /* 90 - 93 */
    "CSR ",     " TO ",     " STEP ",   " THEN ",  /* 94 - 97 */
    "DEG(",     "HEX$(",    "STR$(",    "DMS$(",   /* 98 - 9B */
    "MID$(",    "ALL ",     "BEEP ",    "LET ",    /* 9C - 9F */
    "FOR ",     "NEXT ",    "GOTO ",    "GOSUB ",  /* A0 - A3 */
    "RETURN ",  "IF ",      "PRINT ",   "INPUT ",  /* A4 - A7 */
    "MODE ",    "STOP ",    "END ",     "ON ",     /* A8 - AB */
    "READ ",    "RESTORE ", "DATA ",    "REM ",    /* AC - AF */
    "VAC ",     "SET ",     "PUT ",     "GET ",    /* B0 - B3 */
    "CLEAR ",   "DEFM ",    "&H",       "CUR ",    /* B4 - B7 */
    "POL(",     "HYP",      "REC(",     "FACT ",   /* B8 - BB */
    "NPR(",     "EOX ",     "NCR(",     "EOY ",    /* BC - BF */
    "\\C0",     "SAVE ",    "LOAD ",    "VERIFY ", /* C0 - C3 */
    "LIST ",    "RUN ",     "NEW ",     "PASS ",   /* C4 - C7 */
    "SRAM ",    "LRAM ",    "WRITE#",   "STAT ",   /* C8 - CB */
    "SAVE ",    "^2",       "^3",       "10^",     /* CC - CF */
    "DIM ",     "ERASE "                           /* D0 - D1 */
};


/*
 *   Flags for binary mode and escape mode
 */
bool BinMode = TRUE;
bool EscapeMode = TRUE;

void casioprint( int c )
{
    struct _specials *sp;
    static bool in_string = FALSE;
    static bool transparent = FALSE;
    static bool space_pending = FALSE;
    int l;
    const char *p;

    if ( c == 0x07 ) {
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
        transparent = FALSE;
        space_pending = FALSE;
    }
    if ( space_pending && !transparent && !in_string ) {
        /*
         *  Print pending space before keywords, characters and numbers
         */
        if ( c >= 0x10 && c <= 0x1B 
          || c >= 0x20 && c <= 0x5B
          || c >= 0x60 )
        {
            putchar( ' ' );
        }
    }
    space_pending = FALSE;

    if ( ( transparent || in_string ) && EscapeMode && c <= 0x7F ) {
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

    if ( c <= 0x7F ) {
        /*
         *  Print other characters or tokens
         */
        if ( EscapeMode && characters[ c ] == ' ' ) {
            printf( "\\%2.2X", c );
        }
        else {
            printf( "%c", characters[ c ] );
        }
    }
    else if ( !in_string && !transparent && c <= 0xD1 ) {
        /*
         *  Token
         */
        if ( !in_string && ( c == 0xAE || c == 0xAF ) ) {
            /*
             *  DATA or REM
             */
            transparent = TRUE;
        }
        l = strlen( p = tokens[ c - 0x80 ] );
        if ( l > 0 && !transparent && p[ l - 1 ] == ' ' ) {
            --l;
            space_pending = TRUE;
        }
        printf( "%.*s", l, tokens[ c - 0x80 ] );
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


int bcd2bin (int c)
{
  int x;
  x = c/16;
  return c - 6*x;
}


/* returns 0 when OK or the error code when invalid data encountered */
int list( int c )
{
  static int skipped = 5;
  static int idle_counter = 0;
  static int data_counter = 0;
  static int line_counter = 0;
  static int line_number = 0;
  static int prog_number = 0;
  static int segment_id = 0;
  static int filename_length = 0;
  static int file_id = 0;
  static bool leader_expected = TRUE;
  static bool dataseg_expected = FALSE;
  static bool password_expected = FALSE;

  if ( !BinMode ) {

  /* skip first 5 bytes before the leader */
    if (skipped>0)
    {
      skipped--;
      return 0;
    }

  /* handle the leader */
    if (c == KCS_LEAD_IN) /* idle string (leader) */
    {
      if (idle_counter==0 && !leader_expected)
      {
        return 2;   /* "Premature end of a header/data segment" error */
      }
      if (idle_counter++ >= 12) /* leader should contain at least 12 idle strings */
      {
        leader_expected = FALSE;  /* valid leader detected */
      }
      data_counter = 0;
      return 0;
    }

  /* data byte received */
    idle_counter = 0;
    if (leader_expected)
    {
      return 1;   /* "Leader expected" error */
    }
    c &= 0xff;
  }
  else {
    /* BinMode */
    if ( leader_expected ) {
      if ( !dataseg_expected && ( c & 0xf0 ) >= 0x90 
           && ( ( c & 0x0f ) <= 8 || ( c & 0x0f ) == 0x0F )
           || c == 0x02 )
      {
        /* valid header byte encountered */
        data_counter = 0;
        leader_expected = FALSE;
      }
      else {
        /* 
        printf( "Skipped\n", c );
        */
        return 0;
      }
    }
  }

  /* printf( "[%02.2X]", c ); */

/* handle the first segment byte */
  if (++data_counter == 1)
  {
    segment_id = c;

    if (dataseg_expected)
    {
      if (c == 0x02)    /* data segment ? */
      {
        line_counter = 0;
        return 0;
      }
      return 3;     /* "Premature end of a file" error */
    }

/* header segment expected */
    if (c == 0x02)    /* data segment ? */
    {
      return 4;     /* "Header segment expected" error */
    }

    if ((file_id = c & 0xF0) >= 0x90)
    {
      prog_number = 0;
      printf ("%s: \"", file_type[file_id/16 - 0x09]);
      password_expected = (file_id <= 0xC0 && file_id >= 0xA0);
      filename_length = ( c & 0x0F ) == 0x0F ? 0 : ( c & 0x0F );
      if ( filename_length == 0 ) putchar( '"' );
      return 0;
    }

    return 5;     /* "Unknown segment identifier" error */
  }

/* handle subsequent segment bytes */

/* handle the data segment */
  if (segment_id == 0x02) /* data segment ? */
  {
/* End Marker ? */
    if (c == 0xF0 || c == 0xF1)
    {
      leader_expected = TRUE;
      skipped = 5;
      dataseg_expected = (c == 0xF1);
      password_expected = FALSE;
      return 0;
    }

/* password segment ? */
    if (password_expected)
    {
      if (data_counter == 2)
      {
        printf ("Password: \"");
      }
      else if ( c == 0xFF ) {
        printf( "\"\n" );
      }
      casioprint(c);
      return 0;
    }

/* P0 ? */
    if ( data_counter == 2 && (file_id == 0xF0 || file_id == 0xB0) )
    {
      printf (" P0\n\n");
    }

/* programs separator ? */
    if (c == 0xE0)
    {
      prog_number++;
      line_counter = 0;
      if ((file_id != 0xF0 && file_id != 0xB0) || prog_number > 10)
      {
        return 6;   /* "Unexpected program separator" error */
      }
      if (prog_number < 10)
      {
        printf ("\n P%d\n\n", prog_number);
      }
      else {
        printf ("\n END\n");
      }
      return 0;
    }

/* memo file segment ? */
    if (file_id == 0x90 || file_id == 0xA0)
    {
      casioprint(c);
      return 0;
    }

/* don't display data segments containing variables */
    if (file_id == 0xE0)
    {
      return 0;
    }

/* the least significant byte of the line number ? */
    if (++line_counter == 1)
    {
      line_number = bcd2bin(c);
      return 0;
    }

/* the most significant byte of the line number ? */
    if (line_counter == 2)
    {
      line_number += 100*bcd2bin(c);
      printf ("%d ", line_number);
      return 0;
    }

/* print remaining bytes of the data segment */
    if (c == 0xFF)
    {
      line_counter = 0;
    }
    casioprint(c);
    return 0;
  }

/* handle the header segment */

/* display the file name */
  if (data_counter <= 1 + filename_length)
  {
    casioprint(c);
    if (data_counter == 1 + filename_length)
    {
      putchar( '"' );
    }
    return 0;
  }

/* skip remaining data */
  if (data_counter < 11)
  {
    return 0;
  }

/* last byte of the header segment */
  printf ("\n\n");
  leader_expected = TRUE;
  skipped = 5;
  dataseg_expected = TRUE;
  return 0;
}


int main(int argc, char *argv[])
{
    int c1;
    int counter;      /* character counter */
    long position = 0;    /* file position */
    int err_code;
    unsigned int x;   /* 12-bit word */
    FILE *infp = NULL;
    KCS_FILE *wfp = NULL;
    int skip;
    bool wavemode = FALSE;
    bool ignore = FALSE;

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
        if ( strncmp( argv[ 1 ], "-e", 2 ) == 0 ) {
            EscapeMode = TRUE;
        }
        ++argv;
        --argc;
    }

    if ( argc <= 1 )
    {
        printf( "usage: list730 <options> infile > outfile\n"
                "         -w reads a WAV file directly\n"
                "         -a reads an ASCII encoded file from Piotr's interface\n"
                "         -b<skip> reads a binary file, "
                          "<skip> is an optional offset\n"
                "         -i ignores any errors on the input stream\n"
                "         -e generate backslash escapes for special codes\n" );
        return 2;
    }

    if ( wavemode )
    {
        if ( ( wfp = kcsOpen( *++argv, "rb", 300, 8, 'E', 2 ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    else
    {
        if ( ( infp = fopen( *++argv, BinMode ? "rb" : "rt" ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }

    x = 0;
    counter = 0;

    if ( wavemode )
    {
        while ( ( c1 = kcsReadByte( wfp ) ) >= 0 )
        {
            /* printf( "[%02.2X]", c1 & 0xff ); */
            if ( ignore 
                 && c1 != KCS_LEAD_IN && c1 & ( KCS_FRAMING | KCS_PARITY ) ) 
            {
                continue;
            }
            if ( ( err_code = list( c1 ) ) != 0 )
            {
                printf( "\nInvalid data encountered - %s.\n", err_msg[ err_code ] );
                break;
            }
        }
        if ( c1 != KCS_EOF ) {
            if ( c1 == KCS_BAD_DATA ) {
                fprintf( stderr, "WAV file is bad: %d\n", c1 );
            }
            else {
                perror( "read" );
            }
            return 2;
        }
        kcsClose( wfp );
    }
    else
    {
        while ( ( c1 = fgetc( infp ) ) != EOF )
        {
            /* printf( "[%02.2X]", c1 ); */
      
            if ( BinMode ) {
                if ( c1 == 0x55 && position == 0 ) {
                    /*
                     *  Memory dump
                     */
                    list( 0xff ); /* All programs, no name */
                    for ( c1 = 0; c1 < 10; ++c1 ) list( 0 );
                    list( 0x02 ); /* start data segment */
                    skip = 3;
                }
                if ( skip > 0 ) {
                    --skip;
                    ++position;
                    continue;
                }
                if ((err_code = list(c1)) != 0)
                {
                    printf ("\nInvalid data @%04.4lX [%02.2X] - %s.\n",
                            position, c1, err_msg[err_code]);
                    break;
                }
                ++position;
                continue;
            }
  
            /* skip invalid characters */
            if (c1<0x30 || c1>0x6F)
            {
                counter = 0;
            }
            else
            {
                /* shift the received 6-bit data into the 12-bit word */
                x = (x>>6) & 0x003F;
                x |= (((unsigned int) (c1-0x30)) << 6);
      
                if (++counter == 2) /* already 2 characters processed ? */
                {
                    counter = 0;
                    c1 = (int) ((x>>1) & 0xFF); /* strip the start, stop and parity bits */
                    if ((x & 0x01) != 0)
                    {
                        c1 = KCS_LEAD_IN;   /* idle string (leader) */
                    }
      
                    if ((err_code = list(c1)) != 0)
                    {
                        printf ("\nInvalid data encountered - %s.\n", err_msg[err_code]);
                        break;
                    }
                }
            }
        }
        fclose (infp);
    }
  
    return 0;
}

