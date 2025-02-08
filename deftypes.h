/*
 *	Name:		General Use Types Definitions -- Header Include file
 *	Version:	1.24
 *	Author:		Vladimir Arnost (QA-Software)
 *	Last revision:	Sep-4-1995
 *	Compiler:	Borland C++ 3.1, Watcom C/C++ 10.0
 *
 */

#ifndef __DEFTYPES_H_
#define __DEFTYPES_H_

#if defined(_Windows) && !defined(__WINDOWS__)
  #define __WINDOWS__
#endif
#if defined(__WINDOWS_H) || defined(_INC_WINDOWS)
  #define __WINDOWS_H_INCLUDED__
#endif

/* Global type declarations */

/* machine independent types */
#ifdef __386__
 #if !defined(__NOBYTE__) && !defined(__WINDOWS_H_INCLUDED__) /* avoid multiple declarations */
  typedef unsigned char  BYTE;
  typedef unsigned short WORD;
  typedef unsigned int   DWORD;
 #endif

 #define __FAR
#else
 #if !defined(__NOBYTE__) && !defined(__WINDOWS_H_INCLUDED__)
  typedef unsigned char  BYTE;
  typedef unsigned int   WORD;
  typedef unsigned long  DWORD;
 #endif

 #define __FAR	far
#endif

/* machine dependent types */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int  uint;
typedef unsigned long ulong;

typedef signed char schar;
typedef signed short sshort;
typedef signed int  sint;
typedef signed long slong;

#if __BORLANDC__ >= 0x300    // BC 3.1: if you want to compile in 386 mode,
#define FASTCALL _fastcall   // undef this #define
#else
#define FASTCALL
#endif

#if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__) || defined(__FLAT__)
  #define NEARDATAPTR
#else
  #define FARDATAPTR
#endif

#if defined(__TINY__) || defined(__SMALL__) || defined(__COMPACT__) || defined(__FLAT__)
  #define NEARCODEPTR
#else
  #define FARCODEPTR
#endif

#ifndef NULL
  #ifdef NEARDATAPTR
    #define NULL 0
  #else
    #define NULL 0L
  #endif
#endif

#endif // __DEFTYPES_H_
