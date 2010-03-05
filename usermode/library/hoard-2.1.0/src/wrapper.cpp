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
  wrapper.cpp
  ------------------------------------------------------------------------
  Implementations of malloc(), free(), etc. in terms of hoard.
  This lets us link in hoard in place of the stock malloc.
  ------------------------------------------------------------------------
  @(#) $Id: wrapper.cpp,v 1.40 2001/12/10 20:11:29 emery Exp $
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#include <strings.h>
#endif
#if !defined(__SUNPRO_CC) || __SUNPRO_CC > 0x420
#include <new>
#endif
#include "config.h"

#include "threadheap.h"
#include "processheap.h"
#include "persistentheap.h"
#include "arch-specific.h"


//
// Access exactly one instance of the global persistent heap
// (which contains objects allocated in previous incarnations).
// We create this object dynamically to avoid bloating the object code.
//

inline static persistentHeap * getPersistentAllocator (void) {
  static char * buf = (char *) hoardGetMemory (sizeof(persistentHeap));
  static persistentHeap * theAllocator = new (buf) persistentHeap;
  return theAllocator;
}


//
// Access exactly one instance of the process heap
// (which contains the thread heaps).
// We create this object dynamically to avoid bloating the object code.
//

inline static processHeap * getAllocator (persistentHeap *persistentHeap) {
  static char * buf = (char *) hoardGetMemory (sizeof(processHeap));
  static processHeap * theAllocator = new (buf) processHeap(persistentHeap);
  return theAllocator;
}

#define HOARD_MALLOC(x) pmalloc(x)
#define HOARD_FREE(x) pfree(x)
#define HOARD_REALLOC(x,y) prealloc(x,y)
#define HOARD_CALLOC(x,y) pcalloc(x,y)
#define HOARD_MEMALIGN(x,y) pmemalign(x,y)
#define HOARD_VALLOC(x) pvalloc(x)
#define HOARD_GET_USABLE_SIZE(x) pmalloc_usable_size(x)

extern "C" void * HOARD_MALLOC(size_t);
extern "C" void HOARD_FREE(void *);
extern "C" void * HOARD_REALLOC(void *, size_t);
extern "C" void * HOARD_CALLOC(size_t, size_t);
extern "C" void * HOARD_MEMALIGN(size_t, size_t);
extern "C" void * HOARD_VALLOC(size_t);
extern "C" size_t HOARD_GET_USABLE_SIZE(void *);


extern "C" void * HOARD_MALLOC (size_t sz)
{
  static persistentHeap * persistentheap = getPersistentAllocator();
  static processHeap    * pHeap = getAllocator(persistentheap);
  if (sz == 0) {
	  sz = 1;
  }
  void * addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
  return addr;
}

extern "C" void * HOARD_CALLOC (size_t nelem, size_t elsize)
{
  static persistentHeap * persistentheap = getPersistentAllocator();
  static processHeap    * pHeap = getAllocator(persistentheap);
  size_t sz = nelem * elsize;
  if (sz == 0) {
	  sz = 1;
  }
  void * ptr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
  // Zero out the malloc'd block.
  memset (ptr, 0, sz);
  return ptr;
}

extern "C" void HOARD_FREE (void * ptr)
{

  static persistentHeap * persistentheap = getPersistentAllocator();
  persistentheap->free (ptr);
}


extern "C" void * HOARD_MEMALIGN (size_t alignment, size_t size)
{
  static persistentHeap * persistentheap = getPersistentAllocator();
  static processHeap    * pHeap = getAllocator(persistentheap);
  void * addr = pHeap->getHeap(pHeap->getHeapIndex()).memalign (alignment, size);
  return addr;
}


extern "C" void * HOARD_VALLOC (size_t size)
{
  return HOARD_MEMALIGN (hoardGetPageSize(), size);
}


extern "C" void * HOARD_REALLOC (void * ptr, size_t sz)
{
  if (ptr == NULL) {
    return HOARD_MALLOC (sz);
  }
  if (sz == 0) {
    HOARD_FREE (ptr);
    return NULL;
  }

  // If the existing object can hold the new size,
  // just return it.

  size_t objSize = threadHeap::objectSize (ptr);

  if (objSize >= sz) {
    return ptr;
  }

  // Allocate a new block of size sz.

  void * buf = HOARD_MALLOC (sz);

  // Copy the contents of the original object
  // up to the size of the new block.

  size_t minSize = (objSize < sz) ? objSize : sz;
  memcpy (buf, ptr, minSize);

  // Free the old block.

  HOARD_FREE (ptr);

  // Return a pointer to the new one.

  return buf;
}

extern "C" size_t HOARD_GET_USABLE_SIZE (void * ptr)
{
  return threadHeap::objectSize (ptr);
}


#if 0
extern "C" void malloc_stats (void)
{
  TheWrapper.TheAllocator()->stats();
}
#endif
