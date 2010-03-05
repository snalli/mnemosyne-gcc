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

/*

  Wrappers that re-route everything, including the Win32 API,
  to ordinary ANSI C calls (except for size).

  by Emery Berger

*/

#include <stdlib.h>
#if !defined(__SUNPRO_CC) || __SUNPRO_CC > 0x420
#include <new>
#endif

#if 1
extern "C" void * malloc (size_t);
extern "C" void free (void *);
extern "C" void * calloc (size_t, size_t);
extern "C" void * realloc (void *, size_t);
extern "C" size_t malloc_usable_size (void *);
#endif


void * operator new (size_t size)
{
  return malloc (size);
}

void operator delete (void * ptr)
{
  free (ptr);
}

#if !defined(__SUNPRO_CC) || __SUNPRO_CC > 0x420

void * operator new (size_t size, const std::nothrow_t&) throw() {
  return malloc (size);
} 

void * operator new[] (size_t size) throw(std::bad_alloc)
{
  return malloc (size);
}

void * operator new[] (size_t size, const std::nothrow_t&) throw() {
  return malloc (size);
} 

void operator delete[] (void * ptr) throw()
{
  free (ptr);
}
#endif

#if defined(WIN32)

#include <windows.h>
#include <stdlib.h>

// Replace the CRT debugging allocation routines, just to be on the safe side.
// This is not a complete solution, but should avoid inadvertent mixing of allocations.

#include <stdio.h>


extern "C" void * __cdecl _heap_alloc_base (size_t size)
{
	return malloc (size);
}

void * operator new (unsigned int cb, int, const char *, int)
{
	return ::operator new (cb);
}

void operator delete(void * p, int, const char *, int)
{
  ::operator delete(p);
}

extern "C" void * __cdecl _malloc_dbg (size_t sz, int, const char *, int) {
	return malloc (sz);
}

extern "C" void * __cdecl _malloc_base (size_t sz) {
	return malloc (sz);
}

extern "C" void * __cdecl _calloc_dbg (size_t num, size_t size, int, const char *, int) {
	return calloc (num, size);
}

extern "C" void * __cdecl _calloc_base (size_t num, size_t size) {
	return calloc (num, size);
}

extern "C" void * __cdecl _realloc_dbg (void * ptr, size_t newSize, int, const char *, int) {
	return realloc (ptr, newSize);
}

extern "C" void * __cdecl _realloc_base (void * ptr, size_t newSize) {
	return realloc (ptr, newSize);
}

extern "C" void __cdecl _free_dbg (void * ptr, int) {
	free (ptr);
}

extern "C" void __cdecl _free_base (void * ptr) {
	free (ptr);
}


/* Don't allow expand to work ever. */

extern "C" void * __cdecl _expand (void * ptr, size_t sz) {
  return NULL;
}

extern "C" void * __cdecl _expand_base (void * ptr, size_t sz) {
  return NULL;
}

extern "C" void * __cdecl _expand_dbg (void * ptr, size_t sz, int, const char *, int) {
  return NULL;
}

extern "C" void * __cdecl _nh_malloc (size_t sz, int) {
	return malloc (sz);
}

extern "C" void * __cdecl _nh_malloc_base (size_t sz, int) {
	return malloc (sz);
}

extern "C" void * __cdecl _nh_malloc_dbg (size_t sz, size_t, int, int, const char *, int) {
	return malloc (sz);
}

// We have to replace atexit & _onexit with a version that doesn't use the heap!
// So we make a really large static buffer and hope for the best.

#if 0
typedef void (__cdecl * exitFunction)(void);

enum { MAX_EXIT_FUNCTIONS = 16384 };

static exitFunction exitFunctionBuffer[MAX_EXIT_FUNCTIONS];
static int exitFunctionCount = 0;


extern "C" void __cdecl _exit(int);
extern "C" void __cdecl exit(int);

extern "C" void __cdecl doexit(int code,int quick,int retcaller)
{
	if (quick) {
		_exit(code);
	} else {
		exit(code);
	}
}


extern "C" int __cdecl atexit (void (__cdecl * fn)(void)) {
	if (exitFunctionCount == MAX_EXIT_FUNCTIONS) {
		return 1;
	} else {
		exitFunctionBuffer[exitFunctionCount] = fn;
		exitFunctionCount++;
		return 0;
	}
}


extern "C" _onexit_t __cdecl _onexit(_onexit_t fn)
{
	if (atexit((exitFunction) fn) == 0) {
		return fn;
	} else {
		return NULL;
	}
}

extern "C" void __cdecl _exit(int);
extern "C" int __cdecl _fcloseall (void);

extern "C" void __cdecl exit (int r) {

	// As prescribed, we call all exit functions in LIFO order,
	// flush all open buffers to disk, and then really exit.

	for (int i = exitFunctionCount - 1; i >= 0; i--) {
		(*exitFunctionBuffer[i])();
	}
	_fcloseall();

	_exit(r);
}
#endif

#endif // WIN32
