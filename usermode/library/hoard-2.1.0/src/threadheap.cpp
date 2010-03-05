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
//                 may call sbrk() (via makeSuperblock).

void * threadHeap::malloc (const size_t size)
{
  void *allocRegionPtr; 
  const int sizeclass = sizeClass (size);
  block * b = NULL;

  lock();

  // Look for a free block.
  // We usually have memory locally so we first look for space in the
  // superblock list.

  superblock * sb = findAvailableSuperblock (sizeclass, b, _persistentHeap);

  std::cout << "findAvailableSuperblock: " << sb << std::endl;
  if (sb == NULL) {
    
    // We don't have memory locally.
    // Try to get more from the persistent heap.
    
    assert (_persistentHeap);
    sb = _persistentHeap->acquire ((int) sizeclass, this);
    // If we didn't get any memory from the process heap,
    // we'll have to allocate our own superblock.
    if (sb == NULL) {
      sb = superblock::makeSuperblock (sizeclass, _pHeap, _persistentHeap);
      if (sb == NULL) {
	// We're out of memory!
	unlock ();
	return NULL;
      }
#if HEAP_LOG
      // Record the memory allocation.
      MemoryRequest m;
      m.allocate ((int) sb->getNumBlocks() * (int) sizeFromClass(sb->getBlockSizeClass()));
      _persistentHeap->getLog(getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
      _persistentHeap->setAllocated (0, sb->getNumBlocks() * sizeFromClass(sb->getBlockSizeClass()));
#endif
    }
    
    // Get a block from the superblock.
    b = sb->acquireBlock ();
    assert (b != NULL);
    std::cout << "-sb: " << sb << std::endl;
    // Insert the superblock into our list.
    insertSuperblock (sizeclass, sb, _persistentHeap);
  }

  assert (b != NULL);
  assert (b->isValid());
  assert (sb->isValid());

  b->markAllocated();

  std::cout <<  std::dec << "block_id: " << b->getId() << std::endl;
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
