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

  Wrappers that re-route everything, to ordinary ANSI C calls (except for size).

  by Emery Berger

*/

//TODO: 
#error "What C++ keywords to wrap for persistent allocator?"

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
