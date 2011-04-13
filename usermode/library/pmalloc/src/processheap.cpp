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

#include "config.h"

#include "threadheap.h"

#include "processheap.h"

class persistentHeap;

processHeap::processHeap (persistentHeap *persistentHeap)
  : _buffer (NULL),
    _bufferCount (0)
#if HEAP_FRAG_STATS
  , _currentAllocated (0),
  _currentRequested (0),
  _maxAllocated (0),
  _inUseAtMaxAllocated (0),
  _maxRequested (0)
#endif
{
  int i;
  // The process heap is heap 0.
  setIndex (0);
  for (i = 0; i < MAX_HEAPS; i++) {
    // Set every thread's process heap to this one.
    theap[i].setpHeap (this);
    theap[i].setPersistentHeap (persistentHeap);
    // Set every thread heap's index.
    theap[i].setIndex (i + 1);
  }
#if HEAP_LOG
  for (i = 0; i < MAX_HEAPS + 1; i++) {
    char fname[255];
    sprintf (fname, "log%d", i);
    unlink (fname);
    _log[i].open (fname);
  }
#endif
#if HEAP_FRAG_STATS
  hoardLockInit (_statsLock);
#endif
  hoardLockInit (_bufferLock);
}


// Print out statistics information.
void processHeap::stats (void) {
#if HEAP_STATS
  int umax = 0;
  int amax = 0;
  for (int j = 0; j < MAX_HEAPS; j++) {
    for (int i = 0; i < SIZE_CLASSES; i++) {
      amax += theap[j].maxAllocated(i) * sizeFromClass (i);
      umax += theap[j].maxInUse(i) * sizeFromClass (i);
    }
  }
  printf ("Amax <= %d, Umax <= %d\n", amax, umax);
#if HEAP_FRAG_STATS
  amax = getMaxAllocated();
  umax = getMaxRequested();
  printf ("Maximum allocated = %d\nMaximum in use = %d\nIn use at max allocated = %d\n", amax, umax, getInUseAtMaxAllocated());
  printf ("Still in use = %d\n", _currentRequested);
  printf ("Fragmentation (3) = %f\n", (float) amax / (float) getInUseAtMaxAllocated());
  printf ("Fragmentation (4) = %f\n", (float) amax / (float) umax);
#endif

#endif // HEAP_STATS  
#if HEAP_LOG
  printf ("closing logs.\n");
  fflush (stdout);
  for (int i = 0; i < MAX_HEAPS + 1; i++) {
    _log[i].close();
  }
#endif
}



#if HEAP_FRAG_STATS
void processHeap::setAllocated (int requestedSize,
				int actualSize)
{
  hoardLock (_statsLock);
  _currentRequested += requestedSize;
  _currentAllocated += actualSize;
  if (_currentRequested > _maxRequested) {
    _maxRequested = _currentRequested;
  }
  if (_currentAllocated > _maxAllocated) {
    _maxAllocated = _currentAllocated;
    _inUseAtMaxAllocated = _currentRequested;
  }
  hoardUnlock (_statsLock);
}


void processHeap::setDeallocated (int requestedSize,
				  int actualSize)
{
	hoardLock (_statsLock);
	_currentRequested -= requestedSize;
	_currentAllocated -= actualSize;
	hoardUnlock (_statsLock);
}
#endif


// free (ptr, pheap):
//   inputs: a pointer to an object allocated by malloc().
//   side effects: returns the block to the object's superblock;
//                 updates the thread heap's statistics;
//                 may release the superblock to the process heap.

void processHeap::free (void*)
{
	// TODO: For now the ProcessHeap does not play any role since all 
	// superblocks are acquired from and released to the Persistent Heap
	// which plays the role of a Process Heap as well. Decoupling the 
	// process heap from the persistent heap could allow more flexibility
	// in the policies. We haven't implemented this yet but since processheap
	// is kept around, we make sure that there is no call ever here.
	assert(0);
}
