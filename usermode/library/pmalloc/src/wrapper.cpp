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

#include "genalloc.h"

#include <itm.h>
#include "mtm/include/txlock.h"
#include <pm_instr.h>

m_txmutex_t generic_pmalloc_txmutex = PTHREAD_MUTEX_INITIALIZER;

//
// Access exactly one instance of the global persistent heap
// (which contains objects allocated in previous incarnations).
// We create this object dynamically to avoid bloating the object code.
//

__TM_CALLABLE__
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

__TM_CALLABLE__
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
extern "C" void * HOARD_REALLOC(void *, size_t);
extern "C" void * HOARD_REALLOC_TXN(void *, size_t);
extern "C" void * HOARD_CALLOC(size_t, size_t);
extern "C" void * HOARD_MEMALIGN(size_t, size_t);
extern "C" void * HOARD_VALLOC(size_t);
extern "C" size_t HOARD_GET_USABLE_SIZE(void *);

__attribute__((tm_wrapping(HOARD_MALLOC))) void *HOARD_MALLOC_TXN(size_t);
__attribute__((tm_wrapping(HOARD_FREE))) void HOARD_FREE_TXN(void *);
__attribute__((tm_wrapping(HOARD_REALLOC))) void *HOARD_REALLOC_TXN(void *, size_t);


__TM_CALLABLE__
static void * pmalloc_internal (size_t sz)
{
	void                  *addr;
	static persistentHeap *persistentheap = getPersistentAllocator();
	static processHeap    *pHeap = getAllocator(persistentheap);

	__m_debug()
        __m_print("%lf %d %s %d sz=%d\n",__m_time__,__m_tid__, __func__, __LINE__, sz);

	if (sz == 0) {
		sz = 1;
	}
	//printf("pmalloc[START]: size = %d\n",  (int) sz);

	if (sz >= SUPERBLOCK_SIZE/2) {
		//printf("pmalloc.vista[START]: size = %d\n",  (int) sz);
		/* Fall back to the standard persistent allocator. 
		 * Begin a new atomic block to force the compiler to fall through down the TM 
		 * instrumented path. Actually we are already in the transactional path so
		 * this atomic block should execute as a nested transaction.
		 */
		__transaction_atomic 
		{
			/* 
			 * FIXME: We don't need the lock as we execute with isolation on
			 * We should make the library work properly when transactions don't provide
			 * isolation. A way to do it would be to use TXlocks:
			 *
			 *   m_txmutex_lock(&generic_pmalloc_txmutex); 
			 *   addr = GENERIC_PMALLOC(sz);
			 *   m_txmutex_unlock(&generic_pmalloc_txmutex);
			 *
			 */
		        __m_print("%lf %d %s %d sz=%d (generic malloc)\n",__m_time__,__m_tid__, __func__, __LINE__, sz);

			addr = GENERIC_PMALLOC(sz);
		}
	} else {
		//printf("pmalloc.hoard[START]: size = %d\n",  (int) sz);
		        __m_print("%lf %d %s %d sz=%d (pmalloc)\n",__m_time__,__m_tid__, __func__, __LINE__, sz);
		addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
	}
	//printf("pmalloc[DONE]: addr=%p,  size = %d\n",  addr, (int) sz);
	return addr;
}

extern "C" void * HOARD_MALLOC (size_t sz)
{
	// FIXME: Change the API to ensure atomicity of the allocation as described 
	// in the design document 
	return pmalloc_internal(sz);
}

extern "C" void * HOARD_MALLOC_TXN (size_t sz)
{
        __m_print("%lf %d %s %d sz=%d ()\n",__m_time__,__m_tid__, __func__, __LINE__, sz);
	TM_BEGIN();
		return pmalloc_internal(sz);
	TM_END();
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
	PM_MEMSET(ptr, 0, sz);
	return ptr;
}

static
void _ITM_CALL_CONVENTION free_commit_action(void *arg)
{
	void* ptr = arg;
	static persistentHeap * persistentheap = getPersistentAllocator();

    persistentheap->free (ptr);
	return;
}



static void free_internal (void * ptr)
{
	static persistentHeap * persistentheap = getPersistentAllocator();

	__m_debug();
	//printf("free: %p [%p - %p)\n",
	//        ptr,
	//        ((uintptr_t) persistentheap->getPersistentSegmentBase()),
	//		((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE));

	/* Find out which heap this block has been allocated from. */
	if ((uintptr_t) ptr >= ((uintptr_t) persistentheap->getPersistentSegmentBase()) &&
	    (uintptr_t) ptr < ((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE))
	{
		__m_print("%lf %d %s %d ptr=%p (pfree)\n",__m_time__,__m_tid__, __func__, __LINE__, ptr)

	    _ITM_transaction * td;
    	td = _ITM_getTransaction();
	    if (_ITM_inTransaction(td) > 0) {
		    _ITM_addUserCommitAction (td, free_commit_action, 2, ptr);
        } else {
			persistentheap->free (ptr);
        }
	} else {
		__transaction_atomic 
		{
			__m_print("%lf %d %s %d ptr=%p (generic free)\n",__m_time__,__m_tid__, __func__, __LINE__, ptr)
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
	__m_print("%lf %d %s %d ptr=%p\n",__m_time__,__m_tid__, __func__, __LINE__, ptr)
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


static void * prealloc_internal (void * ptr, size_t sz)
{
	_ITM_transaction *tx;
	static persistentHeap * persistentheap = getPersistentAllocator();

	if (ptr == NULL) {
		return pmalloc_internal (sz);
	}
	if (sz == 0) {
		free_internal (ptr);
	return NULL;
	}

	size_t objSize;
	/* If object allocated from the Hoard heap then check whether there
	 * the existing object can hold the new size
	 */
	if ((uintptr_t) ptr >= ((uintptr_t) persistentheap->getPersistentSegmentBase()) &&
	    (uintptr_t) ptr < ((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE))
	{

		// If the existing object can hold the new size,
		// just return it.
		objSize = persistentHeap::objectSize (ptr);

		if (objSize >= sz) {
			return ptr;
		}
	} else {
		objSize = GENERIC_OBJSIZE(ptr);
	}

	// Allocate a new block of size sz.
	void * buf = pmalloc_internal (sz);

	// Copy the contents of the original object
	// up to the size of the new block.

	size_t minSize = (objSize < sz) ? objSize : sz;
	tx = _ITM_getTransaction();
	_ITM_memcpyRtWt (tx, buf, ptr, minSize);

	// Free the old block.
	free_internal (ptr);

	// Return a pointer to the new one.
	return buf;
}

	
extern "C" void * HOARD_REALLOC (void * ptr, size_t sz)
{
	return prealloc_internal(ptr, sz);
}


extern "C" void * HOARD_REALLOC_TXN (void * ptr, size_t sz)
{
	return prealloc_internal(ptr, sz);
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



/* Transactional Wrappers for volatile memory allocator -- This should be part of MTM? */

/* 
 * TODO: Dynamic Memory allocator should be implemented using commit/undo 
 * actions as described by Intel. Implementation should go into MTM
 */
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
