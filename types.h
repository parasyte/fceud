/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2001 Aaron Oneal
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FCEU_TYPES
#define __FCEU_TYPES

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;

#ifdef __GNUC__
 typedef unsigned long long uint64;
 typedef long long int64;
 #define INLINE inline
 #define GINLINE inline
#elif MSVC
 typedef __int64 int64;
 typedef unsigned __int64 uint64;
 #define INLINE __inline
 #define GINLINE			/* Can't declare a function INLINE
					   and global in MSVC.  Bummer.
					*/
 #define PSS_STYLE 2			/* Does MSVC compile for anything
					   other than Windows/DOS targets?
					*/
#endif
typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
#define byte uint8
#define word uint16

#ifdef __GNUC__
 #ifdef C80x86
  #define FASTAPASS(x) __attribute__((regparm(x)))
  #define FP_FASTAPASS FASTAPASS
 #else
  #define FASTAPASS(x)	
  #define FP_FASTAPASS(x)	
 #endif
#else
 #define FP_FASTAPASS(x)
 #define FASTAPASS(x) __fastcall
#endif

typedef void (FP_FASTAPASS(2) *writefunc)(uint32 A, uint8 V);
typedef uint8 (FP_FASTAPASS(1) *readfunc)(uint32 A);
#endif
