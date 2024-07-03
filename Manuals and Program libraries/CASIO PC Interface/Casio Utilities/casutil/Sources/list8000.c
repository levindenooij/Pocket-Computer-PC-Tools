/*
 *  list8000.c - List Casio fx-8000G/8500G tape files
 *
 *  This program displays the contents of a tape image saved by the Casio
 *  graphic calculators that connect to the FA-80 interface.
 *
 *  Written by Marcus von Cube
 */
#define DEBUG 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bool.h"
#include "wave.h"

/*
 *  Error messages
 */
char *err_msg[] = 
{
    "",                                       /* OK */
    "Leader expected",                        /* message #1 */
    "Premature end of segment",               /* message #2 */
    "Premature end of a file",                /* message #3 */
    "Header segment expected",                /* message #4 */
    "Unknown segment identifier",             /* message #5 */
    "File write error",                       /* message #6 */
    "Unsupported format",                     /* message #7 */
};

/*
 *  Tokens outside the ASCII character set
 *  The Escape sequences are mainly for translation to CAT format
 */
typedef struct _token {
    unsigned char token;
    char *escape;
    char *text;
    char *tDos;
    char *tWin;
} TOKEN;

TOKEN Tokens[] =
{ 
/*    Tok   Esc              Text         CP437        CP1252         */ 
    { 0x01, "\\fempto"     , "fempto"   , NULL       , NULL            },
    { 0x02, "\\pico"       , "pico"     , NULL       , NULL            },
    { 0x03, "\\nano"       , "nano"     , NULL       , NULL            },
    { 0x04, "\\micro"      , "micro"    , NULL       , NULL            },
    { 0x05, "\\milli"      , "milli"    , NULL       , NULL            },
    { 0x06, "\\kilo"       , "kilo"     , NULL       , NULL            },
    { 0x07, "\\Mega"       , "Mega"     , NULL       , NULL            },
    { 0x08, "\\Giga"       , "Giga"     , NULL       , NULL            },
    { 0x09, "\\Tera"       , "Tera"     , NULL       , NULL            },
    { 0x0A, "\\Peta"       , "Peta"     , NULL       , NULL            },
    { 0x0B, "\\Exa"        , "Exa"      , NULL       , NULL            },
    { 0x0C, "\\Disp"       , " Disp\n"  , NULL       , NULL            },
    { 0x0D, "\n"           , "\n"       , NULL       , NULL            },
    { 0x0E, "\\->"         , "->"       , "\x1A"     , "-\x8B"         },
    { 0x0F, "\\EE"         , "E"        , NULL       , NULL            },
    { 0x10, "\\<="         , "<="       , "\xF3"     , NULL            },
    { 0x11, "\\<>"         , "<>"       , "\xD8"     , NULL            },
    { 0x12, "\\>="         , ">="       , "\xF2"     , NULL            },
    { 0x13, "\\=>"         , "=>"       , "=\x10"    , NULL            },
    { 0x14, "\\f1"         , "f1"       , NULL       , NULL            },
    { 0x15, "\\f2"         , "f2"       , NULL       , NULL            },
    { 0x16, "\\f3"         , "f3"       , NULL       , NULL            },
    { 0x17, "\\f4"         , "f4"       , NULL       , NULL            },
    { 0x18, "\\f5"         , "f5"       , NULL       , NULL            },   
    { 0x19, "\\f6"         , "f6"       , NULL       , NULL            },
    { 0x1A, "\\hA"         , "{A}"      , NULL       , NULL            },
    { 0x1B, "\\hB"         , "{B}"      , NULL       , NULL            },
    { 0x1C, "\\hC "        , "{C}"      , NULL       , NULL            },
    { 0x1D, "\\hD"         , "{D}"      , NULL       , NULL            },
    { 0x1E, "\\hE"         , "{E}"      , NULL       , NULL            },
    { 0x1F, "\\hF"         , "{F}"      , NULL       , NULL            },
    { 0x80, "\\Pol("       , "Pol("     , NULL       , NULL            },
    { 0x81, "\\sin "       , "sin "     , NULL       , NULL            },
    { 0x82, "\\cos "       , "cos "     , NULL       , NULL            },
    { 0x83, "\\tan "       , "tan "     , NULL       , NULL            },
    { 0x84, "\\Hex>"       , "h"        , NULL       , NULL            },
    { 0x85, "\\ln "        , "ln "      , NULL       , NULL            },
    { 0x86, "\\sqrt "      , "sqrt "    , "\xFB"     , NULL            },
    { 0x87, "\\(-)"        , "(-)"      , "\xAA"     , "\xAF"          },
    { 0x89, "+"            , "+"        , NULL       , NULL            },
    { 0x8A, "\\xnor"       , " xnor "   , NULL       , NULL            },
    { 0x8B, "\\x^2"        , "^2"       , "\xFD"     , "\xB2"          },
    { 0x8C, "\\Dms"        , "`"        , NULL       , NULL            },
    { 0x8D, "\\Integral("  , "Integral(", "\xF4("    , NULL            },
    { 0x8E, "\\Mo"         , "Mo"       , NULL       , NULL            },
    { 0x8F, "\\Sumx2"      , "Sumx2 "   , "\xE4x\xFD", NULL            },
    { 0x91, "\\asin "      , "arc sin " , NULL       , "sin\xAF\xB9 "  },
    { 0x92, "\\acos "      , "arc cos " , NULL       , "cos\xAF\xB9 "  },
    { 0x93, "\\atan "      , "arc tan " , NULL       , "tan\xAF\xB9 "  },
    { 0x94, "\\Dec>"       , "d"        , NULL       , NULL            },
    { 0x95, "\\log "       , "log "     , NULL       , NULL            },
    { 0x96, "\\curt"       , "Cur "     , NULL       , NULL            },
    { 0x97, "\\Abs "       , "Abs "     , NULL       , NULL            },
    { 0x98, "\\nCr "       , " nCr "    , NULL       , NULL            },
    { 0x99, "-"            , "-"        , NULL       , NULL            },
    { 0x9A, "\\xor"        , " xor "    , NULL       , NULL            },
    { 0x9B, "\\x^-1"       , "^(-1)"    , NULL       , "\xAF\xB9"      },
    { 0x9C, "\\Deg>"       , " deg"     , "\xF8"     , "\xB0"          },
    { 0x9E, "\\Med"        , "Med "     , NULL       , NULL            },
    { 0x9F, "\\Sumx"       , "Sumx "    , "\xE4x"    , NULL            },
    { 0xA0, "\\Rec("       , "Rec("     , NULL       , NULL            },
    { 0xA1, "\\sinh "      , "sinh "    , NULL       , NULL            },
    { 0xA2, "\\cosh "      , "cosh "    , NULL       , NULL            },
    { 0xA3, "\\tanh "      , "tanh "    , NULL       , NULL            },
    { 0xA4, "\\Oct>"       , "o"        , NULL       , NULL            },
    { 0xA5, "\\e^x"        , "e"        , NULL       , NULL            },
    { 0xA6, "\\Int "       , "Int "     , NULL       , NULL            },
    { 0xA7, "\\ Not"       , "Not "     , NULL       , NULL            },
    { 0xA8, "^"            , "^"        , NULL       , NULL            },
    { 0xA9, "*"            , "*"        , NULL       , "\xD7"          },
    { 0xAA, "\\or"         , " or "     , NULL       , NULL            },
    { 0xAB, "!"            , "!"        , NULL       , NULL            },
    { 0xAC, "\\Rad>"       , " rad"     , NULL       , NULL            },
    { 0xAD, "\\minY"       , "minY"     , NULL       , NULL            },
    { 0xAE, "\\minX"       , "minX"     , NULL       , NULL            },
    { 0xAF, "\\Cnt"        , "n"        , NULL       , NULL            },
    { 0xB1, "\\asinh "     , "arc sinh ", NULL       , "sinh\xAF\xB9 " },
    { 0xB2, "\\acosh "     , "arc cosh ", NULL       , "cosh\xAF\xB9 " },
    { 0xB3, "\\atanh "     , "arc tanh ", NULL       , "tanh\xAF\xB9 " },
    { 0xB4, "\\Bin>"       , "b"        , NULL       , NULL            },
    { 0xB5, "\\10^x"       , "(10)^"    , NULL       , NULL            },
    { 0xB6, "\\Frac "      , "Frac "    , NULL       , NULL            },
    { 0xB7, "\\Neg "       , "Neg "     , NULL       , NULL            },
    { 0xB8, "\\xrt"        , " Root "   , NULL       , NULL            },
    { 0xB9, "/"            , "/"        , "\xF6"     , "\xF7"          },
    { 0xBA, "\\and"        , " and "    , NULL       , NULL            },
    { 0xBB, "\\ab/c"       , "_"        , "\xA9"     , NULL            },
    { 0xBC, "\\Gra>"       , " gra"     , NULL       , NULL            },
    { 0xBD, "\\maxY"       , "maxY"     , NULL       , NULL            },
    { 0xBE, "\\maxX"       , "maxX"     , NULL       , NULL            },
    { 0xBF, "\\Sumy2"      , "Sumy2 "   , "\xE4y\xFD", NULL            },
    { 0xC0, "\\Ans"        , "Ans"      , NULL       , NULL            },
    { 0xC1, "\\Ran#"       , "Ran#"     , NULL       , NULL            },
    { 0xC2, "\\MeanX"      , "Mx"       , NULL       , NULL            },
    { 0xC3, "\\MeanY"      , "My"       , NULL       , NULL            },
    { 0xC4, "\\Sdx"        , "SDx"      , "x\xE5"    , NULL            },
    { 0xC5, "\\Sdxn"       , "SDxn"     , "x\xE5n"   , NULL            },
    { 0xC6, "\\Sdy"        , "SDy"      , "y\xE5"    , NULL            },
    { 0xC7, "\\Sdyn"       , "SDyn"     , "y\xE5n"   , NULL            },
    { 0xC8, "\\{a}"        , "a"        , NULL       , NULL            },
    { 0xC9, "\\{b}"        , "b"        , NULL       , NULL            },
    { 0xCA, "\\Cor"        , "Cor"      , NULL       , NULL            },
    { 0xCB, "\\Eox"        , "Ex"       , NULL       , NULL            },
    { 0xCC, "\\Eoy"        , "Ey"       , NULL       , NULL            },
    { 0xCD, "\\r"          , "r"        , NULL       , NULL            },
    { 0xCE, "\\theta"      , "theta"    , NULL       , NULL            },
    { 0xCF, "\\Sumy"       , "Sumy "    , "\xE4y"    , NULL            },
    { 0xD0, "\\Pi"         , "Pi"       , "\xE3"     , NULL            },
    { 0xD1, "\\Cls"        , "Cls"      , NULL       , NULL            },
/*  { 0xD2, "0\x0E" "A~Z:0\x0E\xCD:0\x0E\xCE:26\x0E\x7F\x46\x7F\x51" "1" */
            /* 0->A~Z:0->r:0->theta:26->Dim List 1 */         
    { 0xD2, "\x7F\x2C" "0,X,1,26,1)\x0E\x7F\x51" "1:0\x0E" 
            "A~Z:0\x0E\xCD:0\x0E\xCE"
            /* Seq(0,X,1,26,1)->List 1:0->A~Z:0->r:0->theta */         
                           , "Mcl"      , NULL       , NULL            },
    { 0xD3, "\\Rnd"        , "Rnd"      , NULL       , NULL            },
    { 0xD4, "\\Dec"        , "Dec"      , NULL       , NULL            },
    { 0xD5, "\\Hex"        , "Hex"      , NULL       , NULL            },
    { 0xD6, "\\Bin"        , "Bin"      , NULL       , NULL            },
    { 0xD7, "\\Oct"        , "Oct"      , NULL       , NULL            },
    { 0xD8, "Scl"          , "Scl"      , NULL       , NULL            },
    { 0xD9, "\\Norm"       , "Norm"     , NULL       , NULL            },
    { 0xDA, "\\Deg"        , "Deg"      , NULL       , NULL            },
    { 0xDB, "\\Rad"        , "Rad"      , NULL       , NULL            },
    { 0xDC, "\\Gra"        , "Grad"     , NULL       , NULL            },
    { 0xDE, "\\Intg "      , "Intg "    , NULL       , NULL            },
    { 0xDF, "\\Sumxy"      , "Sumxy "   , "\xE4xy"   , NULL            },
    { 0xE0, "\\Plot "      , "Plot "    , NULL       , NULL            },
    { 0xE1, "\\Line"       , "Line"     , NULL       , NULL            },
    { 0xE2, "\\Lbl "       , "Lbl "     , NULL       , NULL            },
    { 0xE3, "\\Fix "       , "Fix "     , NULL       , NULL            },
    { 0xE4, "\\Sci "       , "Sci "     , NULL       , NULL            },
    { 0xE6, " Cl"          , " Cl"      , NULL       , NULL            },
    { 0xE7, " Dt"          , " Dt"      , NULL       , NULL            },
    { 0xE8, "\\Dsz "       , "Dsz "     , NULL       , NULL            },
    { 0xE9, "\\Isz "       , "Isz "     , NULL       , NULL            },
    { 0xEA, "\\Factor "    , "Factor "  , NULL       , NULL            },
    { 0xEB, "\\ViewWindow ", "Range "   , NULL       , NULL            },
    { 0xEC, "\\Goto"       , "Goto "    , NULL       , NULL            },
    { 0xED, "\\Prog "      , "Prog "    , NULL       , NULL            },
    { 0xEE, "\\Graph Y="   , "Graph Y=" , NULL       , NULL            },
    { 0xEF, "\\Graph Integral " , "Graph Integral "   , "Graph \xF4 " , NULL },
    { 0 }
};

/*
 *  Old tokens only valid for fx-8000G
 */
TOKEN Tokens_fx8000G[] =
{ 
/*    Tok   Esc             Text         CP437        CP1252         */ 
    { 0x18, "\\micro"      , "micro"    , "\xE6"     , "\xB5"          },
/*  { 0xD2, "0\x0E" "A~Z:26\x0E\x7F\x46\x7F\x51" "1" */
    { 0xD2, "\x7F\x2C" "0,X,1,26,1)\x0E\x7F\x51" "1:0\x0E" "A~Z"
            /* Seq(0,X,1,26,1)->List 1:0->A~Z */         
                           , "Mcl"      , NULL       , NULL            },
    { 0xEF, "Print "       , "Print "   , NULL       , NULL            },
    { 0 }
};

/*
 *  Extended tokens 0x7F ..
 */
TOKEN Tokens_7F[] =
{ 
/*    Tok   Esc             Text         CP437        CP1252         */ 
    { 0x2C, "\\Seq("       , "Seq("     , NULL       , NULL            },
    { 0x34, "\\Orange"     , "Orange"   , NULL       , NULL            },
    { 0x35, "\\Blue"       , "Blue"     , NULL       , NULL            },
    { 0x36, "\\Green"      , "Green"    , NULL       , NULL            },
    { 0x46, "\\Dim "       , "Dim "     , NULL       , NULL            },
    { 0x51, "\\List "      , "List "    , NULL       , NULL            },
    { 0 }
};

/*
 *  Constant 0.0 in internal floating point representation
 */
unsigned char Zero[ 8 ] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*
 *  Flag for binary mode 
 */
bool BinMode = TRUE;

/*
 *  Escape mode
 */
enum { ESC_NONE, ESC_CHAR, ESC_HEX } EscapeMode = ESC_NONE;

/*
 *  Translation mode
 */
enum { TRANS_NONE, TRANS_DOS, TRANS_WINDOWS, TRANS_CAT } 
    TransMode = TRANS_NONE;

/*
 *  Encoding type for printNumber()
 */
typedef enum { ENC_ASCII, ENC_ESCAPE, ENC_INTERNAL } ENCODING;

/*
 *  Listing width
 */
int Width = 40;

/*
 *  File is from fx-8000G tape.
 *  For the time being, this is always the case.
 */
bool Fx8000G = TRUE;

/*
 *  Bitmap filename
 */
char *BitmapFile = NULL;

/*
 *  Override filename with option -cat
 */
char *NewName = NULL;

/*
 *  CAS file handle and mode
 */
FILE *CasFile = NULL;
enum { CAS_NONE, CAS_ALL, CAS_PROGRAM, CAS_EDITOR, CAS_7700, 
       CAS_MEMORY, CAS_RANGE } 
    CasMode = CAS_NONE;

/*
 *  First program number
 *  The program area numbers are NOT stored in the tape recording!
 */
int ProgramNumber = 0;

/*
 *  Header bytes
 */
struct _header {
    unsigned char id;
    unsigned char file_name[ 16 + 1 ];
} Header;

#define HEADER_SIZE  18

/*
 *  Record Types
 */
#define TYPE_HEADER  0x08
#define TYPE_PROGRAM 0xF1
#define TYPE_MEMORY  0xF2
#define TYPE_GRAPH   0xF3
#define TYPE_EDITOR  0xF4
#define TYPE_EOF     0xFF
#define TYPE_RANGE   0xAA   /* Pseudo for CAT file creation */

/*
 *  Program modes (index = mode / 32)
 */
char *ProgModes[ 8 ] = { "COMP", "SD1", "LR1", "BASE-n",
                         NULL,   "SD2", "LR2", NULL };
char *ProgOption1s[ 8 ] = { "NL", "NL", "NL", "BN", 
                            NULL, "NL", "NL", NULL };
unsigned char ProgModesCas[ 8 ] = { 0x00, 0x10, 0x20, 0x40,
                                    0x00, 0x10, 0x20, 0x00 };

/*
 *  Buffers for Range / ViewWindow values in internal format
 */
unsigned char Xmin[ 8 ];
unsigned char Xmax[ 8 ];
unsigned char Xscl[ 8 ];
unsigned char Xdot[ 8 ];
unsigned char Ymin[ 8 ];
unsigned char Ymax[ 8 ];
unsigned char Yscl[ 8 ];
unsigned char Ydot[ 8 ];

/*
 *  Buffer for program or file in raw format for later export
 */
struct _filebuffer {
    int mode;
    unsigned char file_name[ 8 + 1 ];
    unsigned char password[ 126 + 1];
    unsigned char data[ 8196 ];
} File;


/*
 *  CAS file header
 */
struct _casheader {
    char colon;
    char id[ 2 ];
    unsigned char subheader[ 5 ];
    unsigned char file_name[ 12 ];
    unsigned char reserved[ 19 ];
    unsigned char checksum;
} CasHeader;

#define CAS_HEADER_SIZE 40

/*
 *  CAS number (Real)
 */
struct _casnumber {
    char colon;
    char reserved[ 2 ];
    unsigned char number[ 8 ];
    unsigned char checksum;
} CasNumber;

#define CAS_NUMBER_SIZE 12

/*
 *  CAS number block subheader
 */
struct _casnum_subheader {
    char id[ 2 ];
    unsigned char count_msb;
    unsigned char count_lsb;
    unsigned char reserved;
};
    

/*
 *  CAS range (Real)
 */
struct _casrange {
    char colon;
    char reserved[ 2 ];
    unsigned char Xmin[ 8 ];
    unsigned char Xmax[ 8 ];
    unsigned char Xscl[ 8 ];
    unsigned char Ymin[ 8 ];
    unsigned char Ymax[ 8 ];
    unsigned char Yscl[ 8 ];
    unsigned char Tthmin[ 8 ];
    unsigned char Tthmax[ 8 ];
    unsigned char Tthptch[ 8 ]; 
    unsigned char checksum;
} CasRange =
{
    ':', 0, 0, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x31, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
    { 0x31, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

#define CAS_RANGE_SIZE 76

/*
 *  CAS program header for multiple programs
 */
struct _cas_prog_table {
    char colon;
    struct _cas_prog {
        unsigned char res1;
        unsigned char size_msb;
        unsigned char size_lsb;
        unsigned char type;
        unsigned char res2;
    } header[ 38 ];   /* 0-9, A-Z, r, theta */
    unsigned char checksum;
} CasProgTable;

#define CAS_PROG_TABLE_SIZE 192


/*
 *  Bitmap buffer
 */
#define G_WIDTH  96
#define G_HEIGHT 64
#define G_BITS 1
#define G_IMAGE_SIZE ( G_WIDTH * G_HEIGHT / ( 8 / G_BITS ) )
#define G_RESOLUTION 2500

struct _bitmap {
    char _filler[ 2 ];
    char id[ 2 ];
    unsigned long file_size;
    unsigned short free1, free2;
    unsigned long offset;
    unsigned long header_size;
    long width;
    long height;
    unsigned short planes;
    unsigned short bits;
    unsigned long compression;
    unsigned long image_size;
    unsigned long x_resolution;
    unsigned long y_resolution;
    unsigned long palette_size;
    unsigned long important;
    struct _rgb {
        unsigned char blue, green, red, free1;
    } palette[ 2 ];
} BitmapHeader = 
{
    { 0, 0 }, { 'B', 'M' }, 0, 
    0, 0, 14 + 40 + 2 * 4,
    40, G_WIDTH, G_HEIGHT, 1, G_BITS, 0, G_IMAGE_SIZE, 
    G_RESOLUTION, G_RESOLUTION, 2, 0,
    { { 190, 220, 190, 0 }, { 114, 56, 56, 0 } }
};

#define B_HEADER_SIZE ( sizeof( struct _bitmap ) - 2 )

/*
 *  Buffer for BMP data. Large enough for triple size image.
 */
unsigned char BitmapImage[ 9 * G_IMAGE_SIZE ];


/*
 *  Buffer for raw data from calculator
 */
unsigned char Graphic[ 0xF00 ];


/*
 *  Create Bitmaps
 *  There are two pictures in the file
 *  If the name contains a '?', we dump them all
 */
int createBitmap( void )
{
    FILE *fd;
    unsigned char *p, *source;
    int x, y, fcount;
    unsigned char mask, source_mask;
    char *fn_index_pos;
    int bit, pass;

    if ( BitmapFile == NULL ) {
        return 0;
    }

    LSB_FIRST( BitmapHeader.offset );
    LSB_FIRST( BitmapHeader.header_size );
    LSB_FIRST( BitmapHeader.planes );
    LSB_FIRST( BitmapHeader.bits );
    LSB_FIRST( BitmapHeader.compression );
    LSB_FIRST( BitmapHeader.x_resolution );
    LSB_FIRST( BitmapHeader.y_resolution );
    LSB_FIRST( BitmapHeader.palette_size );
    LSB_FIRST( BitmapHeader.important );

    /*
     *  Look for wildcard in filename for index
     */
    fn_index_pos = strchr( BitmapFile, '#' );
    if ( fn_index_pos == NULL ) {
        /*
         *  no wildcard found, output only first file
         */
        fcount = 1;
    }
    else {
        fcount = 3;
    }

    for ( pass = 1; pass <= fcount; ++pass ) {
        /*
         *  Analyze the Image and convert it to a BMP file.
         *  The bits are stored right to left, bottom to top, inverted.
         *  One byte is 2x4 rows, high nibble comes first.
         *
         *  The second picture is double sized !
         *  The third file is an enlarged version of the first with 2x2
         *  pixels in a 3x3 box. This looks closer to the original screen.
         */
        int height = G_HEIGHT * pass;
        int width  = G_WIDTH  * pass;
        int source_width = G_WIDTH * ( pass == 2 ? 2 : 1 );
        int size = height * width / ( 8 / G_BITS );

#if DEBUG
        printf( "\nPass %d, %dx%d (%d)\n", pass, height, width, size );
        fflush( stdout );
#endif
        p = BitmapImage;
        memset( BitmapImage, 0, size );
        source = pass != 2 ? Graphic : Graphic + G_IMAGE_SIZE;

        for ( y = 0, source_mask = 0x10; y < height; ++y ) {
            /*
             *  Bottom to top ordering of lines is standard with BMP files
             */
            for ( x = 0, mask = 0x80; x < width; ++x ) {
                /*
                 *  Pixels are stored rightmost first
                 */
                if ( pass <= 2 ) {
                    bit = ( ~source[ source_width - 1 - x ] ) & source_mask;
                }
                else {
                    if ( x % 3 == 2 || y % 3 == 0 ) {
                        bit = 0;
                    }
                    else {
                        bit = ( ~source[ source_width - 1 - x / 3 ] ) 
                            & source_mask;
                    }
                }
                if ( bit ) {    
                    *p |= mask;
                }
                mask >>= 1; 
                if ( mask == 0 ) {
                    mask = 0x80;
                    ++p;
                }
            }
#if DEBUG
            printf( "y=%d,s=%p,m=%x,b=%x\n", y, source - Graphic, source_mask,
                                        *source );
            fflush( stdout );
#endif
            if ( pass <= 2 || y % 3 == 2 ) {
                source_mask <<= 1;
                if ( source_mask == 0 ) {
                    source_mask = 1;
                }
                else if ( source_mask == 0x10 ) {
                    /*
                     *  Next eight rows
                     */
                    source += source_width;
                }
            }
        } 

        if ( fn_index_pos != NULL ) {
            /*
             *  Set index in filename
             */
            *fn_index_pos = (char) ( '0' + pass );
        }

        /*
         *  Update some fields depending on size
         */
        BitmapHeader.file_size = sizeof( BitmapHeader ) - 2 + size;
        BitmapHeader.width = width;
        BitmapHeader.height = height;
        BitmapHeader.image_size = size;

        LSB_FIRST( BitmapHeader.file_size );
        LSB_FIRST( BitmapHeader.width );
        LSB_FIRST( BitmapHeader.height );
        LSB_FIRST( BitmapHeader.image_size );

        /*
         *  Write the file to disk
         */
        fd = fopen( BitmapFile, "wb" );
        if ( fd == NULL ) {
            perror( BitmapFile );
            return 6;
        }
        if ( 1 != fwrite( BitmapHeader.id, B_HEADER_SIZE, 1, fd ) ) {
            perror( BitmapFile );
            return 6;
        }
        if ( 1 != fwrite( BitmapImage, size, 1, fd ) ) {
            perror( BitmapFile );
            return 6;
        }
        fclose( fd );
    }

    if ( fn_index_pos != NULL ) {
        /*
         *  Repair filename
         */
        *fn_index_pos = '#';
    }
    return 0;
}


/*
 *  CAS file handling: Write a block with checksum
 */
int writeCas( void *buffer, int length )
{
    unsigned char *p = (char *) buffer;
    unsigned char chk = 0;
    int l = length - 2;

    *p++ = ':';
    while ( l-- ) {
        chk -= *p++;
    }
    *p = chk;
    if ( 1 != fwrite( buffer, length, 1, CasFile ) ) {
        /*
         *  I/O error
         */
        perror( "CAS file" );
        return 6;
    }
    return 0;
}


/*
 *  Write a CAS header block
 */
int writeCasHeader( char *id, void *subheader, char *file_name )
{
    memcpy( CasHeader.id, id, 2 );
    if ( subheader != NULL ) {
        memcpy( CasHeader.subheader, subheader, 5 );
    }
    else {
        memset( CasHeader.subheader, 0xFF, 5 );
    }
    memset( CasHeader.file_name, 0xFF, 12 + 19 );
    if ( file_name != NULL ) {
        memcpy( CasHeader.file_name, file_name, strlen( file_name ) );
    }
    return writeCas( &CasHeader, CAS_HEADER_SIZE );
}


/*
 *  Write a CAS EOF block
 */
int writeCasEof( char *id, int id_length, int block_length )
{
    memcpy( CasHeader.id, id, id_length );
    memset( CasHeader.id + id_length, 0xFF, block_length - id_length - 2 );
    return writeCas( &CasHeader, block_length );
}


/*
 *  Fill a CAS prog or file header with length and type (index in table)
 */
void fillProgHeader( struct _cas_prog *header, int size, int type )
{
    memset( header, 0, 5 );
    header->size_msb = (unsigned char) ( size >> 8 );
    header->size_lsb = (unsigned char) ( size & 0xff );
    header->type = ProgModesCas[ type ];
}


/*
 *  Write the CAS "All programs" blocks
 */
int writeCasPrograms( int size )
{
    int err;
    struct _cas_prog *subheader = (struct _cas_prog *) CasHeader.subheader;

    fillProgHeader( subheader, size, 0 );
    err = writeCasHeader( "PZ", subheader, NULL );
    if ( err == 0 ) {
        err = writeCas( &CasProgTable, CAS_PROG_TABLE_SIZE );
    }
    if ( err == 0 ) {
        err = writeCas( File.data, size );
    }
    return err;
}


/*
 *  Write a CAS editor file block
 */
int writeCasFile( int size )
{
    int err;
    struct _cas_prog *subheader = (struct _cas_prog *) CasHeader.subheader;

    fillProgHeader( subheader, size, 0 );
    err = writeCasHeader( "FN", subheader, File.file_name );
    if ( err == 0 ) {
        err = writeCas( File.data, size );
    }
    return err;
}


/*
 *  Copy a number upside down for CAS file format
 */
void copyCasNumber( unsigned char *dest, unsigned char *number )
{
    dest[ 7 ] = *number++;
    dest[ 6 ] = *number++;
    dest[ 5 ] = *number++;
    dest[ 4 ] = *number++;
    dest[ 3 ] = *number++;
    dest[ 2 ] = *number++;
    dest[ 1 ] = *number++;
    dest[ 0 ] = *number;
}


/*
 *  Write a CAS number block
 */
int writeCasNumber( unsigned char *number )
{
    CasNumber.reserved[ 0 ] = 0;
    CasNumber.reserved[ 1 ] = 0;
    copyCasNumber( CasNumber.number, number );

    return writeCas( &CasNumber, CAS_NUMBER_SIZE );
}


/*
 *  Write CAS memory block header
 *  count = number of variables (r, theta, A~Z, Defm)
 */
int writeCasMemoryHeader( int count )
{
    struct _casnum_subheader *subheader = 
        (struct _casnum_subheader *) CasHeader.subheader;

    memcpy( subheader->id, "R8", 2 );

    ++count; /* include EOF */
    subheader->count_msb = (unsigned char) ( count >> 8 );
    subheader->count_lsb = (unsigned char) count;
    subheader->reserved = 0xFF;

    return writeCasHeader( "AD", subheader, NULL );
}
    

/*
 *  Write CAS range block including header
 */
int writeCasRange( void )
{
    int err;
    struct _casnum_subheader *subheader = 
        (struct _casnum_subheader *) CasHeader.subheader;

    memcpy( subheader->id, "R8", 2 );

    subheader->count_msb = 0;
    subheader->count_lsb = 9;
    subheader->reserved = 0xFF;

    err = writeCasHeader( "GR", subheader, NULL );
    if ( err != 0 ) {
        return err;
    }

    copyCasNumber( CasRange.Xmin, Xmin );
    copyCasNumber( CasRange.Xmax, Xmax );
    copyCasNumber( CasRange.Xscl, Xscl );
    copyCasNumber( CasRange.Ymin, Ymin );
    copyCasNumber( CasRange.Ymax, Ymax );
    copyCasNumber( CasRange.Yscl, Yscl );

    return writeCas( &CasRange, CAS_RANGE_SIZE );
}
    

/*
 *  Find a token in the token table
 */
TOKEN *findToken( int c, TOKEN *tp )
{
    if ( tp == NULL ) {
        /*
         *  Default handling
         */
        if ( Fx8000G ) {
            /*
             *  Search fx-8000G only tokens first
             */
            tp = findToken( c, Tokens_fx8000G );
            if ( tp->token != 0 ) {
                return tp;
            }
        }
        tp = Tokens;
    }

    /*
     *  Search loop
     */
    while ( tp->token != 0 && tp->token != c ) ++tp;
    return tp;
}


/*
 *  Print a character, translate tokens
 */
void casioprint( int c )
{
    static bool insert_space = FALSE;
    static bool quoted = FALSE;
    static bool ext_7F = FALSE;
    static int last = 0;
    static int column = 0;
    bool literal_space = c == ' ';
    bool parenthesis = c == '(';
    bool cat = TransMode == TRANS_CAT;
    char *p = NULL, *p2;
    char buffer[] = "\\@XX\\@XX";
    TOKEN *tp;

    if ( c == 0xFF ) {
        /*
         *  End of file
         */
        p = "\n";
        quoted = FALSE;
        insert_space = FALSE;
    }
    else if ( c == 0x7F ) {
        /*
         *  Extended token follows
         */
        ext_7F = TRUE;
        return;
    }
    else if ( c < ' ' || c >= 0x80 || ext_7F ) {
        /*
         *  Replacable token
         */
        tp = findToken( c, ext_7F ? Tokens_7F : NULL );
        p = tp->text;

        if ( quoted || cat ) {
            /*
             *  Nonstandard character in string or while creating CAT file
             */
            switch ( EscapeMode ) {

            case ESC_NONE:
                /*
                 *  Replace by text
                 */
                break;

            case ESC_HEX:
                /*
                 *  Replace by Hex-Escape
                 */
                p = NULL;
                break;

            default:
                /*
                 *  Replace by escape sequence
                 */
                if ( p != NULL ) {
                    p = tp->escape;
                }
            }
        }
        if ( TransMode == TRANS_DOS || TransMode == TRANS_WINDOWS ) {
            /*
             *  Check for matching special character(s)
             */
            p2 = TransMode == TRANS_DOS ? tp->tDos : tp->tWin;
            if ( p2 != NULL ) {
                p = p2;
            }
        }
        if ( p == NULL ) {
            /*
             *  Create hex escape
             */
            if ( ext_7F ) {
                sprintf( buffer, "\\@7F\\@%02.2X", c );
            }
            else {
                sprintf( buffer, "\\@%02.2X", c );
            }
            p = buffer;
        }
    }
    ext_7F = FALSE;

    if ( p == NULL ) {
        /*
         *  Character not yet handled
         *  Save character as string and point to it
         */
        buffer[ 0 ] = (char) c;
        buffer[ 1 ] = '\0';
        p = buffer;
    }

    if ( !literal_space && !cat && *p == ' ' && last == ' ' ) {
        /*
         *  Collapse spaces in tokens
         */
        ++p;
    }

    /*
     *  Print the Text
     */
    if ( *p != '\n' && !cat && column + (int) strlen( p ) > Width ) {
        /*
         *  Line wrap
         */
        putchar( '\n' );
        column = 0;
    }
    
    while ( ( c = *p++ ) != '\0' ) {
        /*
         *  Print character, insert or drop spaces as appropriate
         */
        if ( c == '"' ) {
            quoted = !quoted;
        }

        if ( quoted || cat ) {
            /*
             *  Print char as is, no further translation
             */
            putchar( c );
        }
        else {
            if ( !literal_space && c == ' ' && *p == '\0' ) {
                /*
                 *  Token ends with a space
                 */
                insert_space = TRUE;
                c = 0;
            }
            else {
                /*
                 *  Printable character
                 */
                if ( insert_space && !parenthesis ) { 
                    putchar( ' ' );
                    ++column;
                }
                putchar( c );
                insert_space = FALSE;
            }
        }
        last = c;

        if ( c == '\n' ) {
            column = 0;
        }
        else if ( c != 0 ) {
            ++column;
        }
    }
}


/*
 *  Show the file name
 */
void printHeader( void )
{
    int i;
    unsigned char *p = Header.file_name;

    if ( Header.id != TYPE_HEADER ) {
        printf( "Unknown type\n" );
        return;
    }
    if ( *p == '\0' ) {
        printf( "Tape file has no name\n" );
    }
    else {
        printf( "Tape file name: \"" );
        for ( i = 0; 
              i < sizeof ( Header.file_name ) && *p != '\0';
              ++i )
        {
            casioprint( *p++ );
        }
        printf( "\"\n" );
    }
}


/*
 *  Convert a BCD coded byte to binary
 */
int bcd2bin( int c )
{
    int x;
    x = c / 16;
    return c - 6 * x;
}


/*
 *  Print a number (from SAVE M)
 *  If result is present, the string is put therein and not printed 
 */
char *printNumber( unsigned char *buffer, ENCODING enc, char *result )
{
    int exp, flag, i;
    char c, *fmt, *save = result;
    static char temp[] = "+1.234567890123\\EE+12";

    /*
     *  Use internal result buffer if not given as a paramter
     */
    if ( result == NULL ) {
        result = temp;
    }

    /*
     *  Sign flag
     */
    flag = buffer[ 7 ] & 0x0F;
    c = flag < 5 ? ' ' : 
        enc == ENC_INTERNAL ? 0x87 
        : '-';
    if ( c != ' ' || enc == ENC_ASCII ) {
        *result++ = c;
    }

    /*
     *  First digit
     */
    c = '0' + ( buffer[ 7 ] >> 4 );
    *result++ = c;
    *result++ = '.';

    /*
     *  Digits 2 to 13
     */
    for ( i = 6; i > 0; --i ) {
        sprintf( result, "%02.2X", buffer[ i ] );
        result += 2;
    }

    /*
     *  calculate exponent
     */
    exp = bcd2bin( *buffer );

    if ( exp != 0 && ( flag == 0 || flag == 5 ) ) {
        exp -= 100;
    }
    if ( exp <= 9 && exp > 0 ) {
        /*
         *  Adjust the decimal point
         */
        for ( i = 0; i < exp; ++i ) {
            result[ i - 13 ] = result[ i - 12 ];
        }
        result[ i - 13 ] = '.';
        exp = 0;
    }

    /*
     *  Strip trailing zeros
     */
    for ( i = 0; i < 12 && result[ -1 ] == '0'; ++i, --result );
    if ( result[ -1 ] == '.' ) --result;
    *result = '\0';

    /*
     *  Append the exponent if non zero
     */
    if ( exp != 0 ) {
        if ( exp < 0 ) {
            fmt = enc == ENC_ESCAPE   ? "\\EE-%d" :
                  enc == ENC_INTERNAL ? "\x0F\x87%d" :
                  "e-%02.2d";
            exp = -exp;
        }
        else {
            fmt = enc == ENC_ESCAPE   ? "\\EE%d" :
                  enc == ENC_INTERNAL ? "\x0F%d" :
                  "e+%02.2d";
        }
        sprintf( result, fmt, exp );
    }

    /*
     *  Result
     */
    if ( save == NULL ) {
        printf( "%s", temp );
    }
    return save;
}


/*
 *  Print a CAT header
 */
void printCatHeader( int type, int length, void *p1, void *p2, void *p3 )
{
    static char head[] = 
        "%%Header Record\n"
        "Format:%s\n"
        "Communication SW:0\n"
        "Data Type:%s\n";

    static char text[] =
        "Capacity:%u\n"
        "File Name:%s\n"
        "Group Name:\n"
        "Password:";

    static char value[] =
        "Rows:%d\n"
        "Columns:%d\n"
        "Variable Name:%c\n"
        "Group Name:Variable\n"
        "Variable Type:R\n"
        "Variable Length:10\n";

    static char window[] =
        "Rows:1\n"
        "Columns:1\n"
        "Variable Name:%s\n"
        "Group Name:V-Win1\n"
        "Variable Type:R\n"
        "Variable Length:10\n";

    static char image[] =
        "Height:%d\n" 
        "Width:%d\n"
        "Data Name:Picture%d\n"
        "Group Name:\n"
        "Start Position:DR\n"
        "Direction:U\n"
        "Byte Direction:W\n"
        "Bit Weight:F\n"
        "Colors:4\n"
        "Sheets:1\n";

    static char option[] = 
        "Option1:%s\n"
        "Option2:\n"
        "Option3:\n"
        "Option4:\n";

    static char data[] = "%%Data Record\n";

    unsigned char *p;

    switch ( type ) {

    case TYPE_PROGRAM:
    case TYPE_EDITOR:
        /*
         *  Program or text file
         */
        printf( head, "TXT", "PG" );

        /*
         *  Name and password
         */
        printf( text, length + 2, p1 );
        for( p = p2; p != NULL && *p != 0; ++p ) {
            casioprint( *p );
        }
        casioprint( 0xff );

        /*
         *  Program mode
         */
        printf( option, ProgOption1s[ (int) p3 ] );

        /*
         *  Program starts here
         */
        printf( data );
        break;

    case TYPE_MEMORY:
        /*
         *  Variable value
         */
        printf( head, "VAL", "VM" );
        if ( memcmp( p2, Zero, 8 ) == 0 ) {
            printf( value, 0, 0, p1 );
            printf( option, "" );
            printf( data);
        }
        else {
            printf( value, 1, 1, p1 );
            printf( option, "" );
            printf( data );
            printf( "Value : 1 1 " );
            printNumber( p2, ENC_ESCAPE, NULL );
            printf( "\n" );
        }
        break;

    case TYPE_RANGE:
        /*
         *  Range / Window
         */
        printf( head, "VAL", "WD" );
        printf( window, p1 );
        printf( option, "" );
        printf( data );
        printf( "Value : 1 1 " );
        printNumber( p2, ENC_ESCAPE, NULL );
        printf( "\n" );
        break;
         
    case TYPE_GRAPH:
        /*
         *  Image
         */
        printf( head, "IMG", "PC" );
        printf( image, p1, p2, p3 );
        printf( option, "" );
        printf( data );
        /*
         *  Image data follows
         */
        break;
    }
}


/*
 *  Print a CAT terminator
 */
void printCatTail( void )
{
    static char tail[] = "%%End\n";

    printf( tail );
}


/*
 *  Dump a single color image record
 */
void dumpBitmap( int color, int width, int height )
{
    static char image_data[] =
        "Sheet:1\n"
        "Color:%d\n";

    int i; 
    unsigned char *p = BitmapImage;

    printf( image_data, color );

    for ( i = 0; i < width * height / 8; ++i ) {
        printf( "%02.2X", *p++ );
        if ( i % 16 == 15 ) {
            putchar( '\n' );
        }
    }
}


/*
 *  Export the Image in CAT format
 *
 *  Picture1 is the picture in orange centered on a 128x64 screen
 *  Picture2 is the picture in blue   centered on a 128x64 screen
 *  Picture3 is the picture in green  centered on a 128x64 screen
 *  Picture4 is the original picture 96x64 (blue)
 */
void exportImages( void )
{
    unsigned char *p, *source;
    int x, y, x0;
    unsigned char mask, source_mask;
    int bit, pass;

    for ( pass = 1; pass <= 4; ++pass ) {
        /*
         *  Analyze the Image and convert it to a CAT IMG record.
         *  The bits are stored right to left, bottom to top, inverted.
         *  One byte is 2x4 rows, high nibble comes first.
         */
        int height = G_HEIGHT;
        int width  = pass == 4 ? G_WIDTH : 128;
        int source_width = G_WIDTH;
        int size = height * width / 8;
        int x_offset = ( width - source_width ) / 2;
        int color;

        /*
         *  The data record contains 4 images for the colors 
         *  orange, blue, green and white. 
         *  Only one color is filled with actual data.
         */
        p = BitmapImage + x_offset * height / 8;
        memset( BitmapImage, 0, size );

        printCatHeader( TYPE_GRAPH, 0, 
                        (void *) height, (void *) width, (void *) pass );

        /*
         *  Empty records for unused colors
         */
        for ( color = 1; color < ( pass < 4 ? pass : 2 ); ++color ) {
            dumpBitmap( color , width, height );
        }

        for ( x0 = x_offset; x0 < width - x_offset; x0 += 8 ) {
            /*
             *  Casio bitmaps are organized in 8 pixel width columns
             *  bottom to top, right to left
             */
            source_mask = 0x10;
            source = Graphic;

            for ( y = 0; y < height; ++y, ++p ) {
                /*
                 *  Bottom to top ordering of lines is the same
                 */
                for ( x = x0, mask = 1; x < x0 + 8; ++x ) {
                    /*
                     *  Pixels are stored rightmost first
                     */
                    bit = ~source[ x - x_offset ] & source_mask;
                    if ( bit ) {
                        *p |= mask;
                    }
                    mask <<= 1; 
                }
                source_mask <<= 1;
                if ( source_mask == 0 ) {
                    source_mask = 1;
                }
                else if ( source_mask == 0x10 ) {
                    /*
                     *  Next eight rows
                     */
                    source += source_width;
                }
            }
        } 

        /*
         *  Dump actual image
         */
        dumpBitmap( color, width, height );  

        /*
         *  Empty records for unused colors
         */
        memset( BitmapImage, 0, size );
        while ( ++color <= 4 ) {
            dumpBitmap( color , width, height );
        }
        printCatTail();
    }
    return;
}


/*
 *  Create a program name from the program #
 */
char *makeName( char *name, char id )
{
    char *p;

    if ( NewName != NULL ) {
        strncpy( name, NewName, 8 );
    }
    else {
        memcpy( name, Header.file_name, 8 );
    }
    if ( strlen( name ) == 0 ) {
        strcpy( name, "PROG#" );
    }
    /*
     *  Replace '#' or trailing digit by program number
     *  Program 0 does not neccessarily carry digit '0'
     */
    p = strchr( name, '#' );
    if ( p == NULL ) {
        p = name + strlen( name ) - 1;
    }
    if ( id != '0' || *p == '#' ) {
        if ( p < name + 7 && *p != '#' && !isdigit( *p ) ) {
            ++p;
        } 
        *p = id;
    }
    return name;
}


/*
 *  Store a token in the file / program buffer
 *  Some tokens must be replaced by strings beforehand, because they are
 *  unknown to the .CAT file format. The problem is that we have to adjust the 
 *  raw length correctly.
 */
unsigned char *storeToken( int c, unsigned char* ptr )
{
    char *p = NULL;
    static int last_token;
    
    if ( c < ' ' || c > 0x7f ) {
        p = findToken( c, NULL )->escape;
    }
    if ( p == NULL || *p == '\\' || strlen( p ) == 1 ) {
        /*
         *  store token as is
         */
        *ptr++ = (unsigned char) c;
    }
    else {
        /*
         *  No real escape sequence exists, copy the text
         */
        while ( *p != '\0' ) *ptr++ = *p++;
    }

    if ( last_token == 0xED && c != '"' ) {
        /*
         *  Replace Prog n by Prog "PROGn"
         *  Newer calcs do not support program areas
         */
        char name[ 8 + 1 ] = "";

        --ptr;
        if ( c == ' ' ) {
            /*
             *  Ignore any spaces
             */
            return ptr;
        }
        /*
         *  Set program name
         */
        *ptr++ = '"';
        p = makeName( name, c );
        while ( *p != '\0' ) *ptr++ = *p++;
        *ptr++ = '"';
    }

    if ( c == '[' && ptr[ -2 ] >= 'A' && ptr[ -2 ] <= 'Z' ) {
        /*
         *  Replace array names by List 1
         */
        int offset;
        ptr -= 2;
        offset = *ptr - '@';
        *ptr++ = 0x7F;
        *ptr++ = 0121;   /* List */
        *ptr++ = '1';
        *ptr++ = '[';
        sprintf( ptr, "%d", offset );
        ptr += strlen( ptr );
        *ptr++ = 0x89;  /* + */
    }
    last_token = c;
    return ptr;
}


/*
 *  Dump a text file / program
 */
void dumpFile( int length )
{
    unsigned char *p = File.data;
    printCatHeader( TYPE_EDITOR, length,
                    File.file_name, File.password, 
                    (void *) File.mode );
    while ( length-- ) {
        casioprint( *p++ );
    }
    putchar( '\n' );
    printCatTail();
}


/*
 *  Handle next character from file.
 *  This is for a fx-8000G tape recording.
 *
 *  Returns 0 when OK or the error code when invalid data encountered
 */
int list( int c )
{
    static int skipped = 5;
    static int record_type = 0;
    static int idle_counter = 0;
    static int data_counter = 0;
    static int length_counter = 0;
    static int block_length = 0;
    static unsigned char *file_ptr;
    static int file_size;
    static int prog_no = -1;
    static int editor_count = 0;
    static unsigned char number[ 8 ];

    static bool leader_expected = TRUE;
    static bool dataseg_expected = FALSE;
    static bool ignore_lead_in = FALSE;
    static bool ignore_password = FALSE;
    static bool echo = FALSE;

    char *p;
    int i, err;
    bool cat = TransMode == TRANS_CAT;
    bool cas = CasFile != NULL;
    bool is_zero;

    if ( !BinMode ) {
        /*
         *  Skip first 5 bytes before the leader
         */
        if ( skipped > 0 ) {
            skipped--;
            return 0;
        }

        /*
         *  Handle the leader
         */
        if ( c == KCS_LEAD_IN ) {
            /*
             *  Idle string (leader)
             */
            if ( ignore_lead_in ) {
                /*
                 *  Data blocks have short lead-in sequences
                 *  after the id and after the length which we ignore
                 */
                return 0;
            }
            if ( idle_counter == 0 && !leader_expected ) {
                /*
                 *  "Premature end of segment" error
                 */
                return 2;
            }
            if ( idle_counter++ >= 8 ) {
                /*
                 *  Leader contains at least 8 idle strings
                 */
                leader_expected = FALSE;
            }
            data_counter = 0;
            return 0;
        }
        else {
            /*
             *  Data byte received
             */
            idle_counter = 0;
            ignore_lead_in = FALSE;
            if ( leader_expected ) {
                /*
                 *  "Leader expected" error
                 */
                return 1;
            }
            /*
             *  Strip framing information
             */
            c &= 0xff;
        }
    }
    else {
        /*
         *  Binary mode
         */
        if ( leader_expected ) {
            /*
             *  We expect the start of a block
             */
            ignore_lead_in = FALSE;

            if ( ( c & 0xF0 ) == 0xF0 || c == TYPE_HEADER ) {
                /*
                 *  The header blocks starts with 0x08
                 *  Everything starting with 0xF_ is a data segment
                 */
                data_counter = 0;
                leader_expected = FALSE;
            }
            else {
#if DEBUG
                printf( " skipped\n" );
#endif                    
                return 0;
            }
        }
    }

#if DEBUG
     printf( "<%02.2X,%d>", c, data_counter + 1 );
#endif
    if ( ++data_counter == 1 ) {
        /*
         *  Handle the first segment byte
         */
        record_type = c;
#if DEBUG
        printf( "Record type = %X\n", record_type );
#endif
        switch ( c ) {

        case TYPE_HEADER:
            /*
             *  Filename follows
             */
            Header.id = (unsigned char) c;
            block_length = HEADER_SIZE - 1;
            length_counter = 0;
            return 0;

        case TYPE_PROGRAM:
        case TYPE_MEMORY:
        case TYPE_GRAPH:
        case TYPE_EDITOR:
            /*
             *  Length follows after short lead-in
             */
            ignore_lead_in = TRUE;
            length_counter = 2;
            return 0;

        case TYPE_EOF:
            /*
             *  Last segment
             */
            block_length = 1;
            length_counter = 0;
            break;

        default:
            /*
             *  Unknown Id
             */
            return 5;
        }
        if ( !cat ) {
            casioprint( 0xff );
        }
    }

    if ( length_counter > 0 ) {
        /*
         *  Get length of block
         */
        if ( --length_counter == 1 ) {
            /*
             *  LSB
             */
            block_length = c;
        }
        else {
            /*
             *  MSB
             */
            block_length += c * 256;
#if DEBUG
            printf( "Block length = %d\n", block_length );
#endif
            if ( block_length == 0 ) {
                /*
                 *  Empty block
                 */
                leader_expected = TRUE;
            }
            else {
                /*
                 *  There is a short lead-in after the length
                 */
                ignore_lead_in = TRUE;
            }
        }
        return 0;
    }

    if ( cas && prog_no >= 0 
      && ( CasMode != CAS_7700 && record_type != TYPE_PROGRAM 
        || CasMode == CAS_7700 && record_type == TYPE_EOF )
       )
    {
        /*
         *  Dump the program space into open CAS file
         */
        while ( ++prog_no < 38 ) {
            /*
             *  Fill unused program space
             */
            *file_ptr++ = 0xFF;
            fillProgHeader( CasProgTable.header + prog_no, 1, 0 );
        }

        err = writeCasPrograms( file_ptr - File.data + 1 );
        if ( err != 0 ) {
            return err;
        }
        prog_no = -1;
    }

    if ( cas && editor_count > 0 && record_type != TYPE_EDITOR ) {
        /*
         *  Add EOF record to list of editor files
         */
        err = writeCasEof( "\x17", 1, CAS_HEADER_SIZE );
        if ( err != 0 ) {
            return err;
        }
        editor_count = 0;
    }

    /*
     *  Treat bytes according to record type
     */
    switch ( record_type ) {

    case TYPE_HEADER:
        /*
         *  Header is followed by filename and delimiter
         */
        ((unsigned char *) &Header)[ data_counter - 1 ] 
            = (unsigned char) c;

        if ( c == 0x00 ) {
            /*
             *  End of header
             */
            if ( !cat ) {
                printHeader();
            }
            dataseg_expected = TRUE;
            block_length = 1;
        }
        break;

    case TYPE_PROGRAM:
        /*
         *  Program area
         */
        if ( cat ) {
            /*
             *  Export to CAT file
             */
            if ( data_counter == 4 ) {
                /*
                 *  Check type
                 */
                File.mode = c >> 5;
                ++prog_no;
                
                /*
                 *  Set filename
                 */
                makeName( File.file_name, '0' + prog_no + ProgramNumber );
                file_ptr = File.data;
            }
            else {
                /*
                 *  Store byte in buffer
                 */
                file_ptr = storeToken( c, file_ptr );
            }
            if ( block_length == 1 ) {
                /*
                 *  Done with this program
                 */
                if ( c != 0x0D ) {
                    *file_ptr++ = 0x0D;
                }
                dumpFile( file_ptr - File.data );
                file_ptr = NULL;
            }
        }
        else if ( cas ) {
            /*
             *  Export program to CAS file
             */
            if ( CasMode == CAS_ALL || CasMode == CAS_PROGRAM 
              || CasMode == CAS_7700 )
            {
                /*
                 *  Save program contents for later
                 */
                if ( data_counter == 4 ) {
                    /*
                     *  Check type
                     */
                    if ( ++prog_no == 0 ) {
                        /*
                         *  Start of program block
                         *  Leave room for the colon!
                         */
                        file_ptr = File.data + 1;
                        while ( prog_no < ProgramNumber ) {
                            /*
                             *  Fill unused program space
                             */
                            *file_ptr++ = 0xFF;
                            fillProgHeader( CasProgTable.header + prog_no, 
                                            1, 0 );
                            ++prog_no;
                        }
                    }
                    File.mode = c >> 5;
                    file_size = 0;
                }
                else {
                    /*
                     *  Store byte in buffer
                     */
                    *file_ptr++ = c;
                    ++file_size;
                }
                if ( block_length == 1 ) {
                    /*
                     *  Done with this program
                     */
                    *file_ptr++ = 0xFF;
                    ++file_size;
                    fillProgHeader( CasProgTable.header + prog_no, 
                                    file_size, File.mode );
                }
            }
        }
        else {
            /*
             *  List to screen
             */
            if ( data_counter == 4 ) {
                /*
                 *  Show type
                 */
                File.mode = c >> 5;
                ++prog_no;
                p = ProgModes[ File.mode ];
                printf( "Program %d, mode ", prog_no + ProgramNumber );
                if ( p != NULL ) {
                    printf( "%s", p );
                }
                else {
                    printf( "\\x%2.2X", c );
                }
                printf( "\n\n" );
            }
            else {
                casioprint( c );
            }
        }
        break;

    case TYPE_EDITOR:
        /*
         *  Data file
         */
        if ( cat ) {
            if ( data_counter == 4 ) {
                /*
                 *  File name follows
                 */
                file_ptr = File.file_name;
                memset( File.password, 0, sizeof( File.password ) );
            }
            if ( c == 0x00 ) {
                /*
                 *  Password starts here
                 */
                *file_ptr = '\0';
                file_ptr = File.password;
            }
            else if ( c == 0x01 ) {
                /*
                 *  Data starts with next byte;
                 */
                *file_ptr = '\0';
                file_ptr = File.data;
            }
            else if ( c != 0xFF ) {
                /*
                 *  Store byte in buffer
                 */
                file_ptr = storeToken( c, file_ptr );
            }
            if ( c == 0xFF || block_length == 1 ) {
                /*
                 *  Done with this file
                 */
                ++editor_count;
                dumpFile( file_ptr - File.data );
                file_ptr = NULL;
            }
        }
        else if ( CasMode == CAS_ALL || CasMode == CAS_EDITOR ) {
            /*
             *  Export editor file to CAS file
             */
            if ( data_counter == 4 ) {
                /*
                 *  File name follows
                 */
                file_ptr = File.file_name;
                memset( File.password, 0, sizeof( File.password ) );
            }
            if ( c == 0x00 ) {
                /*
                 *  Password starts here (not written to CAS file)
                 */
                *file_ptr = '\0';
                file_ptr = File.password;
            }
            else if ( c == 0x01 ) {
                /*
                 *  Data starts with next byte;
                 */
                *file_ptr = '\0';
                file_ptr = File.data + 1;  /* colon! */
            }
            else if ( c != 0xFF ) {
                /*
                 *  Store byte in buffer
                 */
                *file_ptr++ = c;
            }
            if ( c == 0xFF || block_length == 1 ) {
                /*
                 *  Done with this file, write it out
                 */
                *file_ptr++ = 0xFF;
                ++editor_count;
                err = writeCasFile( file_ptr - File.data + 1 );
                if ( err != 0 ) {
                    return err;
                }
                file_ptr = NULL;
            }
        }
        else if ( CasMode == CAS_7700 ) { 
            /*
             *  Editor to program conversion
             */
            if ( data_counter == 4 ) {
                /*
                 *  File name follows
                 */
                if ( ++prog_no == 0 ) {
                    /*
                     *  Start of program block
                     *  Leave room for the colon!
                     */
                    file_ptr = File.data + 1;
                }
                while ( prog_no < 10 ) {
                    /*
                     *  Fill unused program space
                     *  Start with Prog A
                     */
                    *file_ptr++ = 0xFF;
                    fillProgHeader( CasProgTable.header + prog_no, 
                                    1, 0 );
                    ++prog_no;
                }
                if ( prog_no < 38 ) {
                    printf( "p%c: ", prog_no + 'A' - 10 );
                    file_size = 1;
                    *file_ptr++ = '\''; /* start with comment */
                    ignore_password = FALSE;
                    echo = TRUE;
                }
            }
            if ( c == 0x00 ) {
                /*
                 *  Password starts here (not written to CAS file)
                 */
                ignore_password = TRUE;
            }
            else if ( c == 0x01 ) {
                /*
                 *  Data starts with next byte;
                 */
                if ( prog_no < 38 ) {
                    putchar( '\n' );
                    ++file_size;
                    *file_ptr++ = 0x0D; /* terminate the comment */
                    ignore_password = FALSE;
                    echo = FALSE;
                }
            }
            else if ( c != 0xFF && !ignore_password && prog_no < 38 ) {
                /*
                 *  Store byte in buffer
                 */
                ++file_size;
                *file_ptr++ = c;
                if ( echo ) {
                    casioprint( c );
                }
            }
            if ( c == 0xFF || block_length == 1 ) {
                /*
                 *  Done with this file, create program header
                 */
                if ( prog_no < 38 ) {
                    *file_ptr++ = 0xFF;
                    ++file_size;
                    fillProgHeader( CasProgTable.header + prog_no, 
                                    file_size, 0 );
                }
            }
        }
        else {
            if ( data_counter == 4 ) {
                /*
                 *  Show name
                 */
                printf( "File name: \"" );
                casioprint( c );
            }
            else if ( c == 0x00 ) {
                printf( "\"\nPassword: \"" );
            }
            else if ( c == 0x01 ) {
                printf( "\"\n\n" );
            }
            else {
                casioprint( c );
            }
        }
        break;

    case TYPE_MEMORY:
        /*
         *   Register file
         */
        if ( data_counter == 4 && !cat && !cas ) {
            /*
             *  Start of register list
             */
            printf( "Memory\n\n" );
        }
        i = ( data_counter - 4 ) % 8;
        number[ i ] = (unsigned char) c;
        if ( i == 7 ) {
            i = ( data_counter - 4 ) / 8;
            if ( cat ) {
                if ( i < 26 ) {
                    printCatHeader( record_type, 10, (void *) ( i + 'A' ), 
                                    number, NULL );
                    printCatTail();
                }
            }
            else if ( cas ) {
                /*
                 *  Export to CAS file
                 */
                if ( CasMode == CAS_ALL || CasMode == CAS_MEMORY ) {
                    /*
                     *  Memory block
                     */
                    if ( i == 0 ) {
                        /*
                         *  Write header
                         */
                        err = writeCasMemoryHeader( block_length / 8 + 3 );
                        if ( err != 0 ) {
                            return err;
                        }
                        /*
                         *  No data available for r and theta
                         */
                        err = writeCasNumber( Zero );
                        if ( err == 0 ) {
                            err = writeCasNumber( Zero );
                        }
                        if ( err != 0 ) {
                            return err;
                        }
                    }
                    /*
                     *  Dump the number to file
                     */
                    err = writeCasNumber( number );
                    if ( err != 0 ) {
                        return err;
                    }
                    if ( block_length == 1 ) {
                        /*
                         *  Done with memory, add EOF record
                         */
                        err = writeCasEof( "\x17", 1, CAS_NUMBER_SIZE );
                        if ( err != 0 ) {
                            return err;
                        }
                    }
                }
                else if ( CasMode == CAS_7700 ) { 
                    /*
                     *  Memory to program conversion
                     */
                    if ( i == 0 ) {
                        /*
                         *  Set pointers and create program head
                         */
                        if ( ++prog_no == 0 ) {
                            /*
                             *  Start of program block
                             *  Leave room for the colon!
                             */
                            file_ptr = File.data + 1;
                        }
                        while ( prog_no < 10 ) {
                            /*
                             *  Fill unused program space
                             *  Start with Prog A
                             */
                            *file_ptr++ = 0xFF;
                            fillProgHeader( CasProgTable.header + prog_no, 
                                            1, 0 );
                            ++prog_no;
                        }
                        if ( prog_no < 38 ) {
                            /*
                             *  Start with comment 
                             */
                            strcpy ( file_ptr, "'Memory\r0\x0E" "A~Z\r" );
                            file_size = strlen( file_ptr );
                            file_ptr += file_size;
                            if ( block_length / 8 > 25 ) {
                                /*
                                 *  Defm needed
                                 */
                                sprintf( file_ptr, "\xE5%d\r", block_length / 8 - 25 );
                                file_size += strlen( file_ptr );
                                file_ptr += strlen( file_ptr );
                            }
                            printf( "p%C: %d memories\n", 
                                    prog_no + 'A' - 10, block_length / 8 + 1 );
                        }
                    }
                    /*
                     *  Create assignment statement
                     */
                    if ( prog_no < 38 ) {
                        p = file_ptr;
                        is_zero = ( 0 == memcmp( Zero, number, 8 ) );
                        if ( !is_zero ) {
                            /*
                             *  Convert number to Casio text
                             */
                            file_ptr = printNumber( number, ENC_INTERNAL, file_ptr );
                        }
                        if ( i < 26 ) {
                            /*
                             *  A~Z
                             */
                            if ( !is_zero ) {
                                sprintf( file_ptr, "\x0E%c\r", 'A' + i );
                            }
                        }
                        else {
                            /*
                             *  Array memory beyond Z
                             */
                            if ( is_zero ) {
                                *file_ptr++ = '0';
                            }
                            sprintf( file_ptr, "\x0EZ[%d]\r", i - 26 );
                        }
                        file_ptr += strlen( file_ptr );
                        file_size += (char *) file_ptr - p;
                    }
                    if ( block_length == 1 ) {
                        /*
                         *  Done with this file, create program header
                         */
                        if ( prog_no < 38 ) {
                            *file_ptr++ = 0xFF;
                            ++file_size;
                            fillProgHeader( CasProgTable.header + prog_no, 
                                            file_size, 0 );
                        }
                    }
                }
            }
            else {
                /*
                 *  Dump variable to stdout
                 */
                if ( i < 26 ) {
                    printf( "%3d: %c = ", i + 1, i + 'A' ); 
                }
                else {
                    printf( "%3d: Z[%d] = ", i + 1, i - 26 );
                }
                printNumber( number, ENC_ASCII, NULL );
                putchar( '\n' );
            }
        }
        break;

    case TYPE_GRAPH:
        /*
         *  Graphic
         */
        if ( data_counter == 4 && !cat && !cas ) {
            /*
             *  Show type
             */
            printf( "Range:\n\n" );
        }
        if ( data_counter >= 4 && data_counter <= 9 ) {
            /*
             *  Range: Ymax
             */
            i = data_counter == 9 ? 0 : 7 - ( data_counter - 4 );
            Ymax[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 10 && data_counter <= 15 ) {
            /*
             *  Range: Xmax
             */
            i = data_counter == 15 ? 0 : 7 - ( data_counter - 10 );
            Xmax[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 16 && data_counter <= 21 ) {
            /*
             *  Range: Yscl
             */
            i = data_counter == 21 ? 0 : 7 - ( data_counter - 16 );
            Yscl[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 22 && data_counter <= 29 ) {
            /*
             *  Range: Ydot
             */
            i = 7 - ( data_counter - 22 );
            Ydot[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 30 && data_counter <= 35 ) {
            /*
             *  Range: Ymin
             */
            i = data_counter == 35 ? 0 : 7 - ( data_counter - 30 );
            Ymin[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 36 && data_counter <= 41 ) {
            /*
             *  Range: Xscl
             */
            i = data_counter == 41 ? 0 : 7 - ( data_counter - 36 );
            Xscl[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 42 && data_counter <= 49 ) {
            /*
             *  Range: Xdot
             */
            i = 7 - ( data_counter - 42 );
            Xdot[ i ] = (unsigned char) c;
        }
        else if ( data_counter >= 50 && data_counter <= 55 ) {
            /*
             *  Range: Xmin
             */
            i = data_counter == 55 ? 0 : 7 - ( data_counter - 50 );
            Xmin[ i ] = (unsigned char) c;

            if ( i == 0 ) {
                /*
                 *  Output the range information
                 */
                if ( cat ) {
                    /*
                     *  CAT file
                     */
                    printCatHeader( TYPE_RANGE, 10, "Xmin", Xmin, NULL );
                    printCatTail();
                    printCatHeader( TYPE_RANGE, 10, "Xmax", Xmax, NULL );
                    printCatTail();
                    printCatHeader( TYPE_RANGE, 10, "Xscl", Xscl, NULL );
                    printCatTail();
                    printCatHeader( TYPE_RANGE, 10, "Ymin", Ymin, NULL );
                    printCatTail();
                    printCatHeader( TYPE_RANGE, 10, "Ymax", Ymax, NULL );
                    printCatTail();
                    printCatHeader( TYPE_RANGE, 10, "Yscl", Yscl, NULL );
                    printCatTail();
                }
                else if ( cas ) {
                    /*
                     *  CAS file
                     */
                    if ( CasMode == CAS_ALL || CasMode == CAS_RANGE ) {
                        /*
                         *  Write range header + data block
                         */
                        err = writeCasRange();
                        if ( err != 0 ) {
                            return err;
                        }
                    }
                }
                else {
                    /*
                     *  Print it
                     */
                    printf( "  Xmin = " ); 
                    printNumber( Xmin, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Xmax = " ); 
                    printNumber( Xmax, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Xscl = " ); 
                    printNumber( Xscl, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Xdot = " ); 
                    printNumber( Xdot, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Ymin = " ); 
                    printNumber( Ymin, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Ymax = " ); 
                    printNumber( Ymax, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Yscl = " ); 
                    printNumber( Yscl, ENC_ASCII, NULL );
                    putchar( '\n' );
                    printf( "  Ydot = " ); 
                    printNumber( Ydot, ENC_ASCII, NULL );
                    putchar( '\n' );
                    putchar( '\n' );
                }
            }
        }
        else if ( data_counter >= 56 ) {
            /*
             *  Store pixels
             */
            Graphic[ data_counter - 4 - 0x34 ] = (unsigned char) c;
        }
        if ( block_length <= 1 ) {
            /*
             *  Create Bitmap
             */
            if ( cat ) {
                exportImages();
            }
            else if ( BitmapFile != NULL ) {
                i = createBitmap();
                if ( i != 0 ) {
                    return i;
                }
                if ( !cat ) {
                    printf( "File(s) %s created", BitmapFile ); 
                }
            }
        }
        break;

    case TYPE_EOF:
        /*
         *  Last segment on tape, prepare for new recording
         */
        if ( !cat && !cas ) {
            printf( "End of Recording" );
        }
        dataseg_expected = FALSE;
        break;
    }

    if ( --block_length == 0 ) {
        /*
         *  End of block
         */
        leader_expected = TRUE;
        data_counter = 0;
        if ( !cat && !cas ) {
            casioprint( 0xFF );
            /* printf( "\n" ); */
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
    FILE *infp = NULL;
    KCS_FILE *wfp = NULL;
    char *cas_file = NULL;
    int skip = 0;
    bool wavemode = FALSE;
    bool first = TRUE;
    bool ignore = TRUE;
    int baud = -2400;

    ++argv;
    --argc;

    while ( argc > 1 && **argv == '-' ) {

        if ( strncmp( *argv, "-b", 2 ) == 0 ) {
            BinMode = TRUE;
            skip = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-w", 2 ) == 0 ) {
            BinMode = FALSE;
            wavemode = TRUE;
            Fx8000G = TRUE;
            if ( (*argv)[ 2 ] == '-' ) {
                baud = -baud;
            }
        }
        if ( strncmp( argv[ 1 ], "-i", 2 ) == 0 ) {
            ignore = TRUE;
        }
        if ( strncmp( *argv, "-e", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'C':
                EscapeMode = ESC_CHAR; break;
            case 'X':
            case 'H':
            case '\0':
                EscapeMode = ESC_HEX; break;
            }
        }
        if ( strncmp( *argv, "-cat", 4 ) == 0 ) {
            TransMode = TRANS_CAT;
            EscapeMode = ESC_CHAR;
            BitmapFile = NULL;
            if ( (*argv)[ 4 ] != '\0' ) {
                NewName = *argv + 4;
            }
        }
        else if ( strncmp( *argv, "-cas", 4 ) == 0 ) {
            switch ( toupper( (*argv)[ 4 ] ) ) {
            case 'P':
                CasMode = CAS_PROGRAM; break;
            case 'E':
                CasMode = CAS_EDITOR; break;
            case 'M':
                CasMode = CAS_MEMORY; break;
            case 'R':
                CasMode = CAS_RANGE; break;
            case '7':
                CasMode = CAS_7700; break;
            case 'A':
            case '\0':
                CasMode = CAS_ALL; break;
            }
        }
        else if ( strncmp( *argv, "-c", 2 ) == 0 ) {
            switch ( toupper( (*argv)[ 2 ] ) ) {
            case 'D':
                TransMode = TRANS_DOS; break;
            case 'W':
            case '\0':
                TransMode = TRANS_WINDOWS; break;
            }
        }
        if ( strncmp( *argv, "-l", 2 ) == 0 ) {
            Width = atoi( *argv + 2 );
            if ( Width == 0 ) Width = 40;
        }
        if ( strncmp( *argv, "-p", 2 ) == 0 ) {
            ProgramNumber = atoi( *argv + 2 );
        }
        if ( strncmp( *argv, "-g", 2 ) == 0 ) {
            BitmapFile = *argv + 2;
        }
        ++argv;
        --argc;
    }
#if DEBUG
    printf( "binmode = %d, baud = %d\n", BinMode, baud );
#endif

    if ( argc < 1 || TransMode == TRANS_CAT && CasMode != CAS_NONE ) {
        printf( "usage: list8000 <options> infile [outfile]\n"
                "         -w reads a WAV file directly.\n"
                "         -w- inverts the phase in case of difficulty.\n"
                "         -b<skip> reads a binary file, "
                          "<skip> is an optional offset.\n" 
                "         -i ignores any errors on the input stream\n"
                "         -e[C|X] use backslash escapes in strings:\n"
                "           C: character escapes like '\\Pi'.\n"
                "           X: generic hexadecimal escapes (default).\n"
                "         -c[D|W] use character set translation:\n"
                "           D: DOS code page 437.\n"
                "           W: Windows code page 1252 (default).\n"
                "         -c takes precedence over -e. Both can be mixed.\n"
                "         -l<width> limits the listing width. Default is 40.\n"
                "         -g<name#.bmp> creates bitmap files for any graphic.\n"
                "         -cas[A|P|E|M|R|7] translate output to CAS format for "
                          "FA-121 software.\n"
                "           A: Export all supported types (default).\n"
                "           P: Export program areas only.\n"
                "           E: Export editor files only.\n"
                "           M: Export variable memory only.\n"
                "           R: Export range data only.\n"
                "           7: fx-7700GB mode: convert editor files and memory"
                              " to programs.\n"
                "         -cat<prg#> translate output to CAT format for "
                          "FA-122/123/124 software.\n"
                "             <prg#> is an optional program name prefix.\n"
                "         -p<#> set the first program number.\n"
              );
        return 2;
    }

    if ( CasMode != CAS_NONE ) {
        /*
         *  Set file name for CAS file
         */
        cas_file = argc >= 2 ? argv[ 1 ] : "list8000.cas";
        fprintf( stderr, "Creating FA-121 file %s\n", cas_file );
    }
    else if ( argc >= 2 ) {
        /*
         *  Second argument is output file
         */
#ifdef _WIN32_WCE
        fprintf( stderr, "\nPlease use redirection for output file\n" );
                return 2;
#else
        freopen( argv[ 1 ], "wt", stdout );
#endif
    }

    if ( wavemode ) {
        /*
         *  Open WAV file
         */
        if ( ( wfp = kcsOpen( *argv, "rb", baud, 8, 'E', 2 ) ) == NULL )
        {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    else {
        /*
         *  Open binary file
         */
        if ( ( infp = fopen( *argv, "rb" ) ) == NULL ) {
            fprintf( stderr, "\nCannot open the file %s\n", *argv );
            perror( "error" );
            return 2;
        }
    }
    if ( cas_file != NULL ) {
        /*
         *  Open output file in *.CAS (FA-121) format
         */
        if ( ( CasFile = fopen( cas_file, "wb" ) ) == NULL ) {
            fprintf( stderr, "\nCannot open the file %s\n", cas_file );
            perror( "error" );
            return 2;
        }
        if ( CasMode == CAS_ALL ) {
            err_code = writeCasHeader( "AL", NULL, NULL );
            if ( err_code != 0 ) {
                return 2;
            }
        }
    }
    if ( wavemode ) {
        /*
         *  Process WAV file
         */
        while ( ( c1 = kcsReadByte( wfp ) ) >= 0 ) {
#if DEBUG_
            printf( "[%02.2X]", c1 );
#endif
            if ( ignore 
                 && c1 != KCS_LEAD_IN && c1 & ( KCS_FRAMING | KCS_PARITY ) ) 
            {
                continue;
            }
            err_code = Fx8000G ? list( c1 ) : 7;
            if ( err_code != 0 ) {
                printf( "\nInvalid data encountered \\%02.2X - %s.\n",
                        c1 & 0xFF, err_msg[ err_code ] );
                break;
            }
        }
        if ( c1 < 0 && c1 != KCS_EOF ) {
            fprintf( stderr, "Result: %d\n", c1 );
            if ( errno != 0 ) {
                perror( "read" );
            }
            return 2;
        }
        kcsClose( wfp );
    }
    else {
        /*
         *  Process binary file
         */
        while ( ( c1 = fgetc( infp ) ) != EOF ) {
#if DEBUG_
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
            if ( first ) {
                /*
                 *  Check if tape or CAS file
                 */
                Fx8000G = c1 != ':';
                first = FALSE;
            }
            err_code = Fx8000G ? list( c1 ) : 7;
            if ( err_code != 0 ) {
                /*
                 *  Error
                 */
                printf( "\nInvalid data @%04.4lX \\%02.2X - %s.\n",
                         position, c1, err_msg[ err_code ] );
                break;
            }
            ++position;
        }
        fclose( infp );
        if ( CasMode == CAS_ALL ) {
            /*
             *  CAS "ALL" files have an EOF record
             */
            err_code = writeCasEof( "\x17\x17\x00", 3, CAS_HEADER_SIZE );
            if ( err_code != 0 ) {
                return 2;
            }
        }
        if ( CasFile != NULL ) {
            fclose( CasFile );
        }
    }

    return 0;
}

