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
#include <strings.h>
#if !defined(__SUNPRO_CC) || __SUNPRO_CC > 0x420
#include <new>
#endif
#include "config.h"

#include "threadheap.h"
#include "processheap.h"
#include "persistentheap.h"
#include "arch-specific.h"

//#include "pdlmalloc.h"
//#include "dlmalloc.h"
#include "genalloc.h"

#include <itm.h>

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

#define HOARD_MALLOC          pmalloc
#define HOARD_MALLOC_TXN      pmallocTxn
#define HOARD_FREE            pfree
#define HOARD_FREE_TXN        pfreeTxn
#define HOARD_REALLOC         prealloc
#define HOARD_REALLOC_TXN     preallocTxn
#define HOARD_CALLOC          pcalloc
#define HOARD_MEMALIGN        pmemalign
#define HOARD_VALLOC          pvalloc
#define HOARD_GET_USABLE_SIZE pmalloc_usable_size

//extern "C" __attribute__((tm_callable)) void * HOARD_MALLOC (size_t sz);

extern "C" void * HOARD_MALLOC(size_t);
extern "C" void * HOARD_MALLOC_TXN(size_t);
extern "C" void   HOARD_FREE(void *);
extern "C" void   HOARD_FREE_TXN(void *);
//extern "C" void * HOARD_REALLOC(void *, size_t);
//extern "C" void * HOARD_REALLOC_TXN(void *, size_t);
extern "C" void * HOARD_CALLOC(size_t, size_t);
extern "C" void * HOARD_MEMALIGN(size_t, size_t);
extern "C" void * HOARD_VALLOC(size_t);
extern "C" size_t HOARD_GET_USABLE_SIZE(void *);

__attribute__((tm_wrapping(HOARD_MALLOC))) void *HOARD_MALLOC_TXN(size_t);
__attribute__((tm_wrapping(HOARD_FREE))) void HOARD_FREE_TXN(void *);
//__attribute__((tm_wrapping(HOARD_REALLOC))) void *HOARD_REALLOC_TXN(void *, size_t);


static void * malloc_internal (size_t sz)
{
	void                  *addr;
	static persistentHeap *persistentheap = getPersistentAllocator();
	static processHeap    *pHeap = getAllocator(persistentheap);

	if (sz == 0) {
		sz = 1;
	}
	//printf("pmalloc[START]: size = %d\n",  (int) sz);

	if (sz >= SUPERBLOCK_SIZE/2) {
		/* Fall back to the standard persistent allocator. 
		 * Begin a new atomic block to force the compiler to fall through down the TM 
		 * instrumented path. Actually we are already in the transactional path so
		 * this atomic block should execute as a nested transaction.
		 */
		__tm_atomic 
		{
			addr = GENERIC_PMALLOC(sz);
		}
	} else {
		addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
	}
	//printf("pmalloc[DONE]: addr=%p,  size = %d\n",  addr, (int) sz);
	return addr;
}

extern "C" void * HOARD_MALLOC (size_t sz)
{
	return malloc_internal(sz);
	//std::cerr << "Called persistent memory allocator outside of transaction." << std::endl;
	//abort();
}

extern "C" void * HOARD_MALLOC_TXN (size_t sz)
{
	return malloc_internal(sz);
}

extern "C" void * HOARD_CALLOC (size_t nelem, size_t elsize)
{
	static persistentHeap * persistentheap = getPersistentAllocator();
	static processHeap    * pHeap = getAllocator(persistentheap);
	size_t                  sz = nelem * elsize;

	if (sz == 0) {
		sz = 1;
	}
	void * ptr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
	// Zero out the malloc'd block.
	memset (ptr, 0, sz);
	return ptr;
}

static void free_internal (void * ptr)
{
	static persistentHeap * persistentheap = getPersistentAllocator();

	//printf("free: %p [%p - %p)\n",
	//        ptr,
	//        ((uintptr_t) persistentheap->getPersistentSegmentBase()),
	//		((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE));

	/* Find out which heap this block has been allocated from. */
	if ((uintptr_t) ptr >= ((uintptr_t) persistentheap->getPersistentSegmentBase()) &&
	    (uintptr_t) ptr < ((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE))
	{
		persistentheap->free (ptr);
	} else {
		__tm_atomic 
		{
			GENERIC_PFREE (ptr);
		}
	}
}


extern "C" void HOARD_FREE (void* ptr)
{
	free_internal(ptr);
	//std::cerr << "Called persistent memory allocator outside of transaction." << std::endl;
	//abort();
}


extern "C" void HOARD_FREE_TXN (void * ptr)
{
	free_internal(ptr);
}


extern "C" void * HOARD_MEMALIGN (size_t, size_t)
{
	//TODO: Implement memalign
	assert(0);
#if 0  
	static persistentHeap * persistentheap = getPersistentAllocator();
	static processHeap    * pHeap = getAllocator(persistentheap);
	void * addr = pHeap->getHeap(pHeap->getHeapIndex()).memalign (alignment, size);
	return addr;
#endif 
	return NULL;
}


extern "C" void * HOARD_VALLOC (size_t size)
{
	return HOARD_MEMALIGN (hoardGetPageSize(), size);
}

#if 0
static void * realloc_internal (void * ptr, size_t sz)
{
	_ITM_transaction *tx;

	if (ptr == NULL) {
		return malloc_internal (sz);
	}
	if (sz == 0) {
		free_internal (ptr);
	return NULL;
	}

	// If the existing object can hold the new size,
	// just return it.
	size_t objSize = threadHeap::objectSize (ptr);

	if (objSize >= sz) {
		return ptr;
	}

	// Allocate a new block of size sz.
	void * buf = malloc_internal (sz);

	// Copy the contents of the original object
	// up to the size of the new block.

	// FIXME: how to guarantee atomicity of the memcpy below in the presence of failures?
	// The best approach is to use shadow-update.
	size_t minSize = (objSize < sz) ? objSize : sz;
	tx = _ITM_getTransaction();
	_ITM_memcpyRtWt (tx, buf, ptr, minSize);

	// Free the old block.
	free_internal (ptr);

	// Return a pointer to the new one.
	return buf;
}

#endif

__attribute__((tm_callable)) static void * prealloc_internal (void * ptr, size_t sz)
{
	printf("prealloc_internal(%p, %d)\n", ptr, sz);
	//return GENERIC_PREALLOC(ptr, sz);
}

	
extern "C" __attribute__((tm_callable))  void * prealloc (void * ptr, size_t sz)
{
	return prealloc_internal(ptr, sz);
	//std::cerr << "Called persistent memory allocator outside of transaction." << std::endl;
	//abort();
}

#if 0
extern "C" void * HOARD_REALLOC_TXN (void * ptr, size_t sz)
{
	return realloc_internal(ptr, sz);
}
#endif

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


/* Transactional Wrappers for volatile memory allocator -- This should be part of MTM? */

// TODO: Memory allocator should be implemented using commit/undo actions 
// as described by Intel. Implementation should go into alloc_c.c
// For transactions though that don't abort the following is enough.

__attribute__((tm_wrapping(malloc))) void *malloc_txn(size_t sz) 
{
	return malloc(sz);
}

__attribute__((tm_wrapping(realloc))) void *realloc_txn(void *ptr, size_t sz) 
{
	return realloc(ptr, sz);
}

__attribute__((tm_wrapping(free))) void free_txn(void *ptr) 
{
	free(ptr);
}
