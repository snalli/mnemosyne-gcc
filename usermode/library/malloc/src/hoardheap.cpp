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

#include "hoardheap.h"
#include "processheap.h"
#include "persistentheap.h"
#include "superblock.h"

size_t hoardHeap::_sizeTable[hoardHeap::SIZE_CLASSES]
= {8UL, 16UL, 24UL, 32UL, 40UL, 48UL, 56UL, 64UL, 72UL, 80UL, 88UL, 96UL, 104UL, 112UL, 120UL, 128UL, 136UL, 144UL, 152UL, 160UL, 168UL, 176UL, 184UL, 192UL, 200UL, 208UL, 216UL, 224UL, 232UL, 240UL, 248UL, 256UL, 264UL, 272UL, 280UL, 288UL, 296UL, 304UL, 312UL, 320UL, 328UL, 336UL, 344UL, 352UL, 360UL, 368UL, 376UL, 384UL, 392UL, 400UL, 408UL, 416UL, 424UL, 432UL, 440UL, 448UL, 456UL, 464UL, 472UL, 480UL, 488UL, 496UL, 504UL, 512UL, 576UL, 640UL, 704UL, 768UL, 832UL, 896UL, 960UL, 1024UL, 1088UL, 1152UL, 1216UL, 1280UL, 1344UL, 1408UL, 1472UL, 1536UL, 1600UL, 1664UL, 1728UL, 1792UL, 1856UL, 1920UL, 1984UL, 2048UL, 2112UL, 2560UL, 3072UL, 3584UL, 4096UL, 4608UL, 5120UL, 5632UL, 6144UL, 6656UL, 7168UL, 7680UL, 8192UL, 8704UL, 9216UL, 9728UL, 10240UL, 10752UL, 12288UL, 16384UL, 20480UL, 24576UL, 28672UL, 32768UL, 36864UL, 40960UL, 65536UL, 98304UL, 131072UL, 163840UL, 262144UL, 524288UL, 1048576UL, 2097152UL, 4194304UL, 8388608UL, 16777216UL, 33554432UL, 67108864UL, 134217728UL, 268435456UL, 536870912UL, 1073741824UL, 2147483648UL};

size_t hoardHeap::_threshold[hoardHeap::SIZE_CLASSES] = {1024UL, 512UL, 341UL, 256UL, 204UL, 170UL, 146UL, 128UL, 113UL, 102UL, 93UL, 85UL, 78UL, 73UL, 68UL, 64UL, 60UL, 56UL, 53UL, 51UL, 48UL, 46UL, 44UL, 42UL, 40UL, 39UL, 37UL, 36UL, 35UL, 34UL, 33UL, 32UL, 31UL, 30UL, 29UL, 28UL, 27UL, 26UL, 26UL, 25UL, 24UL, 24UL, 23UL, 23UL, 22UL, 22UL, 21UL, 21UL, 20UL, 20UL, 20UL, 19UL, 19UL, 18UL, 18UL, 18UL, 17UL, 17UL, 17UL, 17UL, 16UL, 16UL, 16UL, 16UL, 14UL, 12UL, 11UL, 10UL, 9UL, 9UL, 8UL, 8UL, 7UL, 7UL, 6UL, 6UL, 6UL, 5UL, 5UL, 5UL, 5UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 4UL, 3UL, 3UL, 2UL, 2UL, 2UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL, 1UL};

hoardHeap::hoardHeap (void)
  : _index (0),
    _reusableSuperblocks (NULL),
    _reusableSuperblocksCount (0)
#if HEAP_DEBUG
  , _magic (HEAP_MAGIC)
#endif
{
  // Initialize the per-heap lock.
  hoardLockInit (_lock);
  for (int i = 0; i < SUPERBLOCK_FULLNESS_GROUP; i++) {
    for (int j = 0; j < SIZE_CLASSES; j++) {
      // Initialize all superblocks lists to empty.
      _superblocks[i][j] = NULL;
    }
  }
  // Hoard resets leastEmptyBin[k] to 0 because initially bins
  // contain no superblocks. However, in our case the persistent
  // heap may have partially-full superblocks. So if 
  // findAvailableSuperblock does not find any superblock it checks
  // whether the persistentHeap has any. To make this work correctly
  // leastEmptyBin has to be set to RESET_LEAST_EMPTY_BIN to make
  // sure all bins are checked at least once. 
  for (int k = 0; k < SIZE_CLASSES; k++) {
    //_leastEmptyBin[k] = 0;
    _leastEmptyBin[k] = RESET_LEAST_EMPTY_BIN;
  }
}

// FIXME:
// Currently findAvailableSuperblock will check whether the persistent heap
// has any partially-full superblock if the current heap does not. 
// This lazy approach avoids distributing persistent superblocks to thread
// heaps when the allocator is first initialized. However it seems that it
// complicated the recycling policy. Consider revising the policy.
superblock * hoardHeap::findAvailableSuperblock (int sizeclass,
                                                 block *& b,
                                                 persistentHeap * persistentheap)
{
  assert (this);
  assert (_magic == HEAP_MAGIC);
  assert (sizeclass >= 0);
  assert (sizeclass < SIZE_CLASSES);

  superblock * sb = NULL;
  int reUsed = 0;

  // Look through the superblocks, starting with the almost-full ones
  // and going to the emptiest ones.  The Least Empty Bin for a
  // sizeclass is a conservative approximation (fixed after one
  // iteration) of the first bin that has superblocks in it, starting
  // with (surprise) the least-empty bin.
  for (int i = _leastEmptyBin[sizeclass]; i >= 0; i--) {
    sb = _superblocks[i][sizeclass];
	if (sb == NULL) {
      // Check whether the persistent heap has a superblock of fullness i and 
	  // size class sizeclass available.
      sb = persistentheap->removePartiallyFullSuperblock(i, sizeclass);
      if (sb) {
        // Insert it into the bin list.

        insertSuperblock (sizeclass, sb, persistentheap);

        assert (sb->getOwner() == this);
        break;
	  }
      if (i == _leastEmptyBin[sizeclass]) {
        // There wasn't a superblock in this bin,
        // so we adjust the least empty bin.
        _leastEmptyBin[sizeclass]--;
      }
    } else {
      assert (sb->getOwner() == this);
      break;
    }
  } 

  if (sb == NULL) {
    // Try to reuse a superblock.
    sb = reuse (sizeclass);
    if (sb) {
      assert (sb->getOwner() == this);
      reUsed = 1;
    }
  }

  if (sb != NULL) {
    // Sanity checks:
    //   This superblock is 'valid'.
    assert (sb->isValid());
    //   This superblock has the right ownership.
    assert (sb->getOwner() == this);
    
    int oldFullness = sb->getFullness();

    // Now get a block from the superblock.
    // This superblock must have space available.
    b = sb->acquireBlock();
    assert (b != NULL);
    
    // Update the stats.
    incUStats (sizeclass);
    
    if (reUsed) {
      insertSuperblock (sizeclass, sb, persistentheap);
      // Fix the stats (since insert will just have incremented them
      // by this amount).
      decStats (sizeclass,
		sb->getNumBlocks() - sb->getNumAvailable(),
		sb->getNumBlocks());
    } else {
      // If we've crossed a fullness group,
      // move the superblock.
      int fullness = sb->getFullness();
      
      if (fullness != oldFullness) {
        // Move the superblock.
        moveSuperblock (sb, sizeclass, oldFullness, fullness);
      }
    }
  }

  // Either we didn't find a superblock or we did and got a block.
  assert ((sb == NULL) || (b != NULL));
  // Either we didn't get a block or we did and we also got a superblock.
  assert ((b == NULL) || (sb != NULL));

  return sb;
}



void hoardHeap::insertSuperblock (int sizeclass,
                                  superblock * sb,
                                  persistentHeap *)
{
	assert (sb->isValid());
	assert (sb->getBlockSizeClass() == sizeclass);
	assert (sb->getPrev() == NULL);
	assert (sb->getNext() == NULL);
	assert (_magic == HEAP_MAGIC);

	// Now it's ours.
	sb->setOwner (this);

	// How full is this superblock?  We'll use this information to put
	// it into the right 'bin'.
	sb->computeFullness();
	int fullness = sb->getFullness();

	// Update the stats.
	incStats (sizeclass,
	          sb->getNumBlocks() - sb->getNumAvailable(),
	          sb->getNumBlocks());

#if 0
	printf("hoardHeap::insertSuperBlock sb: %p\n", sb);
	printf("\tfullness: %d\n", fullness);
	printf("\tsizeclass: %d\n", sizeclass);
	printf("\tnumAvailable: %d\n", sb->getNumAvailable());
	printf("\tnumBlocks: %d\n", sb->getNumBlocks());
#endif

	if ((fullness == 0) &&
	    (sb->getNumBlocks() > 1) &&
	    (sb->getNumBlocks() == sb->getNumAvailable())) 
	{
		// This superblock is empty --
		// recycle this superblock (make it available for any sizeclass).
		recycle (sb);
	} else {
		// Insert it into the appropriate list.
		superblock *& head = _superblocks[fullness][sizeclass];
		sb->insertBefore (head);
		head = sb;
		assert (head->isValid());
    
		// Reset the least-empty bin counter.
		_leastEmptyBin[sizeclass] = RESET_LEAST_EMPTY_BIN;
	}
}


void hoardHeap::printSuperblockList()
{
  superblock *sb = NULL;

  std::cout  << "printSuperblockList" << std::endl;
  for (int i = 0; i < 9; i--) {
    sb = _superblocks[i][12];
	if (sb) {
		std::cout << "sb: " << sb << std::endl;
		std::cout << "  ->psb: " << sb->getPersistentSuperblock() << std::endl;
	}	
  }
}


superblock * hoardHeap::removeMaxSuperblock (int sizeclass)
{
	assert (_magic == HEAP_MAGIC);

	superblock * head = NULL;

	// First check the reusable superblocks list.

	head = reuse (sizeclass);
	if (head) {
		// We found one. Since we're removing this superblock, update the
		// stats accordingly.
		decStats (sizeclass,
		          0,
		          head->getNumBlocks());

		return head;
	}
	// Instead of finding the superblock with the most available space
	// (something that would either involve a linear scan through the
	// superblocks or maintaining the superblocks in sorted order), we
	// just pick one that is no more than
	// 1/(SUPERBLOCK_FULLNESS_GROUP-1) more full than the superblock
	// with the most available space.  We start with the emptiest group.

	int i = 0;

	// HARIS: Doing what is suggested by the comment below could give us
	// a full block. The original code does this for robustness because it
	// is captured in the assertion check that I commented out. So I don't
	// check the last group.
	//
	// Note: the last group (SUPERBLOCK_FULLNESS_GROUP - 1) is full, so
	// we never need to check it. But for robustness, we leave it in.
	// while (i < SUPERBLOCK_FULLNESS_GROUP) {
	while (i < (SUPERBLOCK_FULLNESS_GROUP - 1)) {
		head = _superblocks[i][sizeclass];
		if (head) {
			break;
		}
		i++;
	}

	if (!head) {
		return NULL;
	}

	// When we reincarnate, we bring any partially filled superblocks into 
	// the _superblocks array but we don't enforce Hoard's invariant: 
	//   (head->getNumAvailable() * EMPTY_FRACTION >= head->getNumBlocks())
	// because this would lead to a memory leak. 

	// Make sure that this superblock is at least 1/EMPTY_FRACTION empty.
	//assert (head->getNumAvailable() * EMPTY_FRACTION >= head->getNumBlocks());

	removeSuperblock (head, sizeclass);

	assert (head->isValid());
	assert (head->getPrev() == NULL);
	assert (head->getNext() == NULL);

	return head;
}

superblock * hoardHeap::removePartiallyFullSuperblock (int fullness, int sizeclass)
{
	assert (_magic == HEAP_MAGIC);

	superblock * head = NULL;

	head = _superblocks[fullness][sizeclass];

	if (!head) {
		return NULL;
	}

	// When we reincarnate, we bring any partially filled superblocks into 
	// the _superblocks array but we don't enforce Hoard's invariant: 
	//   (head->getNumAvailable() * EMPTY_FRACTION >= head->getNumBlocks())
	// because this would lead to a memory leak. 

	// Make sure that this superblock is at least 1/EMPTY_FRACTION empty.
	//assert (head->getNumAvailable() * EMPTY_FRACTION >= head->getNumBlocks());

	removeSuperblock (head, sizeclass);

	assert (head->isValid());
	assert (head->getPrev() == NULL);
	assert (head->getNext() == NULL);
	return head;
}


void hoardHeap::removeSuperblock (superblock * sb,
                                  int sizeclass)
{
	assert (_magic == HEAP_MAGIC);

	assert (sb->isValid());
	assert (sb->getOwner() == this);
	assert (sb->getBlockSizeClass() == sizeclass);

	for (int i = 0; i < SUPERBLOCK_FULLNESS_GROUP; i++) {
		if (sb == _superblocks[i][sizeclass]) {
			_superblocks[i][sizeclass] = sb->getNext();
			if (_superblocks[i][sizeclass] != NULL) {
				assert (_superblocks[i][sizeclass]->isValid());
			}
			break;
		}
	}

	sb->remove();
	decStats (sizeclass, sb->getNumBlocks() - sb->getNumAvailable(), sb->getNumBlocks());
}


void hoardHeap::moveSuperblock (superblock * sb,
				int sizeclass,
				int fromBin,
				int toBin)
{
	assert (_magic == HEAP_MAGIC);
	assert (sb->isValid());
	assert (sb->getOwner() == this);
	assert (sb->getBlockSizeClass() == sizeclass);
	assert (sb->getFullness() == toBin);

	// Remove the superblock from the old bin.

	superblock *& oldHead = _superblocks[fromBin][sizeclass];
	if (sb == oldHead) {
		oldHead = sb->getNext();
		if (oldHead != NULL) {
			assert (oldHead->isValid());
		}
	}

	sb->remove();

	// Insert the superblock into the new bin.

	superblock *& newHead = _superblocks[toBin][sizeclass];
	sb->insertBefore (newHead);
	newHead = sb;
	assert (newHead->isValid());

	// Reset the least-empty bin counter.
	_leastEmptyBin[sizeclass] = RESET_LEAST_EMPTY_BIN;
}


// The heap lock must be held when this procedure is called.

int hoardHeap::freeBlock (block *& b,
			   superblock *& sb,
			   int sizeclass,
			   persistentHeap * persistentheap)
{
	assert (sb->isValid());
	assert (b->isValid());
	assert (this == sb->getOwner());

	const int oldFullness = sb->getFullness();
	sb->putBlock (b);
#if 0 // DEBUGPRINT
	std::cout << "hoardheap::freeBlock  hoardheap(this) = " << this 
	          << "sb = " << sb
	          << ", b = " << b
	          << ", sizeclass = " << sizeclass
	          << ", size = " << sizeFromClass(sizeclass) << std::endl;
#endif
	decUStats (sizeclass);
	const int newFullness = sb->getFullness();
  
	// Free big superblocks.
	if (sb->getNumBlocks() == 1) {
		//TODO: currently our persistent allocator handles blocks of size smaller that superblock size
		assert (0);
	    removeSuperblock (sb, sizeclass);
#if HEAP_LOG
		// Record the memory deallocation.
		MemoryRequest m;
		m.deallocate ((int) sb->getNumBlocks() * (int) sizeFromClass(sb->getBlockSizeClass()));
		persistentheap->getLog(getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
		persistentheap->setDeallocated (0, sb->getNumBlocks() * sizeFromClass(sb->getBlockSizeClass()));
#endif
		sb->superblock::~superblock();
		hoardFreeMemory (sb);
		return 1;
	}

	// If the fullness value has changed, move the superblock.
	if (newFullness != oldFullness) {
		moveSuperblock (sb, sizeclass, oldFullness, newFullness);
	} else {
		// Move the superblock to the front of its list (to reduce paging).
		superblock *& head = _superblocks[newFullness][sizeclass];
		if (sb != head) {
			sb->remove();
			sb->insertBefore (head);
			head = sb;
		}
	}
 
	// If the superblock is now empty, recycle it or free it.

	if ((newFullness == 0) &&
	    (sb->getNumBlocks() == sb->getNumAvailable())) 
	{
		removeSuperblock (sb, sizeclass);
		if (_numProcessors == 1) {
			assert(0);
			hoardFreeMemory (sb);
		} else {
			recycle (sb);
			// Update the stats.  This restores the stats to their state
			// before the call to removeSuperblock, above.
			incStats (sizeclass, 0, sb->getNumBlocks());
		}
	}

	// If this is the persistent heap, then we're done.
	if (this == (hoardHeap *) persistentheap) {
		return 0;
	}

	//FIXME: What is the right policy here for persistent blocks?
	//TODO: What is the right policy here for persistent blocks?

	//
	// Release a superblock, if necessary.
	//

	//
	// Check to see if the amount free exceeds the release threshold
	// (two superblocks worth of blocks for a given sizeclass) and if
	// the heap is sufficiently empty.
	//

	// We never move anything to the process heap if we're on a
	// uniprocessor.
	if (_numProcessors > 1) {
		int inUse, allocated;
		getStats (sizeclass, inUse, allocated);
		const bool crossedThreshold 
		               = ((inUse < allocated - getReleaseThreshold(sizeclass)) && 
		                  (EMPTY_FRACTION * inUse < EMPTY_FRACTION * allocated - allocated));
		if (crossedThreshold) {
			// We've crossed the magical threshold. Find the superblock with
			// the most free blocks and give it to the process heap.
			superblock * const maxSb = removeMaxSuperblock (sizeclass);
			assert (maxSb != NULL);

			// Update the statistics.

			assert (maxSb->getNumBlocks() >= maxSb->getNumAvailable());

			// Give the superblock back to the process heap.
			persistentheap->release (maxSb);
    	}
	}

	return 0;
}

// Static initialization of the number of processors (and a mask).
   
int hoardHeap::_numProcessors;
int hoardHeap::_numProcessorsMask;

hoardHeap::_initNumProcs::_initNumProcs(void)
{
  hoardHeap::_numProcessors = hoardGetNumProcessors();
  hoardHeap::_numProcessorsMask = (1 << (lg(hoardGetNumProcessors()) + 4)) - 1;
  if (hoardHeap::_numProcessors > MAX_HEAPS) {
	  hoardHeap::_numProcessorsMask = MAX_HEAPS_MASK;
  }
}

static hoardHeap::_initNumProcs initProcs;
