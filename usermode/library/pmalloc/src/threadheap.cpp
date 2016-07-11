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

#include <iostream>
#include <limits.h>
#include <string.h>

#include "config.h"

#include "hoardheap.h"
#include "threadheap.h"
#include "persistentheap.h"


threadHeap::threadHeap (void)
  : _pHeap (0), _persistentHeap(0)
{}


// malloc (sz):
//   inputs: the size of the object to be allocated.
//   returns: a pointer to an object of the appropriate size.
//   side effects: allocates a block from a superblock;

__TM_CALLABLE__
void * threadHeap::malloc (const size_t size)
{
	void      *allocRegionPtr; 
	const int sizeclass = sizeClass (size);
	block     *b = NULL;

	lock(); // Why do you need this lock when the heap is private ?

	__m_debug();
	// Look for a free block.
	// We usually have memory locally so we first look for space in the
	// superblock list.

	superblock * sb = findAvailableSuperblock (sizeclass, b, _persistentHeap);
	//std::cout << "threadHeap::malloc[1]: size = " << size << ", sb =  " << sb;
	//if (sb) { std::cout << ", owner = " << sb->getOwner();	}	
	//std::cout << ", this = " << this  << std::endl;

	if (sb == NULL) {
		// We don't have memory locally.
		// Try to get more from the persistent heap.
    
		assert (_persistentHeap);
		sb = _persistentHeap->acquire ((int) sizeclass, this);
		//std::cout << "threadHeap::malloc[2]: sb =  " << sb << ", owner = ";
		//if (sb) { std::cout << ", owner = " << sb->getOwner(); }
		//std::cout << ", this = " << this << std::endl;

		if (sb == NULL) {
			// We're out of memory!
			unlock ();
			assert(0 && "We're out of memory!\n");
			return NULL;
		}
    
		// Get a block from the superblock.
		b = sb->acquireBlock ();
		assert (b != NULL);
    	// Insert the superblock into our list.
		insertSuperblock (sizeclass, sb, _persistentHeap);
	}

	assert (b != NULL);
	assert (b->isValid());
	assert (sb->isValid());

	b->markAllocated();

	allocRegionPtr = (void *) (b->getSuperblock()->getBlockRegion(b->getId()));
#if HEAP_LOG
	MemoryRequest m;
	m.malloc (allocRegionPtr, align(size));
	_pHeap->getLog(getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
	b->setRequestedSize (align(size));
	_pHeap->setAllocated (align(size), 0);
#endif

	unlock();
  
	// Skip past the block header and return the pointer.
	return allocRegionPtr;
}
