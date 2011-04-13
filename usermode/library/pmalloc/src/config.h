///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// The Hoard Multiprocessor Memory Allocator
// www.hoard.org
//
// Author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2001, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef _REENTRANT
#define _REENTRANT		// If defined, generate a multithreaded-capable version.
#endif

#ifndef USER_LOCKS
#if !(defined(sparc) || defined(i386) || defined(__sgi) || defined(ppc))
#define USER_LOCKS 0
#else
#define USER_LOCKS 1		// Use our own user-level locks if they're available for the current architecture.
#endif
#endif

#define HEAP_LOG 0		// If non-zero, keep a log of heap accesses.


///// You should not change anything below here. /////

#if defined(WIN32)

#define _CRTIMP __declspec(dllimport)

#if 0
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *_P) { return (_P); }
#if _MSC_VER >= 1200
inline void __cdecl operator delete(void *, void *) { return; }
#endif
#endif
#endif

#endif

// The number of groups of superblocks we maintain based on what
// fraction of the superblock is empty. NB: This number must be at
// least 2, and is 1 greater than the EMPTY_FRACTION in heap.h.

enum { SUPERBLOCK_FULLNESS_GROUP = 9 };

// A superblock that holds more than one object must hold at least
// this many bytes.
enum { SUPERBLOCK_SIZE = 16384 };


// DO NOT CHANGE THESE.  They require running of maketable to replace
// the values in heap.cpp for the _numBlocks array.

#define HEAP_DEBUG 0		// If non-zero, keeps extra info for sanity checking.
#define HEAP_STATS 0		// If non-zero, maintain blowup statistics.
#define HEAP_FRAG_STATS 0	// If non-zero, maintain fragmentation statistics.


// CACHE_LINE = The number of bytes in a cache line.

#if defined(i386) || defined(WIN32)
#define CACHE_LINE 32
#endif

#ifdef sparc
#define CACHE_LINE 64
#endif

#ifdef __sgi
#define CACHE_LINE 128
#endif

#ifndef CACHE_LINE
// We don't know what the architecture is,
// so go for the gusto.
#define CACHE_LINE 64
#endif

#ifdef __GNUG__
// Use the max operator, an extension to C++ found in GNU C++.
#define MAX(a,b) ((a) >? (b))
#else
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif


#endif // _CONFIG_H_

