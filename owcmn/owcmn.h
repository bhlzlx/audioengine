#ifndef __OW_CMN_H__
#define __OW_CMN_H__

#include <stdint.h>
#include <stdlib.h>

typedef int8_t 		owINT8;
typedef int16_t		owINT16;
typedef int32_t		owINT32;
typedef int64_t 	owINT64;

typedef uint8_t		owUINT8;
typedef uint16_t	owUINT16;
typedef uint32_t	owUINT32;
typedef uint64_t	owUINT64;

typedef float		owFLOAT;
typedef double 		owDOUBLE;
typedef long double owLDOUBLE;

typedef char		owCHAR;
typedef unsigned char\
					owBYTE;
		
typedef void		owVOID;
typedef void*		owVOID_PTR;

typedef int8_t		owBOOL;
const owBOOL		owTRUE = 1;
const owBOOL		owFALSE = 0;

typedef size_t		owSIZE_T;

owVOID strrep(char * _szStr, char * _szKeys, char ch);
//owINT32 stricmp( const char * _szStr1, const char * _szStr2);

#endif // __OW_CMN_H__