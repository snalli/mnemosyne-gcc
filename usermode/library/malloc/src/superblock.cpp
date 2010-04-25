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
  superblock.cpp
  ------------------------------------------------------------------------
  The superblock class controls a number of blocks (which are
  allocatable units of memory).
  ------------------------------------------------------------------------
  @(#) $Id: superblock.cpp,v 1.45 2001/12/18 02:42:56 emery Exp $
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

#include <string.h>

#include "arch-specific.h"
#include "config.h"
#include "hoardheap.h"
#include "superblock.h"
#include "persistentheap.h"
#include "persistentsuperblock.h"

#include <iostream>
using namespace std;

superblock::superblock (int numBlocks,	// The number of blocks in the sb.
                        int szclass,	// The size class of the blocks.
                        hoardHeap * o,	// The heap that "owns" this sb.
                        persistentSuperblock *psb) // The persistent sb backing this volatile sb
                       :
#if HEAP_DEBUG
	_magic (SUPERBLOCK_MAGIC),
#endif
	_sizeClass (szclass),
	_numBlocks (numBlocks),
	_numAvailable (0),
	_fullness (0),
	_freeList (NULL),
	_owner (o),
	_next (NULL),
	_prev (NULL),
	dirtyFullness (true)
{
	assert (_numBlocks >= 1);


	// Determine the size of each block. This does not include the buffer 
	// since the block is actually a cached header for the 
	const int blksize =  hoardHeap::align (sizeof(block));

	// Make sure this size is in fact aligned.
	assert ((blksize & hoardHeap::ALIGNMENT_MASK) == 0);

	// Set the first block to just past this volatile superblock header.
	block * b = (block *) hoardHeap::align ((unsigned long) (this + 1));

	_psb = psb;
	// Initialize all the blocks,
	// and insert the block pointers into the linked list.
	for (int i = 0; i < _numBlocks; i++) {
		// Make sure the block is on a double-word boundary.
		assert (((uintptr_t) b & hoardHeap::ALIGNMENT_MASK) == 0);
		new (b) block (this);
		assert (b->getSuperblock() == this);
		b->setId (i);
		if (psb->isBlockFree(i)) {
			b->setNext (_freeList);
			_freeList = b;
			_numAvailable++;
		}
		b = (block *) ((char *) b + blksize);
	}
	computeFullness();
	assert ((uintptr_t) b <= hoardHeap::align (sizeof(superblock) + blksize * _numBlocks) + (uintptr_t) this);

	hoardLockInit (_upLock);
}


superblock * superblock::makeSuperblock (int sizeclass,
                                         persistentSuperblock *pSuperblock)
{
	char *buf;
	int  numBlocks = hoardHeap::numBlocks(sizeclass);
	int  blksize = hoardHeap::sizeFromClass(sizeclass);
  
	// Compute how much memory we need.
	unsigned long moreMemory;

	moreMemory = hoardHeap::align(sizeof(superblock) + (hoardHeap::align (sizeof(block)) * numBlocks));
	// Get some memory from the process heap.
	buf = (char *) hoardGetMemory (moreMemory);
 
	// Make sure that we actually got the memory.
	if (buf == NULL) {
		return 0;
	}
	buf = (char *) hoardHeap::align ((unsigned long) buf);

	// Make sure this buffer is double-word aligned.
	assert (buf == (char *) hoardHeap::align ((unsigned long) buf));
	assert ((((unsigned long) buf) & hoardHeap::ALIGNMENT_MASK) == 0);

	// If sizeclass is different from the pSuperblock's sizeclass then make
	// sure that the pSuperblock is free
	if (pSuperblock->getBlockSize() != blksize) {
		assert(pSuperblock->isFree() == true);
		pSuperblock->setBlockSize(blksize);
	}

	// Instantiate the new superblock in the buffer.

	superblock * sb = new (buf) superblock (numBlocks, sizeclass, NULL, pSuperblock);
	pSuperblock->setSuperblock(sb);

	return sb;
}


superblock * superblock::makeSuperblock (persistentSuperblock *pSuperblock)
{
	// We need to get more memory.

	int  blksize = pSuperblock->getBlockSize();
	int  sizeclass = hoardHeap::sizeClass(blksize);

	return makeSuperblock(sizeclass, pSuperblock);
}


block *superblock::getBlock (int id)
{
	block * b = (block *) hoardHeap::align ((unsigned long) (this + 1));
	const int blksize = hoardHeap::align (sizeof(block));
  
	b = (block *) ((char *) b + id*blksize);
	return b;
}
