/*********************************************************
*                                                        *
*       sharp.h                                          *
*                                                        *
*       Header-Datei fÅr sharp.c                         *
*                                                        *
*       03-Dec-1988     Norbert Unterberg                *
*                                                        *
**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>

#ifdef __TURBOC__

#include <dir.h>

#else

#define findfirst(name, blk, attr)  _dos_findfirst (name, attr, blk)
#define findnext(blk)               _dos_findnext (blk)
#define ffblk                       find_t
#define ff_name                     name
#define ff_attrib                   attrib
#define FA_RDONLY                   _A_RDONLY
#define FA_DIREC                    _A_SUBDIR
#define FA_ARCH                     _A_ARCH

#endif

/* Funktionsprototypen */

void   listprogramm (char *dateiname, byte *prog, unsigned int len);
void   holezeile (FILE *fp, char eingabe[], int max);
int    janein (void);
byte   convert (byte b);
int    compare (register byte *s, register byte *d);
int    tokenisiere (register byte *z, tHEADER *header);
int    ladeprogramm (FILE *fd, tHEADER *header);
void   pause (void);
char * strlower (char *s);
int    main (void);
