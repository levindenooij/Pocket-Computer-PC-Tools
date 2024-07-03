/*
 *   md100.c - Handle Casio MD100 disk images
 *
 *   Marcus von Cube
 *
 *   16.03.2006 0.1 created
 *   25.03.2006 1.0 first fully functional release
 *   30.03.2006 1.1 Direct floppy access (LIBDSK)
 *                  Works only for windows
 *   16.05.2008 1.2 dosdisk.c by Piotr Piatek
 *                  last block always padded with '\0'
 *   03.01.2008 1.3 Escape syntax in BASIC listings
 *   09.12.2010 1.4 added -eC for compatibility and pipe input for put        
 *   14.03.2015 1.5 new info about "flag" and "null" byte in directory
 *                  FAT mask and variable disk size
 */

#define DEBUG 0

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#if defined(_WIN32_WCE)
#include <windef.h>
#define FILENAME_MAX MAX_PATH
#endif

/*
 *  Some compiler specifics
 */
#if defined(__unix__) 
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#define memicmp  strncasecmp
#elif defined(_WIN32_WCE)
#define stricmp  _stricmp
#define strnicmp _strinicmp
#define memicmp  _memicmp
#endif

/*
 *  Constants
 */
#define SIZE_SECTOR 256
#define SIZE_BLOCK ( 4 * SIZE_SECTOR )
#define DEFAULT_BLOCKS ( 80 * 4 )
#define MAX_BLOCKS ( 128 * 4 )
#define MIN_BLOCKS 5
#define BLOCKS_FAT 1
#define BLOCKS_DIR 3
#define START_FAT 0
#define START_DIR 1
#define START_DATA 4
#define MAX_DIR_ENTRY ( BLOCKS_DIR * SIZE_BLOCK / 16 )
#define EOF_CHAR 0x1A
#define DEFAULT_UNUSED 0x1F
#define PROTECTED 0x01

#define OK 0
#define NOT_OK -1

#define TRUE 1
#define FALSE 0

/*
 *  Bit definitions for FAT entries
 */
#define FB_IN_USE  0x8000     /* marks a used entry */
#define FB_LAST    0x4000     /* marks end of chain */
#define FB_SECTORS 0x3000     /* number of last sector in last block */
#define FB_BLOCK   0x01FF     /* number of block (this or next in chain) */

/*
 *  File types
 */
#define TYPE_M 0x0D           /* machine code */
#define TYPE_B 0x10           /* tokenized BASIC */
#define TYPE_S 0x24           /* sequential ASCII file */
#define TYPE_R 0xA4           /* relative file for random access */
#define TYPE_C 0xD4           /* C source file (ASCII) */

/*
 *  Structure of directory entry
 */
typedef struct _dir_entry {
    unsigned char type;       /* one of TYPE_? */
    char          name[ 8 ];  /* blank padded, lower case is preserved */
    char          ext[ 3 ];   /* same here */
    unsigned char unused;     /* unused, non zero value */
    unsigned char block[ 2 ]; /* block number, points to FAT, MSB first */
    unsigned char protect;    /* bit 0 set: protected */
} DIR_ENTRY;

/*
 *  Some errors
 */
typedef enum _error {
    NO_ERROR = 0,
    FILE_EXISTS = 1,
    FILE_NOT_FOUND = 2,
    NO_ROOM = 3,
    IO_ERROR = 4
} ERROR;

const char *Error[] = {
    "OK",
    "File exists",
    "File not found",
    "No room for file",
    "I/O error"
};

/*
 *  File information
 */
typedef struct _file_info {
    char           name[ 13 ];  /* formatted filename */
    char           typeS[ 3 ];  /* "M", "B", "S", "R", "C" or "xx" */
    unsigned short block;       /* number of first block */
    unsigned short blocks;      /* total number of blocks */
    long           size;        /* size of file */
    unsigned char  type;        /* from directory */
    unsigned char  protect;     /* from directory */
    unsigned short next;        /* next block to read */
    long           remaining;   /* bytes to read */
    DIR_ENTRY      *entry;      /* original directory entry */
    ERROR          error;       /* Copy or Rename failed */
} FILE_INFO;

/*
 *  Option -l or -u
 */
typedef enum _set_case {
    AS_IS,
    LOWER,
    UPPER
} CASE;

/*
 *  Option -a or -b
 */
typedef enum _mode {
    AUTO,
    ASCII,
    BINARY
} MODE;

/*
 *  Option -e[N|H|S]
 */
typedef enum _escape_mode {
    NONE,
    HEX,
    SYMBOLS
} ESCAPE_MODE;

/*
 *  Options parsed from command line
 */
typedef struct _options {
    int           ignoreCase;  /* -i */
    CASE          setCase;     /* -l or -u */
    int           type;        /* -t */
    char          typeS[ 4 ];  /* -t as text */
    int           protect;     /* -p */
    MODE          mode;        /* -a or -b */
    ESCAPE_MODE   escape;      /* -e */
    char         *destination; /* -d */
    int           noUpdate;    /* -n */
    int           create;      /* -c */
    int           size;        /* argument to -c */
} OPTIONS;

#define DEFAULT_OPTIONS \
    { FALSE, AS_IS, 0, "", -1, AUTO, FALSE, NULL, FALSE, FALSE, DEFAULT_BLOCKS }

/*
 *  Types as Strings
 */
typedef struct _types {
    unsigned char type;
    char typeName[ 2 ];
} TYPES;

TYPES Types[] = {
    { TYPE_B, "B" },
    { TYPE_C, "C" },
    { TYPE_M, "M" },
    { TYPE_R, "R" },
    { TYPE_S, "S" },
    { 0, "" }
};

/*
 *  BASIC keywords for PB-1000
 *  Double byte tokens from 0x40 to 0xCF with prefixes 4 to 7
 */
char *Tokens[ 4 ][ 0xd0 - 0x40 ] = 
{ 
    { /* Prefix 4 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */  
        NULL,       NULL,       NULL,       NULL,       /* 44 */  
        NULL,       "GOTO ",    "GOSUB ",   "RETURN",   /* 48 */  
        "RESUME ",  "RESTORE ", "WRITE#",   NULL,       /* 4C */  
        NULL,       NULL,       "SYSTEM",   "PASS ",    /* 50 */  
        NULL,       "DELETE ",  "BSAVE ",   "LIST ",    /* 54 */  
        "LLIST ",   "LOAD ",    "MERGE ",   NULL,       /* 58 */  
        NULL,       "TRON",     NULL,       "TROFF",    /* 5C */  
        "VERIFY ",  "MON",      "CALL ",    "POKE ",    /* 60 */  
        NULL,       NULL,       NULL,       NULL,       /* 64 */  
        NULL,       "CHAIN ",   "CLEAR ",   "NEW ",     /* 68 */  
        "SAVE ",    "RUN ",     "ANGLE ",   "EDIT ",    /* 6C */  
        "BEEP ",    "CLS",      "CLOSE ",   NULL,       /* 70 */  
        NULL,       NULL,       "DEF ",     "DEFM ",    /* 74 */  
        "DEFSEG ",  NULL,       "VAC",      NULL,       /* 78 */  
        "DIM ",     "DRAW ",    NULL,       NULL,       /* 7C */  
        "DATA ",    "FOR ",     "NEXT ",    NULL,       /* 80 */  
        NULL,       "ERASE ",   "ERROR ",   "END",      /* 84 */  
        NULL,       NULL,       "FIELD ",   "FORMAT",   /* 88 */  
        "GET ",     "IF ",      NULL,       "LET ",     /* 8C */  
        "LINE ",    "LOCATE ",  NULL,       "LSET ",    /* 90 */  
        NULL,       NULL,       NULL,       "OPEN ",    /* 94 */  
        NULL,       "OUT ",     "ON ",      NULL,       /* 98 */  
        NULL,       NULL,       NULL,       "CALCJMP ", /* 9C */  
        "BLOAD ",   NULL,       "DRAWC ",   "PRINT ",   /* A0 */  
        "LPRINT ",  "PUT ",     NULL,       NULL,       /* A4 */  
        "READ ",    "REM ",     "RSET ",    NULL,       /* A8 */  
        "SET ",     "STAT",     "STOP",     NULL,       /* AC */  
        "MODE ",    NULL,       "VAR ",     "PBLOAD ",  /* B0 */  
        "PBGET ",   NULL,       NULL,       NULL,       /* B4 */  
        NULL,       NULL,       NULL,       NULL,       /* B8 */  
        NULL,       NULL,       NULL,       NULL,       /* BC */  
        NULL,       NULL,       NULL,       NULL,       /* C0 */  
        NULL,       NULL,       NULL,       NULL,       /* C4 */  
        NULL,       NULL,       NULL,       NULL,       /* C8 */  
        NULL,       NULL,       NULL,       NULL        /* CC */  
    },
    { /* prefix 5 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */
        NULL,       NULL,       NULL,       NULL,       /* 44 */
        NULL,       NULL,       NULL,       NULL,       /* 48 */
        NULL,       NULL,       NULL,       "ERL",      /* 4C */
        "ERR",      "CNT",      "SUMX",     "SUMY",     /* 50 */
        "SUMX2",    "SUMY2",    "SUMXY",    "MEANX",    /* 54 */
        "MEANY",    "SDX",      "SDY",      "SDXN",     /* 58 */
        "SDYN",     "LRA",      "LRB",      "COR",      /* 5C */
        "PI",       NULL,       NULL,       "CUR ",     /* 60 */
        NULL,       NULL,       NULL,       "FACT ",    /* 64 */
        NULL,       "EOX ",     "EOY ",     "SIN ",     /* 68 */
        "COS ",     "TAN ",     "ASN ",     "ACS ",     /* 6C */
        "ATN ",     "HYPSIN ",  "HYPCOS ",  "HYPTAN ",  /* 70 */
        "HYPASN ",  "HYPACS ",  "HYPATN ",  "LOG ",     /* 74 */
        "LGT ",     "EXP ",     "SQR ",     "ABS ",     /* 78 */
        "SGN ",     "INT ",     "FIX ",     "FRAC ",    /* 7C */
        "RND ",     NULL,       NULL,       NULL,       /* 80 */
        NULL,       NULL,       "PEEK ",    NULL,       /* 84 */
        NULL,       "LOF ",     "EOF ",     NULL,       /* 88 */
        NULL,       "FRE ",     NULL,       "POINT ",   /* 8C */
        "ROUND",    "RND",      "VALF",     "RAN#",     /* 90 */
        "ASC",      "LEN",      "VAL",      NULL,       /* 94 */
        NULL,       NULL,       NULL,       NULL,       /* 98 */
        "DEG",      NULL,       NULL,       NULL,       /* 9C */
        NULL,       NULL,       NULL,       NULL,       /* A0 */
        NULL,       NULL,       NULL,       "REC",      /* A4 */
        "POL",      NULL,       "NPR",      "NCR",      /* A8 */
        "HYP",      NULL,       NULL,       NULL,       /* AC */
        NULL,       NULL,       NULL,       NULL,       /* B0 */
        NULL,       NULL,       NULL,       NULL,       /* B4 */
        NULL,       NULL,       NULL,       NULL,       /* B8 */
        NULL,       NULL,       NULL,       NULL,       /* BC */
        NULL,       NULL,       NULL,       NULL,       /* C0 */
        NULL,       NULL,       NULL,       NULL,       /* C4 */
        NULL,       NULL,       NULL,       NULL,       /* C8 */
        NULL,       NULL,       NULL,       NULL        /* CC */
    },
    { /* prefix 6 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */
        NULL,       NULL,       NULL,       NULL,       /* 44 */
        NULL,       NULL,       NULL,       NULL,       /* 48 */
        NULL,       NULL,       NULL,       NULL,       /* 4C */
        NULL,       NULL,       NULL,       NULL,       /* 50 */
        NULL,       NULL,       NULL,       NULL,       /* 54 */
        NULL,       NULL,       NULL,       NULL,       /* 58 */
        NULL,       NULL,       NULL,       NULL,       /* 5C */
        NULL,       NULL,       NULL,       NULL,       /* 60 */
        NULL,       NULL,       NULL,       NULL,       /* 64 */
        NULL,       NULL,       NULL,       NULL,       /* 68 */
        NULL,       NULL,       NULL,       NULL,       /* 6C */
        NULL,       NULL,       NULL,       NULL,       /* 70 */
        NULL,       NULL,       NULL,       NULL,       /* 74 */
        NULL,       NULL,       NULL,       NULL,       /* 78 */
        NULL,       NULL,       NULL,       NULL,       /* 7C */
        NULL,       NULL,       NULL,       NULL,       /* 80 */
        NULL,       NULL,       NULL,       NULL,       /* 84 */
        NULL,       NULL,       NULL,       NULL,       /* 88 */
        NULL,       NULL,       NULL,       NULL,       /* 8C */
        NULL,       NULL,       NULL,       NULL,       /* 90 */
        NULL,       NULL,       NULL,       "DMS$",     /* 94 */
        NULL,       NULL,       "MID",      "INPUT ",   /* 98 */
        "MID$",     "RIGHT$",   "LEFT$",    NULL,       /* 9C */
        "CHR$",     "STR$",     NULL,       "HEX$",     /* A0 */
        NULL,       NULL,       NULL,       NULL,       /* A4 */
        "INKEY$",   "KEY",      NULL,       "DATE$",    /* A8 */
        "TIME$",    "CALC$",    NULL,       NULL,       /* AC */
        NULL,       NULL,       NULL,       NULL,       /* B0 */
        NULL,       NULL,       NULL,       NULL,       /* B4 */
        NULL,       NULL,       NULL,       NULL,       /* B8 */
        NULL,       NULL,       NULL,       NULL,       /* BC */
        NULL,       NULL,       NULL,       NULL,       /* C0 */
        NULL,       NULL,       NULL,       NULL,       /* C4 */
        NULL,       NULL,       NULL,       NULL,       /* C8 */
        NULL,       NULL,       NULL,       NULL        /* CC */
    },
    { /* prefix 7 */
        NULL,       NULL,       NULL,       NULL,       /* 40 */
        NULL,       NULL,       NULL,       " THEN ",   /* 44 */
        "ELSE ",    NULL,       NULL,       NULL,       /* 48 */
        NULL,       NULL,       NULL,       NULL,       /* 4C */
        NULL,       NULL,       NULL,       NULL,       /* 50 */
        NULL,       NULL,       NULL,       NULL,       /* 54 */
        NULL,       NULL,       NULL,       NULL,       /* 58 */
        NULL,       NULL,       NULL,       NULL,       /* 5C */
        NULL,       NULL,       NULL,       NULL,       /* 60 */
        NULL,       NULL,       NULL,       NULL,       /* 64 */
        NULL,       NULL,       NULL,       NULL,       /* 68 */
        NULL,       NULL,       NULL,       NULL,       /* 6C */
        NULL,       NULL,       NULL,       NULL,       /* 70 */
        NULL,       NULL,       NULL,       NULL,       /* 74 */
        NULL,       NULL,       NULL,       NULL,       /* 78 */
        NULL,       NULL,       NULL,       NULL,       /* 7C */
        NULL,       NULL,       NULL,       NULL,       /* 80 */
        NULL,       NULL,       NULL,       NULL,       /* 84 */
        NULL,       NULL,       NULL,       NULL,       /* 88 */
        NULL,       NULL,       NULL,       NULL,       /* 8C */
        NULL,       NULL,       NULL,       NULL,       /* 90 */
        NULL,       NULL,       NULL,       NULL,       /* 94 */
        NULL,       NULL,       NULL,       NULL,       /* 98 */
        NULL,       NULL,       NULL,       NULL,       /* 9C */
        NULL,       NULL,       NULL,       NULL,       /* A0 */
        NULL,       NULL,       NULL,       NULL,       /* A4 */
        NULL,       NULL,       NULL,       NULL,       /* A8 */
        NULL,       NULL,       NULL,       NULL,       /* AC */
        NULL,       NULL,       NULL,       NULL,       /* B0 */
        NULL,       NULL,       "TAB ",     NULL,       /* B4 */
        "CSR ",     "REV ",     "NORM ",    "ALL ",     /* B8 */
        " AS ",     "APPEND ",  NULL,       "OFF",      /* BC */
        " STEP ",   " TO ",     "USING ",   "NOT ",     /* C0 */
        " AND ",    " OR ",     " XOR ",    " MOD ",    /* C4 */
        NULL,       NULL,       NULL,       NULL,       /* C8 */
        NULL,       NULL,       NULL,       NULL        /* CC */
    }
};

/*
 *  Escape codes with \
 */
typedef struct _escapes {
    char *text;
    unsigned char token;
} ESCAPE;

ESCAPE Escapes[] = 
{
    { "YN", 0x5C },  /* Yen */
    { "_1", 0x80 },  /* graphics */
    { "_2", 0x81 },  /* graphics */
    { "_3", 0x82 },  /* graphics */
    { "_4", 0x83 },  /* graphics */
    { "_5", 0x84 },  /* graphics */
    { "_6", 0x85 },  /* graphics */
    { "_7", 0x86 },  /* graphics */
    { "_8", 0x87 },  /* graphics */
    { "#",  0x87 },  /* block */
    { "|1", 0x88 },  /* graphics */
    { "|2", 0x89 },  /* graphics */
    { "|3", 0x8A },  /* graphics */
    { "|4", 0x8B },  /* graphics */
    { "|5", 0x8C },  /* graphics */
    { "|6", 0x8D },  /* graphics */
    { "|7", 0x8E },  /* graphics */
    { "^",  0x90 },  /* up arrow */
    { "V",  0x91 },  /* down arrow */
    { "<-", 0x92 },  /* left arrow */
    { "->", 0x93 },  /* right arrow */
    { ".",  0xA5 },  /* dot */
    { "DG", 0xDF },  /* degree */
    { "TR", 0xE4 },  /* triangle */
    { "SP", 0xE8 },  /* spade */
    { "HT", 0xE9 },  /* heart */
    { "DI", 0xEA },  /* diamond */
    { "CL", 0xEB },  /* club */
    { "LD", 0xEC },  /* large dot */
    { "@",  0xED },  /* circle */
    { "/",  0x83 },  /* over */
    { "\\", 0xEF },  /* backslash */
    { "*",  0xF0 },  /* multiply */
    { "]",  0xFE },  /* grey block */
    { NULL, 0    }
};

/*
 *  Other global variables
 */
char *MyName;
char *DiskName;
int MustUpdate = FALSE;
int Blocks = DEFAULT_BLOCKS;
unsigned char FAT_Block[ SIZE_BLOCK * BLOCKS_FAT ];
unsigned char DirBlocks[ SIZE_BLOCK * BLOCKS_DIR ];
DIR_ENTRY *Directory = (DIR_ENTRY *) DirBlocks;
unsigned char Block[ SIZE_BLOCK ];

/*
 *  Local functions
 */
int usage( void );
int doCmd( int argc, char **argv, OPTIONS *options );
int parseOptions( int *argcp, char ***argvp, OPTIONS *options, char *opts );
int cmdDir(  int argc, char **argv, OPTIONS *options );
int cmdType( int argc, char **argv, OPTIONS *options );
int cmdGet(  int argc, char **argv, OPTIONS *options );
int cmdMget( int argc, char **argv, OPTIONS *options );
int cmdPut(  int argc, char **argv, OPTIONS *options );
int cmdMput( int argc, char **argv, OPTIONS *options );
int cmdDel(  int argc, char **argv, OPTIONS *options );
int cmdRen(  int argc, char **argv, OPTIONS *options );
int cmdSet(  int argc, char **argv, OPTIONS *options );
char *getFile( FILE_INFO *info, char *dest, MODE mode, ESCAPE_MODE escape );
FILE_INFO *putFile( char *source, char *dest, MODE mode, ESCAPE_MODE escape, 
                    CASE setCase, int type, int protect, int noUpdate );
void delFile( FILE_INFO *info );
FILE_INFO *renFile( FILE_INFO *info, char *newName, CASE setCase );
FILE_INFO *setFile( FILE_INFO *info, int type, int protect );
int printFile( FILE_INFO *info, MODE mode, ESCAPE_MODE escape, FILE *out );
void printToken( int c, ESCAPE_MODE esc, FILE *out );
FILE_INFO *fileInfo( DIR_ENTRY *dptr, FILE_INFO *info, CASE setCase );
DIR_ENTRY *findFile( char *pattern, int ignoreCase, int type, int protect );
int isWildcard( char *pattern );
void expandPattern( char *pattern, char expPattern[ 8 + 3 + 1 ],
                    CASE setCase );
int readFile( FILE_INFO *info );
int writeFile( FILE_INFO *info, int count, int noUpdate );
int diskFree( void );
int dirFree( void );
int findFreeBlock( int start );
int updateDisk( void );
int openDisk( char *name, int create, int size, int noUpdate );
int closeDisk( void );
int readBlocks( void *dest, int number, int count );
int writeBlocks( void *source, int number, int count );
void clearBlockBuffer( void );

/*
 *  Main entry point
 */
int main( int argc, char **argv )
{
    int result;
    OPTIONS options = DEFAULT_OPTIONS;

    MyName = *argv;
    ++argv;
    --argc;

    /*
     *  Parse options -c and -n which are allowed before the command
     */
    if ( OK != parseOptions( &argc, &argv, &options, "cn" ) ) {
        return usage();
    }

    if ( argc < 1 ) {
        return usage();
    }

    if ( options.create ) {
        /*
         *  Initialize the FAT in case we must create a fresh image
         */
        memset( FAT_Block, 0xFF, 8 );
    }

    /*
     *  Open the image
     */
    DiskName = *argv;
    ++argv;
    --argc;
    if ( OK != openDisk( DiskName, options.create, options.size, options.noUpdate ) ) {
        return 2;
    }

    /*
     *  Read FAT and directory blocks
     */
    result = readBlocks( FAT_Block, START_FAT, BLOCKS_FAT );
    if ( result == OK ) {
        result = readBlocks( DirBlocks, START_DIR, BLOCKS_DIR );
    }
    if ( result != OK ) {
        fprintf( stderr, "Can't read disk directory" );
        return 2;
    }

    /*
     *  Execute command
     */
    result = doCmd( argc, argv, &options );

    if ( result == OK && MustUpdate ) {
        /*
         *  Write FAT and directory blocks back
         */
        result = updateDisk();
    }

    /*
     *  done
     */
    if ( OK != closeDisk() ) {
        result = NOT_OK;
    }

    if ( result != OK ) {
        fprintf( stderr, "\007Command failed\n" );
        return 2;
    }
    return 0;
}


/*
 *  Help
 */
int usage( void )
{
    fprintf( stderr,
             "usage: %s <image> <cmd> <options> <parameters>\n"
             "       <image> holds the floppy data\n"
             "       <cmd> <parameters> is one of:\n"
             "         dir  <options> \"<md100-pattern>\"\n"
             "         type <options> <md100-file>\n"
             "         get  <options> <md100-file> [<pc-file>]\n"
             "         mget <options> \"<md100-pattern>\"\n"
             "         put  <options> <pc-file> [<md100-file>]\n"
             "              <pc-file> may be \"stdin\" or \"stdin.EXT\""
                            " to allow piping\n"
             "         mput <options> <pc-files>\n"
             "         del  <options> \"<md100-pattern>\"\n"
             "         ren  <options> <md100-file> <new name>\n"
             "         set  <options> \"<md100-pattern>\" -t<type> -p<protect>\n"
             "       <options> are:\n"
             "         -i   ignore the case of md100-file(s)\n"
             "         -l   make all files lowercase\n"
             "         -u   make all files uppercase\n"
             "         -tX  select or set type to X (B, C, M, R, S or hex)\n"
             "         -pN  select or set file protection (0, 1)\n"
             "         -b   force binary transfer\n"
             "         -a   force ASCII transfer\n"
             "         -eX  use escape syntax, X=N(one), H(ex) or S(ymbols)\n"
             "         -n   no updates are written to the image\n"
             "         -cS  create a new image if the file does not exist\n"
             "              S is the size (default 320, maximum 512)\n"
             "       Patterns are DOS style, use \"*.*\" for all files\n"
             "       Wildcard expansion on local files depends on platform\n",
             MyName );
    return 2;
}


/*
 *  Parse the command argument and call routine
 */
int doCmd( int argc, char **argv, OPTIONS *options )
{
    static struct _cmds {
        char *cmd;
        int (*fn)( int, char **, OPTIONS * );
        char *opts;
    } cmds[] = {
        /* cmd    routine   options   */
        { "dir",    cmdDir,  "nfpt   ilu " },
        { "type",   cmdType, "nfptabei   " },
        { "list",   cmdType, "nfptabei   " }, /* alias */
        { "get",    cmdGet,  "nfptabeilud" },
        { "mget",   cmdMget, "nfptabeilud" },
        { "put",    cmdPut,  "nfptabe lu " },
        { "mput",   cmdMput, "nfptabe lud" },
        { "del",    cmdDel,  "nfpt   i   " },
        { "delete", cmdDel,  "nfpt   i   " }, /* alias */
        { "ren",    cmdRen,  "nfpt   ilu " },
        { "rename", cmdRen,  "nfpt   ilu " }, /* alias */
        { "set",    cmdSet,  "nfpt   i   " },
        { NULL,     NULL,    NULL          }
    };
    struct _cmds *cptr = cmds;
    char *cmd;
    int result;

    /*
     *  First argument is command name, defaults to "dir"
     */
    if ( argc == 0 ) {
        cmd = "dir";
    }
    else {
        cmd = argv[ 0 ];
        ++argv;
        --argc;
    }

    /*
     *  Find the command
     */
    while ( cptr->cmd != NULL && 0 != stricmp( cptr->cmd, cmd ) ) {
        ++cptr;
    }

    if ( cptr->fn == NULL ) {
        /*
         *  not found
         */
        fprintf( stderr, "%s: Command \"%s\" not found\n", MyName, cmd );
        return usage();
    }

    /*
     *  parse the standard options
     */
    if ( OK != parseOptions( &argc, &argv, options, cptr->opts ) ) {
        return usage();
    }

    /*
     *  Execute the command
     */
    result = (*cptr->fn)( argc, argv, options );

    if ( result != OK || options->noUpdate ) {
        /*
         *  Updates are disabled
         */
        MustUpdate = FALSE;
    }

    return result;
}


/*
 *  Parse options -i, -u, -l, -tX, (-fXX obsolete), -pX, -a, -b, -e, -d, -n, -c
 */
int parseOptions( int *argcp, char ***argvp, OPTIONS *options, char *opts )
{
    TYPES *tptr;

    while ( *argcp > 0 && ***argvp == '-' ) {
        /*
         *  All options are seperate and start with "-"
         */
        char *arg = **argvp;
        char opt = tolower( arg[ 1 ] );

        if ( opt == 'n' ) {
            /*
             *  No updates please!
             */
            options->noUpdate = TRUE;
        }
        else if ( opt == 'c' ) {
            /*
             *  Create a fresh image (with optional size value)
             */
            int size;
            if ( arg[ 2 ] != '\0' ) {
                arg = arg + 2;
            }
            else if ( *argcp >= 2 ) {
                ++(*argvp);
                --(*argcp);
                arg = **argvp;
            }
            else {
                arg = NULL;
                size = DEFAULT_BLOCKS;
            }
            if ( arg != NULL && 1 == sscanf( arg, "%d", &size ) ) {
                if ( size < MIN_BLOCKS || size > MAX_BLOCKS ) { 
                    fprintf( stderr, "Invalid disk size for -c option\n" );
                    return NOT_OK;
                }
            }
            options->size   = size;
            options->create = TRUE;
        }
        else if ( opt == 'i' ) {
            /*
             *  Ignore case
             */
            options->ignoreCase = TRUE;
        }
        else if ( opt == 'l' ) {
            /*
             *  Make filenames lowercase
             */
            options->setCase = LOWER;
        }
        else if ( opt == 'u' ) {
            /*
             *  Make filenames uppercase
             */
            options->setCase = UPPER;
        }
        else if ( opt == 'a' ) {
            /*
             *  Transfers are ASCII
             */
            options->mode = ASCII;
        }
        else if ( opt == 'b' ) {
            /*
             *  Transfers are binary
             */
            options->mode = BINARY;
        }
        else if ( opt == 'e' ) {
            /*
             *  Escape syntax for ASCII files
             */
            switch ( toupper( arg[ 2 ] ) ) {

            case '\0':
            case 'S':
            case 'P':
            case 'C':
                options->escape = SYMBOLS;
                break;

            case 'H':
            case 'X':
                options->escape = HEX;
                break;

            case 'N':
                options->escape = NONE;
                break;

            default:
                fprintf( stderr, "Invalid value for -e option\n" );
                return NOT_OK;
            }
        }
        else if ( opt == 't' ) {
            /*
             *  Type
             */
            if ( arg[ 2 ] != '\0' ) {
                arg = arg + 2;
            }
            else if ( *argcp >= 2 ) {
                ++(*argvp);
                --(*argcp);
                arg = **argvp;
            }
            else {
                fprintf( stderr, "Missing value for -t option\n" );
                return NOT_OK;
            }
            if ( strlen( arg ) == 1 ) {
                /*
                 *  Convert type
                 */
                *arg = toupper( *arg );
                options->typeS[ 0 ] = *arg;

                for ( tptr = Types; tptr->type != 0; ++tptr ) {
                    if ( *arg == *tptr->typeName ) {
                        /*
                         *  Known type
                         */
                        options->type = tptr->type;
                        break;
                    }
                }
                if ( tptr->type == 0 ) {
                    /*
                     *  Unknown type
                     */
                    fprintf( stderr, "Invalid type for -t option\n" );
                    return NOT_OK;
                }
            }
            else {
                /*
                 *  Hex type
                 */
                int type;
                if ( 1 != sscanf( arg, "%2X", &type ) ) {
                    fprintf( stderr, "Invalid type for -t option\n" );
                    return NOT_OK;
                }
                options->type = (unsigned char) type;
                sprintf( options->typeS, "%02.2X", type );
            }
        }
        else if ( opt == 'f' ) {
            /*
             *  Former "flag" option: ignore
             */
        }
        else if ( opt == 'p' ) {
            int protect;
            if ( arg[ 2 ] != '\0' ) {
                arg = arg + 2;
            }
            else if ( *argcp >= 2 ) {
                ++(*argvp);
                --(*argcp);
                arg = **argvp;
            }
            else {
                arg = NULL;
                protect = 1;
            }
            if ( arg != NULL && 1 != sscanf( arg, "%X", &protect ) ) {
                fprintf( stderr, "Invalid value for -p option\n" );
                return NOT_OK;
            }
            options->protect = (unsigned char) protect;
        }
        else if ( opt == 'd' ) {
            /*
             *  Destination
             */
            if ( arg[ 2 ] != '\0' ) {
                arg = arg + 2;
            }
            else if ( *argcp >= 2 ) {
                ++(*argvp);
                --(*argcp);
                arg = **argvp;
            }
            else {
                fprintf( stderr, "Missing value for -d option\n" );
                return NOT_OK;
            }
            options->destination = arg;
        }
        else {
            /*
             *  unknown option
             */
            fprintf( stderr, "Unknown option %s\n", arg );
            return NOT_OK;
        }
        if ( opts != NULL && NULL == strchr( opts, opt ) ) {
            /*
             *  Option not allowed here
             */
            fprintf( stderr, "Option -%c not allowed\n", opt );
            return NOT_OK;
        }

        ++(*argvp);
        --(*argcp);
    }
    return OK;
}


/*
 *  dir <options> <md100-pattern>
 */
int cmdDir( int argc, char **argv, OPTIONS *options )
{
    char *pattern, *p;
    FILE_INFO *info = NULL;
    long total = 0;
    int blocks = 0;
    int files = 0;
    int count;

    if ( argc == 0 ) {
        /*
         *  Default "*.*"
         */
        pattern = "*.*";
        argc = 1;
    }
    else {
        /*
         *  First pattern in list
         */
        pattern = argv[ 0 ];
    }

    printf( "\n Directory of %s\n\n", DiskName );
    printf( "   Name     Type   Size  Blk  Prot\n" );

    /*
     *  List directory for all patterns on command line
     */
    while ( argc-- > 0 ) {
        /*
         *  Loop over all files
         */
        count = 0;
        p = pattern;
        while ( TRUE ) {
            info = fileInfo( findFile( p, options->ignoreCase,
                                          options->type,
                                          options->protect ),
                             info, options->setCase );
            if ( info == NULL ) {
                break;
            }
            p = NULL;
            ++count;
            ++files;

            printf( "%-12s %2s  %6ld %4d    %2X\n",
                    info->name, info->typeS, info->size,
                    info->blocks, info->protect );
            total += info->size;
            blocks += info->blocks;
        }
        if ( count == 0 ) {
            printf( "%-12s %2s  no files\n", pattern, options->typeS );
        }
        pattern = *++argv;
    }

    /*
     *  Statistics
     */
    printf( "\n Total:     %3d  %6ld %4d\n", files, total, blocks );

    files  = dirFree();
    blocks = diskFree();
    total  = (long) blocks * (long) SIZE_BLOCK;

    printf( " Free:      %3d  %6ld %4d\n", files, total, blocks );

    return OK;
}


/*
 *  type <options> <md100-pattern>
 */
int cmdType( int argc, char **argv, OPTIONS *options )
{
    char *pattern;
    FILE_INFO *info = NULL;
    int printNames;
    int count;
    char *p;

    if ( argc < 1 ) {
        return usage();
    }
    pattern = *argv;
    printNames = argc > 1 || isWildcard( pattern );

    /*
     *  Type all files for all patterns on command line
     */
    while ( argc-- > 0 ) {
        /*
         *  Loop over all files
         */
        count = 0;
        p = pattern;
        while ( TRUE ) {
            info = fileInfo( findFile( p, options->ignoreCase,
                                          options->type,
                                          options->protect ),
                             info, options->setCase );
            if ( info == NULL ) {
                break;
            }
            p = NULL;
            ++count;

            if ( printNames ) {
                printf( "\nFile %s, Type %s:\n\n", info->name, info->typeS );
            }
            if ( OK != printFile( info, options->mode, options->escape, 
                                  stdout ) ) 
            {
                fprintf( stderr, "%s: Error reading file\n", info->name );
                return NOT_OK;
            }
        }
        if ( count == 0 ) {
            fprintf( stderr, "%s: file not found\n", pattern );
        }
        pattern = *++argv;
    }
    return OK;
}


/*
 *  get <options> <md100-file> [<destination>]
 */
int cmdGet( int argc, char **argv, OPTIONS *options )
{
    char *source;
    char *dest;
    FILE_INFO *info;

    if ( argc < 1 || argc > 2 ) {
        return usage();
    }
    source = *argv;
    dest = argc == 2 ? argv[ 1 ] : options->destination;

    /*
     *  Copy a single file
     */
    info = fileInfo( findFile( source, options->ignoreCase,
                                       options->type,
                                       options->protect ),
                     NULL, options->setCase );
    if ( info == NULL ) {
        fprintf( stderr, "%s: File not found\n", source );
        return NOT_OK;
    }
    dest = getFile( info, dest, options->mode, options->escape );
    if ( dest == NULL ) {
        fprintf( stderr, "%s: Error copying file\n", info->name );
        return NOT_OK;
    }
    fprintf( stderr, "%-12s %2s copied to %s\n",
                     info->name, info->typeS, dest );
    return OK;
}


/*
 *  type <options> -d <destination> <md100-pattern> 
 */
int cmdMget( int argc, char **argv, OPTIONS *options )
{
    char *pattern;
    FILE_INFO *info = NULL;
    int count;
    int total = 0;
    char *p;
    char *dest;

    if ( argc < 1 ) {
        return usage();
    }
    pattern = *argv;

    /*
     *  Type all files for all patterns on command line
     */
    while ( argc-- > 0 ) {
        /*
         *  Loop over all files
         */
        count = 0;
        p = pattern;
        while ( TRUE ) {
            info = fileInfo( findFile( p, options->ignoreCase,
                                          options->type,
                                          options->protect ),
                             info, options->setCase );
            if ( info == NULL ) {
                break;
            }
            p = NULL;
            ++count;

            dest = getFile( info, options->destination, options->mode,
                            options->escape );
            if ( dest == NULL ) {
                fprintf( stderr, "%s: Error copying file\n", info->name );
                return NOT_OK;
            }
            printf( "%-12s %2s copied to %s\n",
                    info->name, info->typeS, dest );
        }
        if ( count == 0 ) {
            fprintf( stderr, "%s: file not found\n", pattern );
        }
        total += count;
        pattern = *++argv;
    }
    if ( total > 0 ) {
        printf( "%d file%s copied\n", total, total == 1 ? "" : "s" );
    }
    else {
        printf( "No files copied\n" );
    }
    return OK;
}


/*
 *  put <options> <pc-file> [<md100-file>]
 */
int cmdPut( int argc, char **argv, OPTIONS *options )
{
    char *source;
    char *dest;
    FILE_INFO *info;

    if ( argc < 1 || argc > 2 ) {
        return usage();
    }
    source = *argv;
    dest = argc == 2 ? argv[ 1 ] : NULL;

    /*
     *  Copy a single file
     */
    info = putFile( source, dest, options->mode,
                                  options->escape,
                                  options->setCase,
                                  options->type,
                                  options->protect,
                                  options->noUpdate );
    if ( info->error != NO_ERROR ) {
        fprintf( stderr, "%s: Error copying file: %s\n",
                         source, Error[ info->error ] );
        return NOT_OK;
    }
    fprintf( stderr, "%-12s %2s %d created from %s\n",
                     info->name, info->typeS, info->blocks, source );
    return OK;
}


/*
 *  mput <options> <pc-file> ...
 */
int cmdMput( int argc, char **argv, OPTIONS *options )
{
    char *source;
    char *dest;
    FILE_INFO *info;

    if ( argc < 1 ) {
        return usage();
    }
    dest = options->destination;

    /*
     *  Copy all files from command line
     */
    while ( argc-- > 0 ) {

        source = *argv++;
        if ( 0 == strcmp( source, DiskName ) ) {
            /*
             *  Exclude image name
             */
            continue;
        }

        info = putFile( source, dest, options->mode,
                                      options->escape,
                                      options->setCase,
                                      options->type,
                                      options->protect,
                                      options->noUpdate );
        if ( info->error != NO_ERROR ) {
            fprintf( stderr, "%s  Error copying file: %s\n",
                             source, Error[ info->error ] );
            return NOT_OK;
        }
        fprintf( stderr, "%-12s %2s %3d created from %s\n",
                         info->name, info->typeS, info->blocks, source );
    }
    return OK;
}


/*
 *  del <options> <md100-pattern>
 */
int cmdDel( int argc, char **argv, OPTIONS *options )
{
    char *pattern;
    FILE_INFO *info = NULL;
    int count;
    int total = 0;
    char *p;

    if ( argc < 1 ) {
        return usage();
    }
    pattern = *argv;

    /*
     *  Delete all files for all patterns on command line
     */
    while ( argc-- > 0 ) {
        /*
         *  Loop over all files
         */
        count = 0;
        p = pattern;
        while ( TRUE ) {
            info = fileInfo( findFile( p, options->ignoreCase,
                                          options->type,
                                          options->protect ),
                             info, options->setCase );
            if ( info == NULL ) {
                break;
            }
            p = NULL;
            ++count;

            delFile( info );
            printf( "%-12s %2s deleted\n", info->name, info->typeS );
        }
        if ( count == 0 ) {
            fprintf( stderr, "%s: file not found\n", pattern );
        }
        total += count;
        pattern = *++argv;
    }
    if ( total > 0 ) {
        printf( "%d file%s deleted\n", total, total == 1 ? "" : "s" );
    }
    else {
        printf( "No files deleted\n" );
        MustUpdate = FALSE;
    }
    return OK;
}


/*
 *  ren <options> <md100-pattern> <new-name-pattern>
 */
int cmdRen( int argc, char **argv, OPTIONS *options )
{
    char *pattern;
    char *newName;
    FILE_INFO *info = NULL;
    FILE_INFO *newInfo;
    int count;
    int unchanged;
    int total = 0;
    char *p;

    if ( argc < 2 ) {
        return usage();
    }
    pattern = *argv;
    newName = argv[ --argc ];

    /*
     *  Rename all files for all but one patterns on the command line
     *  to the name given by the last pattern
     */
    while ( argc-- > 0 ) {
        /*
         *  Loop over all files
         */
        count = 0;
        unchanged = 0;
        p = pattern;
        while ( TRUE ) {
            info = fileInfo( findFile( p, options->ignoreCase,
                                          options->type,
                                          options->protect ),
                             info, AS_IS );
            if ( info == NULL ) {
                break;
            }
            p = NULL;

            newInfo = renFile( info, newName, options->setCase );
            if ( newInfo->error == FILE_EXISTS ) {
                printf( "%-12s %-2s duplicate: %-12s %-2s\n",
                        info->name, info->typeS,
                        newInfo->name, newInfo->typeS );
                return NOT_OK;
            }
            ++count;

            if ( 0 == strcmp( info->name, newInfo->name ) ) {
                ++unchanged;
                printf( "%-12s %-2s not changed\n", info->name, info->typeS );
            }
            else {
                printf( "%-12s %-2s renamed to %-12s %-2s\n",
                        info->name, info->typeS,
                        newInfo->name, newInfo->typeS );
            }
        }
        if ( count == 0 ) {
            fprintf( stderr, "%s: file not found\n", pattern );
        }
        total += count - unchanged;
        pattern = *++argv;
    }
    if ( total > 0 ) {
        printf( "%d file%s renamed\n", total, total == 1 ? "" : "s" );
    }
    else {
        printf( "No files renamed\n" );
        MustUpdate = FALSE;
    }
    return OK;
}


/*
 *  set <options> <md100-pattern> -t<type> -p<protect> -f (obsolete)
 */
int cmdSet( int argc, char **argv, OPTIONS *options )
{
    char *pattern;
    OPTIONS newOptions = DEFAULT_OPTIONS;
    FILE_INFO *info = NULL;
    FILE_INFO *newInfo;
    int count;
    int total = 0;
    int unchanged;
    char *p;
    int argc2 = argc;
    char **argv2 = argv;

    if ( argc < 2 ) {
        return usage();
    }

    /*
     *  Evaluate options -f and/or -t
     */
    while ( argc2-- > 0 && **++argv2 != '-' );

    if ( argc2 == 0 ) {
        fprintf( stderr, "Required options -f and/or -t missing" );
        return NOT_OK;
    }
    argc -= argc2;

    if ( OK != parseOptions( &argc2, &argv2, &newOptions, "fpt" ) ) {
        return NOT_OK;
    }

    pattern = *argv;

    /*
     *  Set attributes for all files for all patterns on the command line
     *  Attributes are given as options after the patterns
     */
    while ( argc-- > 0 ) {
        /*
         *  Loop over all files
         */
        count = 0;
        unchanged = 0;
        p = pattern;
        while ( TRUE ) {
            info = fileInfo( findFile( p, options->ignoreCase,
                                          options->type,
                                          options->protect ),
                             info, options->setCase );
            if ( info == NULL ) {
                break;
            }
            p = NULL;
            ++count;

            if ( newOptions.type    == info->type
              && newOptions.protect == info->protect )
            {
                ++unchanged;
                printf( "%-12s %2s P=%X not changed\n",
                        info->name, info->typeS, info->protect );
            }
            else {
                newInfo = setFile( info, newOptions.type, newOptions.protect );
                printf( "%-12s %2s P=%X changed to %2s P=%X\n",
                        info->name, info->typeS, info->protect,
                        newInfo->typeS, newInfo->protect );
            }
        }
        if ( count == 0 ) {
            fprintf( stderr, "%s: file not found\n", pattern );
        }
        total += count - unchanged;
        pattern = *++argv;
    }
    if ( total > 0 ) {
        printf( "%d file%s changed\n", total, total == 1 ? "" : "s" );
    }
    else {
        printf( "No files changed\n" );
    }
    return OK;
}


/*
 *  Expand a file-pattern, DOS like, "*" is equivalent to "*.*"
 */
void expandPattern( char *pattern, char expPattern[ 8 + 3 + 1 ], 
                    CASE setCase )
{
    int i;
    char c;

    expPattern[ 8 + 3 ] = '\0';

    if ( pattern == NULL || 0 == strcmp( pattern, "*" ) ) {
        /*
         *  "*": all files
         */
        memset( expPattern, '?', 8 + 3 );
        return;
    }

    /*
     *  Expand the pattern
     */
    i = 0;
    memset( expPattern, ' ', 8 + 3 );

    while ( *pattern != '\0' && i < 8 + 3 ) {
        if ( *pattern == '*' ) {
            if ( i < 8 ) {
                memset( expPattern + i, '?', 8 - i );
                i = 8;
            }
            else {
                memset( expPattern + i, '?', 8 + 3 - i );
                i = 8 + 3;
            }
        }
        else if ( *pattern == '.' ) {
            i = i < 8 ? 8 : 8 + 3;
        }
        else {
            expPattern[ i ] = setCase == UPPER ? toupper( *pattern )
                            : setCase == LOWER ? tolower( *pattern )
                                               : *pattern;
            ++i;
        }
        if ( i == 8 ) {
            while ( pattern[ 1 ] != '\0' && *pattern != '.' ) {
                ++pattern;
            }
        }
        ++pattern;
    }
#if DEBUG
    printf( "expandPattern: expanded pattern=\"%s\"\n", expPattern );
#endif
}


/*
 *  Copy a file to PC
 */
char *getFile( FILE_INFO *info, char *dest, MODE mode, ESCAPE_MODE escape )
{
    int count;
    int i, j, l;
    FILE *out;
    int binary;
    
    /*
     *  Build destination filename
     */
    if ( dest == NULL ) {
        dest = "*";
    }
    l = strlen( dest );
    if ( dest[ l - 1 ] == '*'
      || dest[ l - 1 ] == '/'
      || dest[ l - 1 ] == '\\' )
    {
        /*
         *  Destination ends with directory seperator or "*"
         *  Concatenate with source filename
         */
        static char tmp[ FILENAME_MAX + 13 ];
        strncpy( tmp, dest, FILENAME_MAX );
        dest = tmp;
        dest[ FILENAME_MAX - 1 ] = '\0';
        l = strlen( dest );
        if ( dest[ l - 1 ] == '*' ) {
            --l;
        }
        strcpy( dest + l, info->name );

        /*
         *  Remove invalid characters
         */
        while ( dest[ l ] != '\0' ) {

            if ( NULL != strchr( ":\\/*?[]", dest[ l ] ) ) {
                dest[ l ] = '_';
            }
            ++l;
        }
    }
#if DEBUG
    printf( "Copy %s %s\n", info->name, dest );
#endif
    if ( mode == ASCII ) {
        binary = info->type == TYPE_M;
    }
    else if ( mode == BINARY ) {
        binary = TRUE;
    }
    else {
        binary = info->type != TYPE_C && info->type != TYPE_S;
    }

    out = fopen( dest, binary ? "wb" : "wt" );
    if ( out == NULL ) {
        perror( dest );
        return NULL;
    }

    errno = 0;

    if ( binary ) {
        /*
         *  Transparent copy
         */
        while ( TRUE ) {
            count = readFile( info );
#if DEBUG
            printf( "[count=%d]\n", count );
#endif
            if ( count <= 0 ) {
                break;
            }

            if ( count != (int) fwrite( Block, 1, count, out ) ) {
                /*
                 *  Error
                 */
                break;
            }
        }
    }
    else {
        /*
         *  "Print" to file
         */
        count = printFile( info, ASCII, escape, out );
    }
    fclose( out );

    if ( errno != 0 ) {
        /*
         *  Error
         */
        info->error = IO_ERROR;
        perror( dest );
        return NULL;
    }

    return count == NOT_OK ? NULL : dest;
}


/*
 *  Copy a file to MD-100
 */
FILE_INFO *putFile( char *source, char *dest, MODE mode, ESCAPE_MODE escape,
                    CASE setCase, int type, int protect, int noUpdate )
{
    int free = diskFree();
    long length;
    FILE *in;
    int i, j;
    int mustRead;
    char *p;
    FILE_INFO *info;
    DIR_ENTRY newEntry;
    char ext[ 3 ];
    unsigned char tmp[ 3 ];
    ESCAPE *ep;

    /*
     *  Find base name of source
     */
    i = (int) strlen( source );
    while ( --i > 0 ) {
        if ( source[ i - 1 ] == '\\' || source[ i - 1 ] == '/' ) {
            break;
        }
    }

    /*
     *  Combine source and destination name
     */
    memset( &newEntry, '\0', sizeof( DIR_ENTRY ) );
    expandPattern( dest, newEntry.name, setCase );
    memcpy( ext, newEntry.ext, 3 );

    for ( p = source + i, i = 0; i < 8 + 3 && *p != '\0'; ++i, ++p ) {

        char c = setCase == UPPER ? toupper( *p )
               : setCase == LOWER ? tolower( *p )
               : *p;

        if ( c == '.' ) {
            /*
             *  Start of extension, restore pattern for last three bytes
             */
            memcpy( newEntry.name + 8, ext, 3 );
            i = 7;
            continue;
        }
        if ( newEntry.name[ i ] == '?' ) {
            /*
             *  Replace wildcard with character from source filename
             */
            newEntry.name[ i ] = c;
        }
    }

    /*
     *  Get rid of remaining wildcards
     */
    for ( i = 0; i < 8 + 3; ++i ) {
        if ( newEntry.name[ i ] == '?' ) {
            newEntry.name[ i ] = ' ';
        }
    }

    /*
     *  Set type
     */
    if ( type <= 0 ) {

        if ( 0 == memicmp( newEntry.ext, "c  ", 3 )
          || 0 == memicmp( newEntry.ext, "h  ", 3 )
          || 0 == memicmp( newEntry.ext, "BAT", 3 ) )
        {
            /*
             *  C-File
             */
            type = TYPE_C;
        }
        else if ( 0 == memicmp( newEntry.ext, "EXE", 3 ) ) {
            /*
             *  Binary executable
             */
            type = TYPE_M;
        }
        else if ( 0 == memicmp( newEntry.ext, "BAS", 3 ) ) {
            /*
             *  Tokenized BASIC
             */
            type = TYPE_B;
        }
        else if ( 0 == memicmp( newEntry.ext, "REL", 3 ) ) {
            /*
             *  Relative file
             */
            type = TYPE_R;
        }
        else {
            /*
             *  Default: Sequential file
             */
            type = TYPE_S;
        }
    }
    newEntry.type = type;

    /*
     *  Set protect attribute and unused field
     */
    if ( protect < 0 ) {
        protect = 0;
    }
    newEntry.protect = protect;
    newEntry.unused = DEFAULT_UNUSED;

    /*
     *  Transfer mode
     */
    if ( mode == AUTO ) {

        if ( type == TYPE_M || type == TYPE_R || type == TYPE_B ) {
            mode = BINARY;
        }
        else {
            mode = ASCII;
        }
    }

    /*
     *  Fill file information block with newly created directory entry
     *  This is preliminary to care for early exit (NO_ROOM)
     */
    info = fileInfo( &newEntry, NULL, AS_IS );

#if DEBUG
    printf( "Source: %s, New: %s\n", source, info->name );
#endif

    /*
     *  Open source file
     */
    if ( mode == ASCII &&
         ( strcmp( source, "stdin" ) == 0 ||
           strncmp( source, "stdin.", 6 ) == 0 ) )
    {
        /*
         *  Allow piping for ASCII files
         */
        in = stdin;
        length = 1;
    }
    else {
        in = fopen( source, mode == ASCII ? "rt" : "rb" );
        if ( in == NULL ) {
            perror( source );
            info->error = FILE_NOT_FOUND;
            return info;
        }

        /*
         *  Determine file size for a quick check
         */
        if ( fseek( in, 0, SEEK_END ) != 0 || ( length = ftell( in ) ) < 0 )
        {
            perror( source );
            info->error = IO_ERROR;
            fclose( in );
            return info;
        }
    }

    /*
     *  Find a free directory entry or an entry with the same name
     *  as the new file
     */
    for ( i = 0; i < MAX_DIR_ENTRY; ++i ) {
        /*
         *  Check for entries with same name
         */
        if ( Directory[ i ].type != 0
          && 0 == memcmp( Directory[ i ].name, newEntry.name, 8 + 3 ) )
        {
            /*
             *  File exists
             */
            info = fileInfo( Directory + i, info, AS_IS );
#if DEBUG
            printf( "Found %-12s %-2s\n", info->name, info->typeS );
#endif
            free += info->blocks;

            if ( length / SIZE_BLOCK + 1 > free ) {
                /*
                 *  Not enough free space, do not delete old file!
                 */
                info->error = NO_ROOM;
                if ( in != stdin ) fclose( in );
                return info;
            }

            /*
             *  Remove old file
             */
            delFile( info );
            break;
        }
    }

    if ( i == MAX_DIR_ENTRY ) {
        /*
         *  Find empty directory entry
         */
        for ( i = 0; i < MAX_DIR_ENTRY; ++i ) {
            /*
             *  check for used entries
             */
            if ( Directory[ i ].type == 0 ) {
                info->entry = Directory + i;
                break;
            }
        }
    }

    if ( i == MAX_DIR_ENTRY || length / SIZE_BLOCK + 1 > free ) {
        /*
         *  Directory or disk is full
         */
        info->error = NO_ROOM;
        if ( in != stdin ) fclose( in );
        return info;
    }

    /*
     *  Update the directory
     */
    memcpy( info->entry, &newEntry, sizeof( DIR_ENTRY ) );
    MustUpdate = TRUE;

    /*
     *  Reposition source to start of file
     */
    if ( fseek( in, 0, SEEK_SET ) != 0 ) {
        perror( source );
        info->error = IO_ERROR;
        if ( in != stdin ) fclose( in );
        return info;
    }

    /*
     *  Now copy the data
     */
    errno = 0;
    length = 0;
    clearBlockBuffer();
    mustRead = TRUE;
    do {
        if ( mustRead ) {
            /*
             *  Read a byte from source file
             */
            i = fgetc( in );
            if ( i == EOF && !feof( in ) ) {
                /*
                 *  I/O error
                 */
                perror( source );
                info->error = IO_ERROR;
                if ( in != stdin ) fclose( in );
                return info;
            }
        }

        if ( i != EOF ) {

            if ( mode == ASCII && i == '\n' && mustRead ) {
                /*
                 *  Expand '\n' to CR+LF
                 */
                Block[ length ] = '\r';
                i = '\n';
                mustRead = FALSE;
            }
            else if ( mode == ASCII && i == '\r' ) {
                /*
                 *  Ignore stray CR
                 */
                continue;
            }
            else if ( mode == ASCII && escape != NONE && i == '\\' ) {
                /*
                 *  Look for escape sequence in source
                 */
                for ( j = 0; j < 2; ++j ) {
                    /*
                     *  2 passes for one or two character escapes
                     */
                    i = fgetc( in );
                    if ( i == EOF ) {
                        break;
                    }
                    tmp[ j ] = toupper( i );
                    tmp[ j + 1 ] = '\0';
                    for ( ep = Escapes; ep->text != NULL; ++ep ) {
                        /* printf( "%s:%s\t", tmp, ep->text ); */
                        if ( 0 == strcmp( ep->text, (char *) tmp ) ) {
                            break;
                        }
                    }
                    if ( ep->text != NULL ) {
                        /*
                         *  Symbolic escape sequence found
                         */
                        /* printf( "OK\n" ); */
                        Block[ length ] = ep->token;
                        mustRead = TRUE;
                        break;
                    }
                    else if ( j == 1 ) {
                        /*
                         *  Check for hex escape
                         */
                        i = 0;
                        sscanf( (char *) tmp, "%2X", &i );
                        if ( i <= 0x0F && tmp[ 0 ] != '0' ) {
                            /*
                             *  Illegal or unknown escape
                             *  Copy the bytes after '\'
                             */
                            /* printf( "NOT OK\n" ); */
                            Block[ length ] = tmp[ 0 ];
                            i = tmp[ 1 ];
                            mustRead = FALSE;
                        }
                        else {
                            /*
                             *  Copy value of hex code
                             */
                            /* printf( "OK:HEX\n" ); */
                            Block[ length ] = (unsigned char) i;
                            mustRead = TRUE;
                        }
                    }
                }
                if ( i == EOF && !feof( in ) ) {
                    /*
                     *  I/O error
                     */
                    perror( source );
                    info->error = IO_ERROR;
                    if ( in != stdin ) fclose( in );
                    return info;
                }
            }
            else {
                /*
                 *  Copy the byte to block
                 */
                Block[ length ] = (unsigned char) i;
                mustRead = TRUE;
            }
        }

        /*
         *  Increment size and flush buffer if full or EOF
         */
        if ( i == EOF || ++length == SIZE_BLOCK ) {
#if DEBUG
            printf( "i=%d, length=%ld\n", i, length );
#endif
            if ( length >= 0 /* always! - we must add the EOF marker! */
                 && OK != writeFile( info, (int) length, noUpdate ) )
            {
                /*
                 *  Write error
                 */
                if ( in != stdin ) fclose( in );
                return info;
            }
            length = 0;
            clearBlockBuffer();
        }
    } while ( i != EOF );

    /*
     *  Done
     */
    if ( in != stdin ) fclose( in );
    return info;
}


/*
 *  Delete a file.
 */
void delFile( FILE_INFO *info )
{
    int i = info->next & FB_BLOCK;
    int j;

    while ( i != 0 ) {
        /*
         *  Follow the FAT chain
         */
        j = 2 * ( i & FB_BLOCK );
        i = FAT_Block[ j ] << 8 | FAT_Block[ j + 1 ];
#if DEBUG
        printf( "FAT: %04.4x\n", i );
#endif        
        if ( !( i & FB_IN_USE ) ) {
            /*
             *  Broken chain
             */
            break;
        }
        /*
         *  Mark block as free
         */
        FAT_Block[ j ] = FAT_Block[ j + 1 ] = '\0';

        if ( i & FB_LAST ) {
            /*
             *  end of chain;
             */
            i = 0;
        }
    }

    /*
     *  Delete directory entry
     */
    memset( info->entry, '\0', sizeof( DIR_ENTRY ) );

    info->next = 0;
    MustUpdate = TRUE;
}


/*
 *  Rename a file
 */
FILE_INFO *renFile( FILE_INFO *info, char *newName, CASE setCase )
{
    char newPattern[ 8 + 3 + 1 ];
    static FILE_INFO newInfo;
    int i;
    char c;
    int found;

    /*
     *  New name my contain wildcards
     */
    expandPattern( newName, newPattern, setCase );

    for ( i = 0; i < 8 + 3; ++i ) {
        /*
         *  Update each character in name
         */
        c = newPattern[ i ];
        if ( c == '?' ) {
            c = info->entry->name[ i ];
        }
        newPattern[ i ] = setCase == UPPER ? toupper( c )
                        : setCase == LOWER ? tolower( c )
                        : c;
    }

    /*
     *  Check if name exists
     */
    for ( i = 0, found = FALSE; i < MAX_DIR_ENTRY; ++i ) {
        /*
         *  Find used entry with same name,
         *  but don't stumble over current entry!
         */
        found = Directory + i != info->entry
             && Directory[ i ].type != 0
             && 0 == memcmp( Directory[ i ].name, newPattern, 8 + 3 );
        if ( found ) {
            break;
        }
    }

    if ( found ) {
        /*
         *  Duplicate, return information for existing file
         */
        fileInfo( Directory + i, &newInfo, AS_IS );
        newInfo.error = FILE_EXISTS;
    }
    else {
        /*
         *  Update the name
         */
        memcpy( info->entry->name, newPattern, 8 + 3 );
        fileInfo( info->entry, &newInfo, AS_IS );
        MustUpdate = TRUE;
    }
    return &newInfo;
}


/*
 *  Set file attributes
 */
FILE_INFO *setFile( FILE_INFO *info, int type, int protect )
{
    static FILE_INFO newInfo;

    /*
     *  New name may contain wildcards
     */
    if ( type > 0 ) {
        info->entry->type = (unsigned char) type;
    }
    if ( protect >= 0 ) {
        info->entry->protect = (unsigned char) protect;
    }
    MustUpdate = TRUE;
    return fileInfo( info->entry, &newInfo, AS_IS );
}


/*
 *  Print a file
 */
int printFile( FILE_INFO *info, MODE mode, ESCAPE_MODE escape, FILE *out )
{
    int start, count;
    int i, j, l;
    long addr = 0L;
    unsigned char c;
    int pos = 0;
    int length = 0;
    int line_nr = 0;
    int first = TRUE;

    while ( TRUE ) {
        count = readFile( info );
        start = 0;
#if DEBUG
        printf( "[count=%d]\n", count );
#endif
        if ( count <= 0 ) {
            break;
        }

        switch ( mode == BINARY ? -1 : info->type ) {
        
        case TYPE_S:
        case TYPE_C:
            /*
             *  Text file
             */
            for ( i = 0; i < count; ++i ) {
                c = Block[ i ];
                if ( c == '\r' ) {
                    continue;
                }
                fputc( c, out );
            }
            break;

        case TYPE_R:
            /*
             *  Data file
             */
            for ( i = 0; i < count; i += 256 ) {
                fprintf( out, "%4d: ", ++addr );
                for ( l = 256; l > 0 && Block[ i + l - 1 ] == ' '; --l );
                fwrite( Block + i, l, 1, out );
                fputc( '\n', out );
            }
            break;

        case TYPE_B:
            /*
             *  BASIC file
             */
            if ( first ) {
                /*
                 *  Print header information
                 */
                c = Block[ 17 ];
                if ( c != 0xff ) {
                    /*
                     *  File has a password
                     */
                    fprintf( out, "Password: " );
                    for( i = 17; i < 17 + 8 && Block[ i ] != 0xff; ++i ) {
                        c = Block[ i ] ^ 0xFF;
                        if ( c < ' ' || c >= 0x80 ) {
                            fprintf( out, "\\%02.2X", c );
                        }
                        else {
                            fputc( c, out );
                        }
                    }
                    fputc( '\n', out );
                }
                start = 256;
            }
            for ( i = start; i < count; ++i ) {
                c = Block[ i ];
#if DEBUG_
                printf( "<l=%d,p=%d,c=%02.2X>", length, pos, c );
#endif
                if ( --length == -1 ) {
                    pos = 0;
                }

                switch ( pos++ ) {

                case 0:
                    length = c;
                    continue;

                case 1:
                    line_nr = c;
                    continue;

                case 2:
                    line_nr += c * 256;
                    fprintf( out, "%d ", line_nr );
                    continue;

                case 3:
                    if ( c == ' ' ) {
                        continue;
                    }
                    break;
                }
                printToken( c, escape, out );
            }
            break;

        case TYPE_M:
            if ( first ) {
                /*
                 *  Print header information
                 */
                fprintf( out, "Adresses: %02.2X%02.2X-%02.2X%02.2X,"
                                       " Entry: %02.2X%02.2X\n\n",
                              Block[ 26 ], Block[ 25 ],
                              Block[ 28 ], Block[ 27 ],
                              Block[ 30 ], Block[ 29 ] );
                start = 256;
                addr = Block[ 26 ] * 256 + Block[ 25 ];
            }
            /* no break */
                
        default:
            /*
             *  Binary data
             */
            for ( i = start; i < count; i += 16 ) {
                fprintf( out, mode == BINARY ? "%05.5X: " : "%04.4X: ", addr );
                addr += 16;
                l = count - i < 16 ? count - i : 16;
                for ( j = 0; j < l; ++j ) {
                    fprintf( out, "%02.2X ", Block[ i + j ] );
                }
                while ( j++ < 16 ) {
                    fprintf( out, "   " );
                }
                fputc( ' ', out );
                for ( j = 0; j < l; ++j ) {
                    char c = Block[ i + j ];
                    fputc( c < ' ' || c >= 0x7F ? ' ' : c, out );
                }
                fputc( '\n', out );
            }
            break;
        }
        first = FALSE;
    }
    if ( info->type == TYPE_C || info->type == TYPE_S ) {
        /*
         *  Terminate output with a last linefeed
         */
        if ( c != '\n' ) {
            fputc( '\n', out );
        }
    }
    return count == NOT_OK ? NOT_OK : OK;
}


/*
 *  Print a character from a BASIC program, translate tokens
 */
void printToken( int c, ESCAPE_MODE escape, FILE *out )
{
    static int insert_space = FALSE;
    static int pending_colon = FALSE;
    static int last = 0;
    static int quoted = FALSE;
    static int quoted_until_eol = FALSE;
    char *p = NULL;
    char buffer[] = "\\XX\\XX";
    static int prefix = 0;
    static int lsb = -1;
    ESCAPE *ep;

    if ( prefix == 3 ) {
        /*
         *  Binary line number
         */
        if ( lsb == -1 ) {
            lsb = c;
            c = -1;
        }
        else {
            sprintf( buffer, "%d", lsb + c * 256 );
            p = buffer;
            lsb = -1;
            prefix = 0;
        }
    }
    else {
        /*
         *  Text or token
         */
        switch ( c ) {

        case 0x00:
            p = "\n";
            break;

        case 0x01:
            pending_colon = TRUE;
            c = -1;
            break;

        case 0x02:
            p = "'";
            quoted_until_eol = TRUE;
            break;

        case 0x03:
            prefix = c;
            lsb = -1;
            c = -1;
            break;

        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            prefix = c;
            c = -1;
            break;

        default:
            if ( prefix != 0 ) {
                /*
                 *  Token
                 */
                if ( prefix < 4 || prefix > 7 || c < 0x40 || c > 0xCF ) {
                    p = NULL;
                }
                else {
                    p = Tokens[ prefix - 4 ][ c - 0x40 ];
                }
                if ( p == NULL ) {
                    sprintf( buffer, "\\%02.2X\\%02.2X", prefix,
                                     (unsigned char) c );
                    p = buffer;
                }
            }
            prefix = 0;
        }
    }

    if ( c < 0 ) {
        /*
         *  Character is already handled
         */
    }
    else if ( c == ' ' ) {
        /*
         *  Literal space is always printed
         */
        if ( pending_colon ) {
            /*
             *  The colon had been postponed
             */
            fputc( ':', out );
            pending_colon = FALSE;
        }
        fputc( c, out );
        insert_space = FALSE;
        last = ' ';
    }
    else {
        if ( escape != NONE ) {
            /*
             *  Create escape sequences like \PI
             */
            if ( escape == SYMBOLS ) {
                for ( ep = Escapes; ep != NULL && ep->text != NULL; ++ep ) {
                    if ( c == ep->token ) {
                        sprintf( buffer, "\\%s", ep->text );
                        p = buffer;
                        break;
                    }
                }
            }
            if ( p == NULL && ( c >= 0x80 || c == '\\' ) ) {
                /*
                 *  Hex escape
                 */
                sprintf( buffer, "\\%02.2X", c );
                p = buffer;
            }
        }
        if ( p == NULL ) {
            /*
             *  Save character as string and point to it
             */
            buffer[ 0 ] = (char) c;
            buffer[ 1 ] = '\0';
            p = buffer;
        }

        if ( *p == ' ' && last == ' ' ) {
            /*
             *  Collapse spaces in tokens
             */
            ++p;
        }

        if ( pending_colon ) {
            if ( strncmp( p, "ELSE", 4 ) != 0 ) {
                /*
                 *  Print the postponed colon except before ELSE
                 */
                fputc( ':', out );
            }
            pending_colon = FALSE;
        }

        /*
         *  Print the Text
         */
        while ( ( c = *p++ ) != '\0' ) {
            /*
             *  Print character, insert or drop spaces as appropriate
             */
            switch ( c ) {
            
            case '"':
                quoted = !quoted || quoted_until_eol;
                insert_space = FALSE;
                break;

            case '\n':
                quoted = quoted_until_eol = FALSE;
                insert_space = FALSE;
                break;
            }

            if ( quoted ) {
                /*
                 *  Print char as is, no further translation
                 */
                fputc( c, out );
                insert_space = FALSE;
            }
            else {
                /*
                 *  Check for spaces and unprintable characters
                 */
                if ( c < ' ' && c != '\n' ) {
                    fprintf( out, "\\%02.2X", (unsigned char) c );
                }
                else if ( c == ' ' && *p == '\0' ) {
                    /*
                     *  Token ends with a space
                     */
                    insert_space = TRUE;
                }
                else {
                    /*
                     *  Printable character
                     */
                    if ( insert_space && ( c >= '0' && c <= '9' || c >= 'A' ) ) { 
                        fputc( ' ', out );
                    }
                    fputc( c, out );
                    insert_space = FALSE;
                }
            }
            last = c;
        }
    }
}


/*
 *  Get file information from directory entry
 */
FILE_INFO *fileInfo( DIR_ENTRY *dptr, FILE_INFO *info, CASE setCase )
{
    static FILE_INFO localInfo;
    int i, j, l;
    char c;
    TYPES *tptr;

    if ( dptr == NULL ) {
        return NULL;
    }

    if ( info == NULL ) {
        info = &localInfo;
    }
    memset( info, '\0', sizeof( FILE_INFO ) );

    /*
     *  Link to original entry in directory
     */
    info->entry = dptr;

    /*
     *  Collapse the filename
     */
    for ( l = 8; l > 0; --l ) {
        /*
         *  Find length of basename
         */
        if ( dptr->name[ l - 1 ] != ' ' ) {
            break;
        }
    }

    for ( i = 0, j = 0; i < l; ++i ) {
        /*
         *  Copy basename
         */
        c = dptr->name[ i ];
        info->name[ j++ ] = setCase == LOWER ? tolower( c )
                          : setCase == UPPER ? toupper( c )
                          : c;
    }

    for ( l = 3; l > 0; --l ) {
        /*
         *  Find length of extension
         */
        if ( dptr->ext[ l - 1 ] != ' ' ) {
            break;
        }
    }

    if ( l > 0 ) {
        /*
         *  Append extension
         */
        info->name[ j++ ] = '.';

        for ( i = 0; i < l; ++i ) {
            char c = dptr->ext[ i ];
            info->name[ j++ ] = setCase == LOWER ? tolower( c )
                              : setCase == UPPER ? toupper( c )
                              : c;
        }
    }

    /*
     *  Convert type
     */
    info->type = dptr->type;

    for ( tptr = Types; tptr->type != 0; ++tptr ) {
        if ( info->type == tptr->type ) {
            /*
             *  Known type
             */
            strcpy( info->typeS, tptr->typeName );
            break;
        }
    }

    if ( tptr->type == 0 ) {
        /*
         *  Unknown type
         */
        sprintf( info->typeS, "%02.2X", dptr->type );
    }

    /*
     *  Store protect attribute and first block
     */
    info->protect = dptr->protect;
    info->block = ( dptr->block[ 0 ] << 8 ) | dptr->block[ 1 ];
    info->next = info->block;

    /*
     *  Compute length from FAT
     */
    i = info->block;

    if ( i == 0 ) {
        /*
         *  No block
         */
        info->blocks = 0;
    }
    else {
        /*
         *  Count blocks
         */
        info->blocks = 1;
        l = 0;

        while ( TRUE ) {
            /*
             *  Count length && get next block
             */
            i = FAT_Block[ 2 * i ] << 8 | FAT_Block[ 2 * i + 1 ];

            if ( i & FB_LAST ) {
                /*
                 *  End of chain, add partial block
                 */
                j = ( ( i & FB_SECTORS ) >> 12 ) + 1;
                l += j;
                break;
            }

            /*
             *  Add full block and proceed
             */
            ++info->blocks;
            l += SIZE_BLOCK / SIZE_SECTOR;
            i &= FB_BLOCK;
        }

        /*
         *  Set size
         */
        info->size = (long) l * SIZE_SECTOR;

        /*
         *  Read last block
         */
        if ( OK == readBlocks( Block, i, 1 ) ) {
            /*
             *  search backwards for EOF_CHAR
             */
            j *= SIZE_SECTOR;
            for ( i = j - 1; i >= j - SIZE_SECTOR; --i ) {
                if ( Block[ i ] == EOF_CHAR ) {
                    /*
                     *  Exact size of file
                     */
                    info->size -= j - i;
                    break;
                }
            }
        }
    }
    info->remaining = info->size;

    /*
     *  All info collected
     */
    return info;
}


/*
 *  Find a file in the directory
 */
DIR_ENTRY *findFile( char *pattern, int ignoreCase, int type, int protect )
{
    static char expPattern[ 8 + 3 + 1 ] = "";
    static int position = 0;
    int i;
    char c;
    int count;
    int found = FALSE;
    DIR_ENTRY *dptr;

    if ( pattern != NULL ) {
        /*
         *  expand the pattern DOS like
         */
        expandPattern( pattern, expPattern, ignoreCase ? UPPER : AS_IS );

        /*
         *  initialize the pointer
         */
        position = -1;
    }

    while ( !found && position < MAX_DIR_ENTRY - 1 ) {
        /*
         *  find next entry
         */
        dptr = Directory + ++position;

        if ( dptr->type == 0 ) {
            /*
             *  Entry is unused
             */
            continue;
        }
        if ( type != 0 && dptr->type != (unsigned char) type ) {
            /*
             *  Type not matched
             */
            continue;
        }
        if ( protect >= 0 && dptr->protect != (unsigned char) protect ) {
            /*
             *  Protect attribute not matched
             */
            continue;
        }

        /*
         *  Compare
         */
        for ( i = 0; i < 8 + 3; ++i ) {
            if ( expPattern[ i ] == '?' ) {
                continue;
            }
            c = dptr->name[ i ];
            if ( expPattern[ i ] != ( ignoreCase ? toupper( c ) : c ) ) {
#if DEBUG_
                printf( "%c != %c\n", expPattern[ i ], c );
#endif
                break;
            }
        }
        found = ( i == 8 + 3 );
    }

    /*
     *  Return entry or NULL if no match
     */
    return found ? Directory + position : NULL;
}


/*
 *  Check for "?" and "*"
 */
int isWildcard( char *pattern )
{
    return strchr( pattern, '*' ) != NULL || strchr( pattern, '?' ) != NULL;
}


/*
 *  Read next block from file.
 *  Return number of bytes read, 0 for EOF, NOT_OK (-1) if error
 */
int readFile( FILE_INFO *info )
{
    long count = info->remaining > SIZE_BLOCK ? SIZE_BLOCK : info->remaining;
    int i = info->next & FB_BLOCK;

#if DEBUG
    printf( "[remaining:%ld, next=%04.4X]", info->remaining, info->next );
#endif
    if ( count == 0 || info->next == 0 ) {
        /*
         *  end of file
         */
        return 0;
    }

    if ( OK != readBlocks( Block, i, 1 ) ) {
        return NOT_OK;
    }

    info->remaining -= count;

    if ( info->remaining == 0 ) {
        /*
         *  Done
         */
        info->next = 0;
    }
    else {
        /*
         *  Follow the FAT chain
         */
        info->next = FAT_Block[ 2 * i ] << 8 | FAT_Block[ 2 * i + 1 ];

        if ( !( info->next & FB_IN_USE ) ) {
            /*
             *  Broken chain
             */
            info->next = 0;
        }
    }

    return count;
}


/*
 *  Write next block to file.
 *  Count must be <= SIZE_BLOCK.
 *  If count < SIZE_BLOCK, an EOF marker is added.
 *  Returns OK or NOT_OK (-1) if error.
 */
int writeFile( FILE_INFO *info, int count, int noUpdate )
{
    int i = info->next & FB_BLOCK;

    if ( i == 0 ) {
        /*
         *  Find first free block
         */
        i = findFreeBlock( 0 );
        if ( i == 0 ) {
            /*
             *  Disk full
             */
            info->error = NO_ROOM;
            return NOT_OK;
        }
        info->block = info->next = i;
        info->entry->block[ 0 ] = (unsigned char) ( i >> 8 );
        info->entry->block[ 1 ] = (unsigned char) i;
    }
    if ( count == SIZE_BLOCK ) {
        /*
         *  reserve another block
         */
        int j = findFreeBlock( i + 1 );
        if ( j == 0 ) {
            /*
             *  Disk full
             */
            info->error = NO_ROOM;
            return NOT_OK;
        }
        info->next = FB_IN_USE | j;
    }
    else {
        /*
         *  This is the last block
         */
        Block[ count ] = EOF_CHAR;
        info->next = FB_IN_USE | FB_LAST | count / 256 << 12 | i;
    }
#if DEBUG
    printf( "[current=%04.4X,next=%04.4X]", i, info->next );
#endif

    /*
     *  Mark block in FAT
     */
    FAT_Block[ 2 * i ]     = (unsigned char) ( info->next >> 8 );
    FAT_Block[ 2 * i + 1 ] = (unsigned char) info->next;
    MustUpdate = TRUE;

    /*
     *  Write data
     */
    if ( !noUpdate && OK != writeBlocks( Block, i, 1 ) ) {
        /*
         *  I/O error
         */
        fprintf( stderr, "%s: Cant't write data block\n", info->name );
        info->error = IO_ERROR;
        return NOT_OK;
    }

    /*
     *  Update info
     */
    info->blocks += 1;
    info->size += count;
    return OK;
}


/*
 *  Compute free space
 */
int diskFree( void )
{
    int free = Blocks - START_DATA;
    int i;

    for ( i = START_DATA; i < Blocks; ++i ) {
        /*
         *  check for used blocks
         */
        if ( FAT_Block[ 2 * i ] != 0 ) {
            --free;
        }
    }
    return free;
}


/*
 *  Compute free directory space
 */
int dirFree( void )
{
    int free = MAX_DIR_ENTRY;
    int i;

    for ( i = 0; i < MAX_DIR_ENTRY; ++i ) {
        /*
         *  check for used entries
         */
        if ( Directory[ i ].type != 0 ) {
            --free;
        }
    }
    return free;
}


/*
 *  Find a free block
 */
int findFreeBlock( int start )
{
    int i;

    for ( i = start; i < Blocks; ++i ) {
        int j = FAT_Block[ 2 * i ] << 8 | FAT_Block[ 2 * i + 1 ];
        if ( !( j & FB_IN_USE ) ) {
            /*
             *  Free block found
             */
            return i;
        }
    }
    return 0;
}


/*
 *  Update the FAT and directory blocks
 */
int updateDisk( void )
{
    int result = writeBlocks( FAT_Block, START_FAT, BLOCKS_FAT );
    if ( result == OK ) {
        result = writeBlocks( DirBlocks, START_DIR, BLOCKS_DIR );
    }
    if ( result != OK ) {
        fprintf( stderr, "Can't update directory blocks\n" );
        return NOT_OK;
    }
    return OK;
}


/***************************************************************************
 *
 *  Access to the disk image or physical disk.
 *
 *  On Linux, the standard I/O routines work fine directly on the device!
 *
 **************************************************************************/

/*
 *  External routines, depending on implementation
 *
 *  The source is simply included so that everything is compiled in one go
 */
#if defined( LIBDSK ) || defined( LIBDISK )
/*
 *  Use LibDsk by John Elliot
 */
#include "libdisk.c"

#elif defined(__OS2__)
/*
 *  Access through DosDevIoControl
 */
#include "os2disk.c"

#elif defined(__WIN32__)
/*
 *  Yet to be implemented
 */
// #include "windisk.c"
#define NO_FLOPPY

#elif defined(__MSDOS__)
/*
 *  Direct access to floppy in DOS
 */
#include "dosdisk.c"

#else
/*
 *  No special floppy routines available
 */
#define NO_FLOPPY
#endif

#ifdef NO_FLOPPY
/*
 *  No special floppy routines implemented
 */
int noDirectAccess( void )
{
    errno = EBADF;
#ifdef __linux__
    /*
     *  Just a reminder for Linux users who do not need special routines
     */
    fprintf( stderr, "Use /dev/fd0casio as image name\n"
                     "You'll need fdutils-5.5 to set it up:\n"
                     "    mknod /dev/fd0casio b 2 80\n"
                     "    /usr/local/bin/setfdprm /dev/fd0casio"
                        " SS DD sect=16 ssize=256\n" );
#else
    fprintf( stderr, "Direct disk access not implemented\n" );
#endif
    return NOT_OK;
}

#define openDirect( name, create, noUpdate ) noDirectAccess()
#define closeDirect()                        noDirectAccess()
#define readDirect( dest, number, count )    noDirectAccess()
#define writeDirect( source, number, count ) noDirectAccess()
#endif


/*
 *  Global file handle
 */
FILE *Handle;
int Direct = FALSE;


/*
 *  Open the disk image or device
 */
int openDisk( char *name, int create, int size, int noUpdate )
{
    if ( strlen( name ) == 2 && name[ 1 ] == ':' ) {
        /*
         *  Drive name given
         */
        Direct = TRUE;
        Blocks = DEFAULT_BLOCKS;
        return openDirect( name, create, noUpdate );
    }
    else {
        /*
         *  Standard file access
         */
        char *mode;
        unsigned long image_size;
        
        Direct = FALSE;
        Blocks = size;
        mode = noUpdate ? "rb" : "r+b";

        Handle = fopen( name, mode );

        if ( Handle == NULL && create ) {
            /*
             *  Image not found, create a new file
             */
            Handle = fopen( name, "w+b" );

            if ( Handle != NULL ) {
                /*
                 *  File created, write FAT and empty directory
                 */
                if ( OK != updateDisk() ) {
                    return NOT_OK;
                }
                /*
                 *  Write a dummy block to the end of the image
                 */
                if ( OK != writeBlocks( Block, Blocks - 1, 1 ) ) {
                    return NOT_OK;
                }
            }
        }
        if ( Handle == NULL ) {
            perror( name );
            return NOT_OK;
        }
        /*
         *  Check actual image size
         */
        if ( OK != fseek( Handle, 0, SEEK_END ) ) {
            perror( "fseek" );
            return NOT_OK;
        }
        image_size = ftell( Handle );
        size = (int) ( image_size / SIZE_BLOCK );
        if ( size > MAX_BLOCKS || size < MIN_BLOCKS - 1 ) {
            fprintf( stderr, "Image size out of range\n" );
            return NOT_OK;
        }
        if ( size > Blocks ) {
            Blocks = size;
        }
    }
    return OK;
}


/*
 *  Close the disk image or device
 */
int closeDisk( void )
{
    if ( Direct ) {
        /*
         *  Terminate access to device
         */
        return closeDirect();
    }
    else {
        /*
         *  Close image file or Linux device
         */
        if ( Handle != NULL ) {
            FILE *f = Handle;
            Handle = NULL;

            if ( 0 != fclose( f ) ) {
                perror( "fclose" );
                return NOT_OK;
            }
        }
    }
    return OK;
}


/*
 *  Read blocks into memory
 */
int readBlocks( void *dest, int number, int count )
{
    number &= FB_BLOCK;

#if DEBUG
    if ( count > 1 ) {
        printf( "read blocks %d-%d\n", number, number + count - 1 );
    }
    else {
        printf( "read block %d\n", number );
    }
#endif
    if ( Direct ) {
        /*
         *  Direct disk read
         */
        return readDirect( dest, number, count );
    }
    else {
        /*
         *  Position to block
         */
        if ( OK != fseek( Handle, SIZE_BLOCK * (long) number, SEEK_SET ) ) {
            perror( "fseek" );
            return NOT_OK;
        }

        /*
         *  Read block(s)
         */
        if ( count != (int) fread( dest, SIZE_BLOCK, count, Handle ) ) {
            perror( "fread" );
            return NOT_OK;
        }
    }
    return OK;
}


/*
 *  Write blocks to image
 */
int writeBlocks( void *source, int number, int count )
{
    number &= FB_BLOCK;

#if DEBUG
    printf( "write blocks %d-%d\n", number, number + count - 1 );
#endif
    if ( Direct ) {
        /*
         *  Direct disk write
         */
        return writeDirect( source, number, count );
    }
    else {
        /*
         *  Position to block
         */
        if ( OK != fseek( Handle, SIZE_BLOCK * (long) number, SEEK_SET ) ) {
            perror( "fseek" );
            return NOT_OK;
        }

        /*
         *  Write block(s)
         */
        if ( count != (int) fwrite( source, SIZE_BLOCK, count, Handle ) ) {
            perror( "fwrite" );
            return NOT_OK;
        }
    }
    return OK;
}


/*
 *  Clear the internal block buffer
 */
void clearBlockBuffer( void )
{
    memset( Block, '\0', SIZE_BLOCK );
}

