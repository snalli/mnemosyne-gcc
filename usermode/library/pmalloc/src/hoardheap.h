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
  heap.h
  ------------------------------------------------------------------------
  hoardHeap, the base class for threadHeap and processHeap.
  ------------------------------------------------------------------------
  @(#) $Id: hoardheap.h,v 1.79 2001/12/20 15:36:18 emery Exp $
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/


#ifndef _HEAP_H_
#define _HEAP_H_

#include "config.h"

#include "arch-specific.h"
#include "superblock.h"
#include "heapstats.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>

class persistentHeap; // forward declaration

class hoardHeap {
public:

	hoardHeap (void);

	// A thread heap must be at least 1/EMPTY_FRACTION empty before we
	// start returning superblocks to the process heap.
	// enum { EMPTY_FRACTION = SUPERBLOCK_FULLNESS_GROUP - 1 };
	enum { EMPTY_FRACTION = 1 };

	// Reset value for the least-empty bin.  The last bin
	// (SUPERBLOCK_FULLNESS_GROUP-1) is for completely full superblocks,
	// so we use the next-to-last bin.
	enum { RESET_LEAST_EMPTY_BIN = SUPERBLOCK_FULLNESS_GROUP - 2 };

	// The number of empty superblocks that we allow any thread heap to
	// hold once the thread heap has fallen below 1/EMPTY_FRACTION
	// empty.
	enum { MAX_EMPTY_SUPERBLOCKS = 1 } ; // EMPTY_FRACTION / 2 };

	// The maximum number of thread heaps we allow.  (NOT the maximum
	// number of threads -- Hoard imposes no such limit.)  This must be
	// a power of two! NB: This number is twice the maximum number of
	// PROCESSORS supported by Hoard. 2P
	enum { MAX_HEAPS = 64 };

	// ANDing with this rounds to MAX_HEAPS.
	enum { MAX_HEAPS_MASK = MAX_HEAPS - 1 };

	//
	// The number of size classes.
	//

	enum { SIZE_CLASSES = 132 };

	// Every object is aligned so that it can always hold a double.
	enum { ALIGNMENT = sizeof(double) };

	// ANDing with this rounds to ALIGNMENT.
	enum { ALIGNMENT_MASK = ALIGNMENT - 1};

	// Used for sanity checking.
	enum { HEAP_MAGIC = 0x0badcafe };

	// Get the usage and allocated statistics.
	inline void getStats (int sizeclass, int& U, int& A);


#if HEAP_STATS
	// How much is the maximum ever in use for this size class?
	inline int maxInUse (int sizeclass);

	// How much is the maximum memory allocated for this size class?
	inline int maxAllocated (int sizeclass);
#endif

	// Insert a superblock into our list.
	void insertSuperblock (int sizeclass,
	                       superblock * sb,
	                       persistentHeap * persistentheap);

	// Remove the superblock with the most free space.
	superblock * removeMaxSuperblock (int sizeclass);

	// Remove a superblock of some given fullness.
	superblock * removePartiallyFullSuperblock (int fullness, int sizeclass);

	// Find an available superblock (i.e., with some space in it).
	superblock * findAvailableSuperblock (int sizeclass,
	                                      block *& b,
	                                      persistentHeap * persistentheap);

	// Lock this heap.
	inline void lock (void);

	// Unlock this heap.
	inline void unlock (void);

	// Set our index number (which heap we are).
	inline void setIndex (int i);

	// Get our index number (which heap we are).
	inline int getIndex (void);

	// Free a block into a superblock.
	// This is used by processHeap::free().
	// Returns 1 iff the superblock was munmapped.
	int freeBlock (block *& b,
	               superblock *& sb,
	               int sizeclass,
	               persistentHeap * persistentheap);

	//// Utility functions ////

	// Return the size class for a given size.
	inline static int sizeClass (const size_t sz);

	// Return the size corresponding to a given size class.
	inline static size_t sizeFromClass (const int sizeclass);

	// Return the release threshold corresponding to a given size class.
	inline static int getReleaseThreshold (const int sizeclass);

	// Return how many blocks of a given size class fit into a superblock.
	inline static int numBlocks (const int sizeclass);

	// Align a value.
	inline static size_t align (const size_t sz);

	void printSuperblockList();
private:

	// Disable copying and assignment.

	hoardHeap (const hoardHeap&);
	const hoardHeap& operator= (const hoardHeap&);

	// Recycle a superblock.
	inline void recycle (superblock *);

	// Reuse a superblock (if one is available).
	inline superblock * reuse (int sizeclass);

	// Remove a particular superblock.
	inline void removeSuperblock (superblock *, int sizeclass);

public:
	// Move a particular superblock from one bin to another.
	void moveSuperblock (superblock *,
	                     int sizeclass,
	                     int fromBin,
	                     int toBin);
private:

	// Update memory in-use and allocated statistics.
	// (*UStats = just update U.)
	inline void incStats (int sizeclass, int updateU, int updateA);

public:
	inline void incUStats (int sizeclass);
private:

	inline void decStats (int sizeclass, int updateU, int updateA);
	inline void decUStats (int sizeclass);

	//// Members ////

#if HEAP_DEBUG
	// For sanity checking.
	const unsigned long _magic;
#else
  #define _magic HEAP_MAGIC
#endif

  // Heap statistics.
  heapStats	_stats[SIZE_CLASSES];

  // The per-heap lock.
  hoardLockType _lock;

  // Which heap this is (0 = the process (global) heap).
  int _index;

  // Reusable superblocks.
  superblock *	_reusableSuperblocks;
  int		_reusableSuperblocksCount;

  // Lists of superblocks.
  superblock *	_superblocks[SUPERBLOCK_FULLNESS_GROUP][SIZE_CLASSES];

  // The current least-empty superblock bin.
  int	_leastEmptyBin[SIZE_CLASSES];

  // The lookup table for size classes.
  static size_t	_sizeTable[SIZE_CLASSES];

  // The lookup table for release thresholds.
  static size_t	_threshold[SIZE_CLASSES];

public:
  // A little helper class that we use to define some statics.
  class _initNumProcs {
  public:
  	_initNumProcs(void);
  };

  friend class _initNumProcs;
protected:
  // number of CPUs, cached
  static int _numProcessors;
  static int _numProcessorsMask;
};



void hoardHeap::incStats (int sizeclass, int updateU, int updateA) {
  assert (_magic == HEAP_MAGIC);
  assert (updateU >= 0);
  assert (updateA >= 0);
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  _stats[sizeclass].incStats (updateU, updateA);
}



void hoardHeap::incUStats (int sizeclass) {
  assert (_magic == HEAP_MAGIC);
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  _stats[sizeclass].incUStats ();
}


void hoardHeap::decStats (int sizeclass, int updateU, int updateA) {
  assert (_magic == HEAP_MAGIC);
  assert (updateU >= 0);
  assert (updateA >= 0);
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  _stats[sizeclass].decStats (updateU, updateA);
}


void hoardHeap::decUStats (int sizeclass)
{
  assert (_magic == HEAP_MAGIC);
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  _stats[sizeclass].decUStats();
}


void hoardHeap::getStats (int sizeclass, int& U, int& A) {
  assert (_magic == HEAP_MAGIC);
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  _stats[sizeclass].getStats (U, A);
}


#if HEAP_STATS
int hoardHeap::maxInUse (int sizeclass) {
  assert (_magic == HEAP_MAGIC);
  return _stats[sizeclass].getUmax();
}


int hoardHeap::maxAllocated (int sizeclass) {
  assert (_magic == HEAP_MAGIC);
  return _stats[sizeclass].getAmax(); 
}
#endif


int hoardHeap::sizeClass (const size_t sz) {
  // Find the size class for a given object size
  // (the smallest i such that _sizeTable[i] >= sz).
  int sizeclass = 0;
  while (_sizeTable[sizeclass] < sz) 
    {
      sizeclass++;
      assert (sizeclass < SIZE_CLASSES);
    }
  return sizeclass;
}


size_t hoardHeap::sizeFromClass (const int sizeclass) {
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  return _sizeTable[sizeclass];
}


int hoardHeap::getReleaseThreshold (const int sizeclass) {
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  return _threshold[sizeclass];
}


int hoardHeap::numBlocks (const int sizeclass) {
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);
  const size_t s = sizeFromClass (sizeclass);
  assert (s > 0);
  const int blksize = align (s);
  // Compute the number of blocks that will go into this superblock.
  int nb =  MAX (1, ((SUPERBLOCK_SIZE) / blksize));
  return nb;
}


void hoardHeap::lock (void) 
{
  assert (_magic == HEAP_MAGIC);
  hoardLock (_lock);
}


void hoardHeap::unlock (void) {
  assert (_magic == HEAP_MAGIC);
  hoardUnlock (_lock);
}


size_t hoardHeap::align (const size_t sz)
{
  // Align sz up to the nearest multiple of ALIGNMENT.
  // This is much faster than using multiplication
  // and division.
  return (sz + ALIGNMENT_MASK) & ~((size_t) ALIGNMENT_MASK);
}


void hoardHeap::setIndex (int i) 
{
  _index = i; 
}


int hoardHeap::getIndex (void)
{
  return _index;
}




void hoardHeap::recycle (superblock * sb)
{
	assert (sb != NULL);
	assert (sb->getOwner() == this);
	assert (sb->getNumBlocks() > 1);
	assert (sb->getNext() == NULL);
	assert (sb->getPrev() == NULL);
	assert (hoardHeap::numBlocks(sb->getBlockSizeClass()) > 1);
	sb->insertBefore (_reusableSuperblocks);
	_reusableSuperblocks = sb;
	++_reusableSuperblocksCount;
	//printf ("hoardHeap::recycle: heap = %d, count = %d, sb = %p\n", getIndex(), _reusableSuperblocksCount, sb);
}


superblock * hoardHeap::reuse (int sizeclass)
{ 
  persistentSuperblock *psb;

  if (_reusableSuperblocks == NULL) {
    return NULL;
  }

  // Make sure that we aren't using a sizeclass
  // that is too big for a 'normal' superblock.
  if (hoardHeap::numBlocks(sizeclass) <= 1) {
    return NULL;
  }

  // Pop off a superblock from the reusable-superblock list.
  assert (_reusableSuperblocksCount > 0);
  superblock * sb = _reusableSuperblocks;
  _reusableSuperblocks = sb->getNext();
  sb->remove();
  psb = sb->getPersistentSuperblock();
  assert (psb != NULL);
  assert (sb->getNumBlocks() > 1);
  --_reusableSuperblocksCount;

  // Reformat the superblock if necessary.
  if (sb->getBlockSizeClass() != sizeclass) {
    decStats (sb->getBlockSizeClass(),
	      sb->getNumBlocks() - sb->getNumAvailable(),
	      sb->getNumBlocks());
    sb->superblock::~superblock();
    
	//FIXME: what to do with stats inc/dec? do I need to reflect those to the persistent superblock?
	//FIXME: what to do with the persistent superblock? keep the same
	assert(psb->isFree() == true); // persistent superblock must be free to change its sizeclass
	int new_blksize = sizeFromClass(sizeclass);
	psb->setBlockSize(new_blksize);

	// BEGIN HACK
	// Look at the hack note  under the persistentSuperblock constuctor to see why we reallocate
	// memory. Original Hoard didn't do this.
	char *buf;
	unsigned long moreMemory;

	moreMemory = hoardHeap::align(sizeof(superblock) + (hoardHeap::align (sizeof(block)) * numBlocks(sizeclass)));
	// Get some memory from the process heap.
	buf = (char *) hoardGetMemory (moreMemory);
    //sb = new ((char *) sb) superblock (numBlocks(sizeclass), sizeclass, this, psb);
    sb = new ((char *) buf) superblock (numBlocks(sizeclass), sizeclass, this, psb);
	// END HACK

    psb->setSuperblock(sb);

    incStats (sizeclass,
	      sb->getNumBlocks() - sb->getNumAvailable(),
	      sb->getNumBlocks());
  }

  assert (sb->getOwner() == this);
  assert (sb->getBlockSizeClass() == sizeclass);
  return sb;
}

#endif // _HEAP_H_
