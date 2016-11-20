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

#include <mtm_i.h>
#include <itm.h>
#include <txlock.h>
#include <pm_instr.h>

m_txmutex_t generic_pmalloc_txmutex = PTHREAD_MUTEX_INITIALIZER;

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


extern "C"
void * mtm_pmalloc (size_t sz)
{
	void                  *addr;
	static persistentHeap *persistentheap = getPersistentAllocator();
	static processHeap    *pHeap = getAllocator(persistentheap);

	if (sz == 0) {
		sz = 1;
	}

        if (sz >= SUPERBLOCK_SIZE/2)
        	addr = GENERIC_PMALLOC(sz);
	else
		addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);

	return addr;
}

extern "C"
void * mtm_pcalloc (size_t nelem, size_t elsize)
{
	void                  *addr;
	static persistentHeap * persistentheap = getPersistentAllocator();
	static processHeap    * pHeap = getAllocator(persistentheap);
	size_t                  sz = nelem * elsize;

	if (sz == 0) {
		sz = 1;
	}
        if (sz >= SUPERBLOCK_SIZE/2)
        	addr = GENERIC_PMALLOC(sz);
	else
		addr = pHeap->getHeap(pHeap->getHeapIndex()).malloc (sz);
	PM_MEMSET(addr, 0, sz);
	return addr;
}


extern "C"
void mtm_pfree (void* ptr)
{
	static persistentHeap * persistentheap = getPersistentAllocator();

	if ((uintptr_t) ptr >= ((uintptr_t) persistentheap->getPersistentSegmentBase()) &&
	    (uintptr_t) ptr < ((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE))
	{
		persistentheap->free (ptr);
        } else {
		GENERIC_PFREE (ptr);
	}
}

extern "C"
size_t mtm_get_obj_size(void *ptr)
{
	static persistentHeap * persistentheap = getPersistentAllocator();
	size_t objSize = -1;
	if ((uintptr_t) ptr >= ((uintptr_t) persistentheap->getPersistentSegmentBase()) &&
	    (uintptr_t) ptr < ((uintptr_t) persistentheap->getPersistentSegmentBase() + PERSISTENTHEAP_SIZE))
	{
		objSize = persistentHeap::objectSize (ptr);
	} else {
		objSize = GENERIC_OBJSIZE(ptr);
	}
	
	return objSize;
}

extern "C" void * mtm_prealloc (void * ptr, size_t sz)
{
	_ITM_transaction *tx;
	static persistentHeap * persistentheap = getPersistentAllocator();

	if (ptr == NULL) {
		return mtm_pmalloc (sz);
	}
	if (sz == 0) {
		mtm_pfree (ptr);
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
	void * buf = mtm_pmalloc (sz);

	// Copy the contents of the original object
	// up to the size of the new block.

	size_t minSize = (objSize < sz) ? objSize : sz;
	tx = _ITM_getTransaction();
	_ITM_memcpyRtWt (buf, ptr, minSize);

	// Free the old block.
	mtm_pfree (ptr);

	// Return a pointer to the new one.
	return buf;
}
