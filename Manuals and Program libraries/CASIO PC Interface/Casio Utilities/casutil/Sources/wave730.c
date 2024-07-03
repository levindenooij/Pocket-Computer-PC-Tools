/*
 *  wave730.c - Create Casio tape WAV file
 *
 *  This program creates a WAV file from a tape image saved by the PB-100, 
 *  PB-220, FX-700P up to FX-730P calculators through the sound card or with
 *  Piotr's serial interface.
 *
 *  Options:
 *    -b<skip> read BIN file (Default); skip <skip> garbage bytes
 *    -a       read ASCII file (Piotr's format)
 *
 *  Based on list730.c by Piotr Piatek
 *  Written by Marcus von Cube
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bool.h"
#include "wave.h"

#define LEAD_IN_TIME 20       /* tens of a second */
#define LEAD_IN_TIME_FIRST 20 /* tens of a second */

/* error messages */
const char *err_msg[] = {
  "",           /* OK */
  "leader expected",        /* message #1 */
  "premature end of a header/data segment", /* message #2 */
  "premature end of a file",        /* message #3 */
  "header segment expected",        /* message #4 */
  "unknown segment identifier",     /* message #5 */
  "unexpected program separator",   /* message #6 */
  "write error"                     /* message #7 */
};

/* file types 0x9_ to 0xF_ */
const char *file_type[] = {
  "Memo",
  "Memo",   /* password protected */
  "All programs", /* password protected */
  "Program",    /* password protected */
  "Program",
  "Variables",
  "All programs"
};

/* character codes 0x00 to 0x7F */
const char characters[] =
" +-*/^!\"#$> = < 0123456789. )( EABCDEFGHIJKLMNOPQRSTUVWXYZ      abcdefghijklmnopqrstuvwxyz  ?,;:o ' @x: <   u v>%  [&_'.]       ";

/* flag for binary mode */
bool BinMode = TRUE;

/* wave file handling */
KCS_FILE *Out;
int LeadInTime = LEAD_IN_TIME_FIRST;

void casioprint (int c)
{
  switch (c)
  {
    case 0x0B:
      printf (">=");
      break;
    case 0x0D:
      printf ("<=");        
      break;
    case 0x0F:
      printf ("<>");
      break;
    case 0x1B:
      printf ("PI");
      break;
    case 0x1E:
      printf ("E-");
      break;
    case 0xFE:
      printf (":");
      break;
    case 0xFF:
      printf ("\n");
      break;
    default:
      if (c <= 0x7F)
        printf ("%c", characters[c]);
      break;
  }
}


/* returns 0 when OK or the error code when invalid data encountered */
int output (int c)
{
  static int skipped = 0;
  static int idle_counter = 0;
  static int data_counter = 0;
  static int segment_id = 0;
  static int file_id = 0;
  static bool lead_in_expected = TRUE;
  static bool dataseg_expected = FALSE;
  static bool password_expected = FALSE;

  if ( !BinMode ) {

    if ( lead_in_expected && c != 0x100 )
    {
      /* Data byte where an idle string is expected.
         This can be a trailing byte in a block of unknown
         meaning or just garbage.
         Write it to the file but don't interprete it.
      */
      if ( 0 != kcsWriteByte( Out, c ) ) {
        return 7;   /* I/O error */
      }
      if ( ++skipped >= 20 ) {
        return 1;   /* "Leader expected" error */
      }
      return 0;
    }

  /* handle the lead_in */
    if ( c == 0x100 ) /* idle string (lead_in) */
    {
      if ( idle_counter == 0 && !lead_in_expected )
      {
        /* unexpected idle character */
        return 2;   /* "Premature end of a header/data segment" error */
      }
      if (idle_counter++ >= 12) /* lead_in should contain at least 12 idle strings */
      {
        /* valid lead_in detected, write a lead-in to the file */
        if ( LeadInTime != 0 && 0 != kcsLeadIn( Out, LeadInTime ) ) {
          return 7;   /* I/O error */
        }
        lead_in_expected = FALSE;  /* valid lead_in detected */
        LeadInTime = 0; /* Don't write another lead-in in this block */
      }
      skipped = 0;
      data_counter = 0;
      return 0;
    }

  /* data byte received */
    idle_counter = 0;
    LeadInTime = LEAD_IN_TIME;  /* for the next block to come */
  }
  else {
    /* BinMode */
    if ( lead_in_expected ) {
      if ( !dataseg_expected && ( c & 0xf0 ) >= 0x90 && ( c & 0x0f ) <= 8
           || c == 0x02 || c == 0x01 )
      {
        /* valid header byte encountered */
        data_counter = 0;
        lead_in_expected = FALSE;
        if ( 0 != kcsLeadIn( Out, LeadInTime ) ) {
          return 7;
        }
        LeadInTime = LEAD_IN_TIME;
      }
      else {
        /*
         *  write all bytes, even if ignored
         */
        if ( 0 != kcsWriteByte( Out, c ) ) {
          return 7;
        }    
        return 0;
      }
    }
  }
  /*
   *  write the byte
   */
  if ( 0 != kcsWriteByte( Out, c ) ) {
    return 7;
  }    

/* handle the first segment byte */
  if (++data_counter == 1)
  {
    segment_id = c;

    if (dataseg_expected)
    {
      if (c == 0x02 || c == 0x01)    /* data segment ? */
      {
        return 0;
      }
      return 3;     /* "Premature end of a file" error */
    }

/* header segment expected */
    if (c == 0x02 || c == 0x01)    /* data segment ? */
    {
      return 4;     /* "Header segment expected" error */
    }

    if ((file_id = c & 0xF0) >= 0x90)
    {
      printf ("%s: \"", file_type[file_id/16 - 0x09]);
      password_expected = (file_id <= 0xC0 && file_id >= 0xA0);
      return 0;
    }

    return 5;     /* "Unknown segment identifier" error */
  }

/* handle subsequent segment bytes */

/* handle the data segment */
  if (segment_id == 0x02 || segment_id == 0x01) /* data segment ? */
  {
/* End Marker ? */
    if (c == 0xF0 || c == 0xF1)
    {
      lead_in_expected = TRUE;
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

    return 0;
  }

/* handle the header segment */

/* display the file name */
  if (data_counter <= 1 + (segment_id & 0x0F))
  {
    casioprint(c);
    if (data_counter == 1 + (segment_id & 0x0F))
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
  printf( "\n" );
  lead_in_expected = TRUE;
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
  FILE *infp;
  int skip;

  if ( argc > 1 ) {
    if ( strncmp( argv[ 1 ], "-a", 2 ) == 0 ) {
      BinMode = FALSE;
      ++argv;
      --argc;
    }

    if ( strncmp( argv[ 1 ], "-b", 2 ) == 0 ) {
      BinMode = TRUE;
      ++argv;
      --argc;
      skip = atoi( *argv + 2 );
    }
  }  
  if (argc<=1)
  {
    printf( "usage: wave730 [-a|-b<skip>] infile outfile\n"
            "       -a read an ASCII encoded file from Piotr's interface\n"
            "       -b<skip> read a binary file, "
                    "<skip> is an optional offset\n" );
    return 1;
  }

  if ( ( infp = fopen( *++argv, BinMode ? "rb" : "rt" ) ) == NULL )
  {
    fprintf( stderr, "\nCannot open the input file %s\n", *argv );
    perror( *argv );
    return 1;
  }

  if ( ( Out = kcsOpen( *++argv, "wb", 300, 8, 'E', 2 ) ) == NULL )
  {
    fprintf (stderr, "\nCannot open the output file %s\n",*argv );
    perror( *argv );
    return 1;
  }

  x = 0;
  counter = 0;

  while ((c1 = fgetc(infp)) != EOF)
  {
    /* printf( "[%02.2X]", c1 ); */
    ++position;

    if ( BinMode ) {
      if ( skip > 0 ) {
        --skip;
        continue;
      }
      if ((err_code = output(c1)) != 0)
      {
        if ( err_code == 9 ) {
          perror( err_msg[err_code] );
        }
        else {
          printf ("\nInvalid data @%04.4lX [%02.2X] - %s.\n",
                   position, c1, err_msg[err_code]);
        }
        break;
      }
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
          c1 = 0x100;   /* idle string (lead_in) */
        }

        if ((err_code = output(c1)) != 0)
        {
          if ( err_code == 9 ) {
            perror( err_msg[err_code] );
          }
          else {
            printf ("\nInvalid data @%04.4lX [%02.2X] - %s.\n",
                     position, c1, err_msg[err_code]);
          }
          break;
        }
      }
    }
  }

  (void) fclose (infp);
  kcsWriteBit( Out, 0 );
  kcsWriteBit( Out, 0 );
  kcsClose( Out );
  return 0;
}
