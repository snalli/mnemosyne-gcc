#include <sys/time.h>
#include <mnemosyne.h>
#include <stdint.h>
#include <sys/mman.h>
#include "config.h"
#include "threadheap.h"
#include "persistentheap.h"
#include "persistentsuperblock.h"

MNEMOSYNE_PERSISTENT void *psegmentheader = 0;
MNEMOSYNE_PERSISTENT void *psegment = 0;

persistentHeap::persistentHeap (void)
  : _buffer (NULL),
    _bufferCount (0)
{
	int                  i;
	persistentSuperblock *psb;

	// Format persistent heap; this happens only the first time 
	// the heap is ever incarnated.
	format();
	scavenge();
	
	// Initialize some logically non-persistent information 
	// FIXME: This should really be implemented as a volatile index to avoid 
	// writing PCM cells
	for(i=0; i<PERSISTENTSUPERBLOCK_NUM; i++) { 
		psb = (persistentSuperblock *) ((uintptr_t) psegmentheader + i*sizeof(persistentSuperblock));
		psb->volatileInit();
	}	
}


void persistentHeap::format()
{
	int   i;
	void  *b;
	void  *buf;

	assert((!psegmentheader && !psegment) || (psegmentheader && psegment));

	if (!psegmentheader) {
		psegmentheader = m_pmap((void *) PERSISTENTHEAP_HEADER_BASE, PERSISTENTSUPERBLOCK_NUM*sizeof(persistentSuperblock), PROT_READ|PROT_WRITE, 0);
		assert(psegmentheader != (void *) -1);
		psegment = m_pmap((void *)PERSISTENTHEAP_BASE, (uint64_t) persistentSuperblock::PERSISTENTSUPERBLOCK_SIZE * PERSISTENTSUPERBLOCK_NUM, PROT_READ|PROT_WRITE, 0);
		assert(psegment != (void *) -1);
		for(i=0; i<PERSISTENTSUPERBLOCK_NUM; i++) { 
			b = (void *) ((uintptr_t) psegmentheader + i*sizeof(persistentSuperblock));
			buf = (void *) ((uintptr_t) psegment + i*persistentSuperblock::PERSISTENTSUPERBLOCK_SIZE);
			new(b) persistentSuperblock((char *)buf);
		}
		_psegmentBase = psegment;
	}
}


// For every persistent superblock create a superblock that higher layers can use
void persistentHeap::scavenge()
{
#ifdef _M_STATS_BUILD
	struct timeval     start_time;
	struct timeval     stop_time;
	unsigned long long op_time;
#endif

	int                  i;
	int                  sizeclass;
	int                  blksize;
	persistentSuperblock *psb;
	superblock           *sb;

#ifdef _M_STATS_BUILD
	gettimeofday(&start_time, NULL);
#endif
	for(i=0; i<PERSISTENTSUPERBLOCK_NUM; i++) { 
		psb = (persistentSuperblock *) ((uintptr_t) psegmentheader + i*sizeof(persistentSuperblock));
		blksize = psb->getBlockSize();
		sizeclass = sizeClass(blksize);
		sb = superblock::makeSuperblock (psb);
		insertSuperblock (sizeclass, sb, (persistentHeap *) NULL);
#if 0
		std::cout << "psb: " << psb << std::endl;
		std::cout << "  ->fullness : " << psb->getFullness() << std::endl;
		std::cout << "  ->isFree   : " << psb->isFree() << std::endl;
		std::cout << "  ->sizeClass: " << sizeclass << std::endl;
		std::cout << "  ->blksize: " << blksize << std::endl;
		std::cout << "sb: " << sb << std::endl;
		std::cout << "  ->numBlocks: " << sb->getNumBlocks() << std::endl;
		std::cout << "  ->numAvailable: " << sb->getNumAvailable() << std::endl;
		std::cout << "  ->getFullness: " << sb->getFullness() << std::endl;
#endif		
	}
#ifdef _M_STATS_BUILD
	gettimeofday(&stop_time, NULL);
	op_time = 1000000 * (stop_time.tv_sec - start_time.tv_sec) +
	                     stop_time.tv_usec - start_time.tv_usec;
	printf("persistent_heap_scavenge_latency = %llu (us)\n", op_time);
#endif
}


// Print out statistics information.
void persistentHeap::stats (void) {
}



// free (ptr, pheap):
//   inputs: a pointer to an object allocated by malloc().
//   side effects: returns the block to the object's superblock;
//                 updates the thread heap's statistics;
//                 may release the superblock to the process heap.

void persistentHeap::free (void * ptr)
{
	// Return if ptr is 0.
	// This is the behavior prescribed by the standard.
	if (ptr == 0) {
		return;
	}

	// Find the block and superblock corresponding to this ptr.

	uintptr_t psb_index = ((uintptr_t) ptr - (uintptr_t) psegment) / persistentSuperblock::PERSISTENTSUPERBLOCK_SIZE;
	persistentSuperblock *psb = (persistentSuperblock *) ((uintptr_t) psegmentheader + psb_index * sizeof(persistentSuperblock));
	int blksize = psb->getBlockSize();
	uintptr_t block_index = (((uintptr_t) ptr - (uintptr_t) psegment) % persistentSuperblock::PERSISTENTSUPERBLOCK_SIZE) / blksize;
	superblock *sb = psb->getSuperblock();
	assert (sb);
	assert (sb->isValid());
	block *b = sb->getBlock(block_index);
	//std::cout << "block_index = " << block_index << std::endl;
	//std::cout << "psb = " << psb << std::endl;
	//std::cout << "1.sb = " << sb << std::endl;

	// Check to see if this block came from a memalign() call.
	// TODO: Currently we don't support memalign()

	b->markFree();

	assert (sb == b->getSuperblock());
	assert (sb);
	assert (sb->isValid());

	const int sizeclass = sb->getBlockSizeClass();

	//
	// Return the block to the superblock,
	// find the heap that owns this superblock
	// and update its statistics.
	//

	hoardHeap * owner;

	// By acquiring the up lock on the superblock,
	// we prevent it from moving to the global heap.
	// This eventually pins it down in one heap,
	// so this loop is guaranteed to terminate.
	// (It should generally take no more than two iterations.)
	sb->upLock();
	for (;;) {
		owner = sb->getOwner();
		owner->lock();
		if (owner == sb->getOwner()) {
			break;
		} else {
			owner->unlock();
		}
		// Suspend to allow ownership to quiesce.
		hoardYield();
	}

#if HEAP_LOG
	MemoryRequest m;
	m.free (ptr);
	getLog (owner->getIndex()).append(m);
#endif
#if HEAP_FRAG_STATS
	setDeallocated (b->getRequestedSize(), 0);
#endif

	int sbUnmapped = owner->freeBlock (b, sb, sizeclass, this);

	owner->unlock();
	if (!sbUnmapped) {
		sb->upUnlock();
	}
}

size_t persistentHeap::objectSize (void * ptr) 
{
	// Find the superblock pointer.
	uintptr_t psb_index = ((uintptr_t) ptr - (uintptr_t) psegment) / persistentSuperblock::PERSISTENTSUPERBLOCK_SIZE;
	persistentSuperblock *psb = (persistentSuperblock *) ((uintptr_t) psegmentheader + psb_index * sizeof(persistentSuperblock));
	int blksize = psb->getBlockSize();
	uintptr_t block_index = (((uintptr_t) ptr - (uintptr_t) psegment) % persistentSuperblock::PERSISTENTSUPERBLOCK_SIZE) / blksize;
	superblock *sb = psb->getSuperblock();
	assert (sb);
	assert (sb->isValid());
  
  // Return the size.
  return sizeFromClass (sb->getBlockSizeClass());
}


