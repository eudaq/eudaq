/*************************************************************
File      : /dd/sdev_src/c/work/common/include/types.typ
Goal      : Common basic types definitions.
          : I never use default C types names ( char, int ... )
          : i redefined them here ( SInt8, UInt32 ... ).
          :
          : If something go wong on you'r plateform with C types
          : definition you need to update this file.
Prj date  : 2000 - 2002
File date : 
Doc date  : 23/11/2002
Author    : Gilles CLAUS
E-mail    : claus@lepsi.in2p3.fr
----------------------------------------------------------------------------------
License   : You are free to use this source files for your own development as long
          : as it stays in a public research context. You are not allowed to use it
          : for commercial purpose. You must put this header with laboratory and
          : authors names in all development based on this library.
----------------------------------------------------------------------------------
Labo      : LEPSI */
/*************************************************************/


/*H****************************************************************************
FILE   : Types.Typ
BUT    : Define types.
AUTEUR : G.CLAUS
****************************************************************************H*/

#ifndef TYPES_TYP
#define TYPES_TYP

  // typedef enum ELang { ELang_FRENCH, ELang_ENGLISH, ELang_GERMAN, ELang_LANG_NB } TLang;
  
#ifndef NODEFTYPCHAR

/*
17/11/04 GC

BCPPB declares an identfier with the same name " Char "
this problem happens while compiling Mimo* JTAG application (MP).
This Char identifier is used in SpinEdit object.

MP used conditionnal compilation, i decided to remove this Char
type definition, because i believe i never use it.
Wait and see ...

*/

/*   typedef unsigned char Char; */

#endif

#ifndef UInt8
  typedef unsigned char UInt8;
#endif

#ifndef UByte
  typedef UInt8 UByte;
#endif

#ifndef SInt8
  typedef char SInt8;
#endif

#ifndef SByte
  typedef SInt8 SByte;
#endif

#ifndef UInt16
  typedef unsigned 	short UInt16;
#endif

#ifndef UWord
  typedef UInt16 UWord;
#endif

#ifndef SInt16
  typedef short SInt16;
#endif

#ifndef SWord
  typedef SInt16 SWord;
#endif

#ifndef UInt32
  typedef unsigned long int UInt32;
#endif

#ifndef ULong
  typedef UInt32 ULong;
#endif

#ifndef SInt32
  typedef long int SInt32;
#endif
  
#ifndef SLong
  typedef SInt32 SLong;
#endif


#ifndef UInt64
  typedef unsigned __int64 UInt64;
#endif


#ifndef SInt64
  typedef __int64 SInt64;
#endif

/* ROOT !
typedef single Real32;
typedef double Real64;
typedef extended Real80;
*/


/* Pointeurs */

typedef char* TPChar;

typedef UInt8* TPUInt8;
typedef TPUInt8 TPUByte;

typedef SInt8* TPSInt8;
typedef TPSInt8 TPSByte;

typedef UInt16* TPUInt16;
typedef TPUInt16 TPUWord;

typedef SInt16* TPSInt16;
typedef TPSInt16 TPSWord;

typedef UInt32* TPUInt32;
typedef TPUInt32 TPULong;

typedef SInt32* TPSInt32;
typedef TPSInt32 TPSLong;

/* ROOT !
typedef Real32* TPReal32;
typedef Real64* TPReal64;
typedef Real80* TPReal80;
*/

  

#endif
