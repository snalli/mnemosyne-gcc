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
  persistentheap.h
  ------------------------------------------------------------------------
  We use one persistentHeap for the whole program. It acts as a proxy to 
  the actual persistent superblocks stored in non-volatile memory.
  ------------------------------------------------------------------------
  Haris Volos
  ========================================================================
*/

#ifndef _PERSISTENTHEAP_H_
#define _PERSISTENTHEAP_H_

#include "config.h"

#include <stdio.h>

#include "arch-specific.h"
#include "hoardheap.h"
#include "threadheap.h"

#if HEAP_LOG
#include "memstat.h"
#include "log.h"
#endif

enum {PERSISTENTHEAP_HEADER_BASE = 0xa00000000};
enum {PERSISTENTHEAP_BASE = 0xb00000000};
enum {PERSISTENTSUPERBLOCK_NUM = 1024};
enum {PERSISTENTHEAP_SIZE = PERSISTENTSUPERBLOCK_NUM * SUPERBLOCK_SIZE};

class persistentHeap : public hoardHeap {

public:

  // Always grab at least this many superblocks' worth of memory which
  // we parcel out.
  enum { REFILL_NUMBER_OF_SUPERBLOCKS = 16 };

  persistentHeap (void);

  ~persistentHeap (void) {
#if HEAP_STATS
    stats();
#endif
  }

  // Format persistent heap 
  void format (void);

  // Memory deallocation routines.
  void free (void * ptr);

  // Print out statistics information.
  void stats (void);

  // Get a thread heap index.
  inline int getHeapIndex (void);

  // Get the thread heap with index i.
  inline threadHeap& getHeap (int i);

  // Extract a superblock.
  inline superblock * acquire (const int c,
			       hoardHeap * dest);

  // Insert a superblock.
  inline void release (superblock * sb);

  void scavenge();
  persistentSuperblock *acquirePersistentSuperblock(bool isFree, int fullness, int sizeClass);

#if HEAP_LOG
  // Get the log for index i.
  inline Log<MemoryRequest>& getLog (int i);
#endif

#if HEAP_FRAG_STATS
  // Declare that we have allocated an object.
  void setAllocated (int requestedSize,
		     int actualSize);

  // Declare that we have deallocated an object.
  void setDeallocated (int requestedSize,
		       int actualSize);

  // Return the number of wasted bytes at the high-water mark
  // (maxAllocated - maxRequested)
  inline int getFragmentation (void);

  int getMaxAllocated (void) {
    return _maxAllocated;
  }

  int getInUseAtMaxAllocated (void) {
    return _inUseAtMaxAllocated;
  }

  int getMaxRequested (void) {
    return _maxRequested;
  }
  
#endif

private:

  // Hide the lock & unlock methods.

  inline void lock (void) {
    hoardHeap::lock();
  }

  inline void unlock (void) {
    hoardHeap::unlock();
  }

  // Prevent copying and assignment.
  persistentHeap (const persistentHeap&);
  const persistentHeap& operator= (const persistentHeap&);

  // The per-thread heaps.
  threadHeap theap[MAX_HEAPS];

#if HEAP_FRAG_STATS
  // Statistics required to compute fragmentation.  We cannot
  // unintrusively keep track of these on a multiprocessor, because
  // this would become a bottleneck.

  int _currentAllocated;
  int _currentRequested;
  int _maxAllocated;
  int _maxRequested;
  int _inUseAtMaxAllocated;
  int _fragmentation;

  // A lock to protect these statistics.
  hoardLockType _statsLock;
#endif

#if HEAP_LOG
  Log<MemoryRequest> _log[MAX_HEAPS + 1];
#endif

  // A lock for the superblock buffer.
  hoardLockType _bufferLock;

  char * 	_buffer;
  int 		_bufferCount;
};


threadHeap& persistentHeap::getHeap (int i)
{
  assert (i >= 0);
  assert (i < MAX_HEAPS);
  return theap[i];
}


#if HEAP_LOG
Log<MemoryRequest>& persistentHeap::getLog (int i)
{
  assert (i >= 0);
  assert (i < MAX_HEAPS + 1);
  return _log[i];
}
#endif


// Hash out the thread id to a heap and return an index to that heap.
int persistentHeap::getHeapIndex (void) {
//  int tid = hoardGetThreadID() & hoardHeap::_numProcessorsMask;
  int tid = hoardGetThreadID() & MAX_HEAPS_MASK;
  assert (tid < MAX_HEAPS);
  return tid;
}


superblock * persistentHeap::acquire (const int sizeclass,
				   hoardHeap * dest)
{
  lock ();

  // Remove the superblock with the most free space.
  superblock * maxSb = removeMaxSuperblock (sizeclass);
  if (maxSb) {
    maxSb->setOwner (dest);
  }

  unlock ();

  return maxSb;
}



// Put a superblock back into our list of superblocks.
void persistentHeap::release (superblock *)
{
//FIXME
#if 0
  assert (EMPTY_FRACTION * sb->getNumAvailable() > sb->getNumBlocks());

  lock();

  // Insert the superblock.
  insertSuperblock (sb->getBlockSizeClass(), sb, this);

  unlock();
#endif
}


#endif // _PERSISTENTHEAP_H_

