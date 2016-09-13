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
  superblock.h
  ------------------------------------------------------------------------
  The superblock class controls a number of blocks (which are
  allocatable units of memory).
  ------------------------------------------------------------------------
  @(#) $Id: superblock.h,v 1.41 2001/12/20 15:36:20 emery Exp $
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

#ifndef _SUPERBLOCK_H_
#define _SUPERBLOCK_H_

#include "config.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "arch-specific.h"
#include "block.h"
#include "persistentsuperblock.h"
#include <map>

#define debug_race 0

#if debug_race
#define __m_time__                ({gettimeofday(&mtm_time, NULL); \
                                        (double)mtm_time.tv_sec +    \
                                        ((double)mtm_time.tv_usec / 1000000);})
#define __m_tid__                 syscall(SYS_gettid)
#define __m_debug()               fprintf(stderr, "%lf %d %s %d \n",__m_time__,__m_tid__, __func__, __LINE__);
#define __m_fail()               fprintf(stderr, "FAIL %lf %d %s %d \n",__m_time__,__m_tid__, __func__, __LINE__);
#define __m_print(args ...)	 fprintf(stderr, args);
#else
#define __m_time__                ({gettimeofday(&mtm_time, NULL); \
                                        (double)mtm_time.tv_sec +    \
                                        ((double)mtm_time.tv_usec / 1000000);})
#define __m_tid__                 syscall(SYS_gettid)
#define __m_debug()		 {;}
#define __m_fail()		 {;}
#define __m_print(args ...)	 {;}
#endif

class hoardHeap;            // forward declaration
class processHeap;          // forward declaration
class persistentHeap;       // forward declaration

class superblock {

public:

  // Construct a superblock for a given size class and set the heap
  // owner.
  superblock (int numblocks,
	      int sizeclass,
	      hoardHeap * owner,
          persistentSuperblock *psb);

  ~superblock (void)
  {
	  hoardLockDestroy (_upLock);
  }

  // Make (allocate or re-use) a superblock for a given size class.
  static superblock * makeSuperblock (int sizeclass, persistentSuperblock *pSuperblock);
  static superblock * makeSuperblock (persistentSuperblock *pSuperblock);

  // Find out who allocated this superblock.
  inline hoardHeap * getOwner (void);

  // Set the superblock's owner.
  inline void setOwner (hoardHeap * o);

  // Get (allocate) a block from the superblock.
  inline block * acquireBlock (void);

  // Put a block back in the superblock.
  inline bool putBlock (block * b);

  // Get a pointer to the block identified by id.
  block * getBlock (int id);

  // How many blocks are available?
  inline int getNumAvailable (void);

  // How many blocks are there, in total?
  inline int getNumBlocks (void);

  // What size class are blocks in this superblock?
  inline int getBlockSizeClass (void);

  // Insert this superblock before the next one.
  inline void insertBefore (superblock * nextSb);

  // Return the next pointer (to the next superblock in the list).
  inline superblock * getNext (void);

  // Return the prev pointer (to the previous superblock in the list).
  inline superblock * getPrev (void);

  // Return the 'fullness' of this superblock.
  inline int getFullness (void);
  
#if HEAP_FRAG_STATS
  // Return the amount of waste in every allocated block.
  int getMaxInternalFragmentation (void);
#endif

  // Remove this superblock from its linked list.
  inline void remove (void);

  // Is this superblock valid? (i.e.,
  // does it have the right magic number?)
  inline int isValid (void);

  inline void upLock (void) {
    hoardLock (_upLock);
  }

  inline void upUnlock (void) {
    hoardUnlock (_upLock);
  }

  // Compute the 'fullness' of this superblock.
  inline void computeFullness (void);

  inline persistentSuperblock *getPersistentSuperblock(void);
  inline void *getBlockRegion(int id);
private:
  typedef std::map<block*, bool>  blocks_present_t;
  blocks_present_t blocks_present;
  // Disable copying and assignment.

  superblock (const superblock&);
  const superblock& operator= (const superblock&);

  // Used for sanity checking.
  enum { SUPERBLOCK_MAGIC = 0xCAFEBABE };

#if HEAP_DEBUG
  unsigned long _magic;
#endif

  const int 	_sizeClass;	// The size class of blocks in the superblock.
  const int 	_numBlocks;	// The number of blocks in the superblock.
  int		_numAvailable;	// The number of blocks available.
  int		_fullness;	// How full is this superblock?
				// (which SUPERBLOCK_FULLNESS group is it in)
  block *	_freeList;	// A pointer to the first free block.
  hoardHeap * 	_owner;		// The heap who owns this superblock.
  superblock * 	_next;		// The next superblock in the list.
  superblock * 	_prev;		// The previous superblock in the list.

  persistentSuperblock *_psb; // The persistent superblock backing this volatile superblock header.
  bool dirtyFullness;

  hoardLockType _upLock;	// Lock this when moving a superblock to the global (process) heap.

  // We insert a cache pad here to prevent false sharing with the
  // first block (which immediately follows the superblock).

  double _pad[CACHE_LINE / sizeof(double)];
};


hoardHeap * superblock::getOwner (void)
{
	assert (isValid());
	hoardHeap * o = _owner;
	__m_debug();
  __m_print("%lf %d %s %d sb=%p o=%p\n",__m_time__, __m_tid__, __func__, __LINE__, this, o, getNumAvailable());
	return o;
}


void superblock::setOwner (hoardHeap * o) 
{
	__m_debug();
  __m_print("%lf %d %s %d sb=%p o=%p\n",__m_time__, __m_tid__, __func__, __LINE__, this, o, getNumAvailable());
	assert (isValid());
	_owner = o;
}

__TM_CALLABLE__
block * superblock::acquireBlock (void)
{
	__m_debug();
	assert (isValid());
  __m_print("%lf %d %s(1) %d sb=%p freelist=%p num=%d\n",__m_time__, __m_tid__, __func__, __LINE__, this, _freeList, getNumAvailable());
	// Pop off a block from this superblock's freelist,
	// if there is one available.
	if (_freeList == NULL) {
		// The freelist is empty.
		__m_fail();
		assert (getNumAvailable() == 0);
		return NULL;
	}
	assert (getNumAvailable() > 0);
	block * b = _freeList;
	_freeList = _freeList->getNext();
	_numAvailable--;
  __m_print("%lf %d %s(2) %d sb=%p b=%p freelist=%p num=%d\n",__m_time__, __m_tid__, __func__, __LINE__, this, b, _freeList, getNumAvailable());
	_psb->allocBlock(b->getId());

	b->setNext(NULL);

	dirtyFullness = true;
	//  computeFullness();

  	// blocks_present[b] = false;
	return b;
}


bool superblock::putBlock (block * b)
{
  __m_debug();
  __m_print("%lf %d %s(1) %d sb=%p b=%p free=%p num=%d\n",__m_time__, __m_tid__, __func__, __LINE__, this, b, _freeList, getNumAvailable());
  
  //if(blocks_present[b] == true)
	// return false;
  assert (isValid());
  // Push a block onto the superblock's freelist.
  assert (b->isValid());
  assert (b->getSuperblock() == this);
  assert (getNumAvailable() < getNumBlocks());
  b->setNext (_freeList);
  _freeList = b;
  _numAvailable++;
  __m_print("%lf %d %s(2) %d sb=%p b=%p free=%p num=%d\n",__m_time__, __m_tid__, __func__, __LINE__, this, b, _freeList, getNumAvailable());
  _psb->freeBlock(b->getId());

  dirtyFullness = true;
  // blocks_present[b] = true;
  return true;
  //  computeFullness();
}



int superblock::getNumAvailable (void)
{
  assert (isValid());
  return _numAvailable;
}


int superblock::getNumBlocks (void)
{
  assert (isValid());
  return _numBlocks;
}


int superblock::getBlockSizeClass (void)
{
  assert (isValid());
  return _sizeClass;
}


superblock * superblock::getNext (void)
{
  assert (isValid());
  return _next; 
}

superblock * superblock::getPrev (void)
{
  assert (isValid());
  return _prev; 
}


void superblock::insertBefore (superblock * nextSb) {
  assert (isValid());
  // Insert this superblock before the next one (nextSb).
  assert (nextSb != this);
  _next = nextSb;
  if (nextSb) {
    _prev = nextSb->_prev;
    nextSb->_prev = this;
  }
}


void superblock::remove (void) {
  // Remove this superblock from a doubly-linked list.
  if (_next) {
    _next->_prev = _prev;
  }
  if (_prev) {
    _prev->_next = _next;
  }
  _prev = NULL;
  _next = NULL;
}

__TM_CALLABLE__
int superblock::isValid (void)
{
  assert (_numBlocks > 0);
  assert (_numAvailable <= _numBlocks);
  assert (_sizeClass >= 0);
  return 1;
}


void superblock::computeFullness (void)
{
	assert (isValid());
	_fullness = (((SUPERBLOCK_FULLNESS_GROUP - 1)
	              * (getNumBlocks() - getNumAvailable())) / getNumBlocks());
}

int superblock::getFullness (void)
{
	assert (isValid());
	if (dirtyFullness) {
		computeFullness();
		dirtyFullness = false;
	}
	return _fullness;
}

persistentSuperblock *superblock::getPersistentSuperblock(void)
{
	return _psb;
}

__TM_CALLABLE__
void *superblock::getBlockRegion(int id)
{
	return _psb->getBlockRegion(id);
}


#endif // _SUPERBLOCK_H_
