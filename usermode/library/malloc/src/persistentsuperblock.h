#ifndef _PERSISTENTSUPERBLOCK_H
#define _PERSISTENTSUPERBLOCK_H

#include "config.h"

#include <iostream>
#include <stdint.h>
#include <assert.h>

#include "hoardheap.h"

class superblock; // forward declaration

#define PCM_STORE(addr, val)                               \
        *(addr) = val; 

#define PCM_BARRIER

#if 0
# define __BITMAP_ZERO(bitsetp) \
  do {									      \
    unsigned int __i;							      \
    bitmap_t *__arr = (bitsetp);					      \
    for (__i = 0; __i < sizeof (bitmap_t) / sizeof (__bitmap_mask); ++__i)      \
      __arr->__bits[__i] = 0;						      \
  } while (0)
#endif

/* Basic access macros for bitmaps.  */
# define __BITMAPELT(bit)	((bit) / BITMAP_ARRAY_ENTRY_SIZE_BITS)
# define __BITMAPMASK(bit)	((uint64_t) 1 << ((bit) % BITMAP_ARRAY_ENTRY_SIZE_BITS))
# define __BITMAP_SET(bit, bitmap)                            \
  PCM_STORE(&bitmap[__BITMAPELT (bit)],                       \
            bitmap[__BITMAPELT (bit)] | __BITMAPMASK (bit));
# define __BITMAP_CLR(bit, bitmap)                            \
  PCM_STORE(&bitmap[__BITMAPELT (bit)],                       \
            bitmap[__BITMAPELT (bit)] & ~__BITMAPMASK (bit));
# define __BITMAP_ISSET(bit, bitmap) \
  ((bitmap[__BITMAPELT (bit)] & __BITMAPMASK (bit)) != 0)


class persistentSuperblock {

public:
	enum { PERSISTENTSUPERBLOCK_SIZE = 8192  };
	enum { PERSISTENTBLOCK_MIN_SIZE = 8  };
	enum { BITMAP_SIZE = PERSISTENTSUPERBLOCK_SIZE / PERSISTENTBLOCK_MIN_SIZE  };
	enum { BITMAP_ARRAY_ENTRY_SIZE_BITS = 8 * sizeof(uint64_t)  };
	enum { BITMAP_ARRAY_ENTRY_MASK = -1  };
	enum { BITMAP_ARRAY_SIZE = BITMAP_SIZE / BITMAP_ARRAY_ENTRY_SIZE_BITS  };

	persistentSuperblock(char *pregion) {
		int i;
		for (i=0; i<BITMAP_ARRAY_SIZE; i++) {
			PCM_STORE(&_bitmap[i], 0);
		}
		PCM_STORE(&_pregion, pregion);
		PCM_STORE(&_blksize, PERSISTENTBLOCK_MIN_SIZE);
		PCM_BARRIER
	}

	void volatileInit() {
		_cached = false;
		_acquired = false;
	}

	bool isAcquired() {
		return _acquired;
	}

	void acquire() {
		_acquired = true;
	}

	int getFullness() {
		int i;
		int setBits = 0;

		for (i=0; i<BITMAP_SIZE; i++) {
			if (__BITMAP_ISSET(i, _bitmap)) {
				setBits++;
			}
		}

		return (SUPERBLOCK_FULLNESS_GROUP-1) * setBits / BITMAP_SIZE;
	}

	void allocBlock(int index) {
		int bitsPerBlock = _blksize / PERSISTENTBLOCK_MIN_SIZE;
		int firstBit = index * bitsPerBlock;
		int i;

		for (i=firstBit; i<firstBit+bitsPerBlock; i++) {
			__BITMAP_SET(i, _bitmap);
		}
	}

	void freeBlock(int index) {
		int bitsPerBlock = _blksize / PERSISTENTBLOCK_MIN_SIZE;
		int firstBit = index * bitsPerBlock;
		int i;

		for (i=firstBit; i<firstBit+bitsPerBlock; i++) {
			__BITMAP_CLR(i, _bitmap);
		}
	}

	bool isBlockFree(int index) {
		int bitsPerBlock = _blksize / PERSISTENTBLOCK_MIN_SIZE;
		int firstBit = index * bitsPerBlock;

		if (__BITMAP_ISSET(firstBit, _bitmap)) {
			return false;
		}
		return true;
	}


	void * getBlockRegion(int index) {
		std::cout << "getBlockRegion: " << _pregion << std::endl;
		std::cout << "getBlockRegion: " << index << std::endl;
		return (void *) (_pregion + index * _blksize);
	}
	
	int getNumBlocks (void)
	{
		  return PERSISTENTSUPERBLOCK_SIZE/_blksize;
	}

	int getNumAvailable (void)
	{
		int bitsPerBlock = _blksize / PERSISTENTBLOCK_MIN_SIZE;
		int i;
		int numAvailable = 0;

		for (i=0; i<BITMAP_SIZE; i+=bitsPerBlock) {
			if (__BITMAP_ISSET(i, _bitmap)) {
				numAvailable++;
			}
		}

		return PERSISTENTSUPERBLOCK_SIZE/_blksize;
	}

	bool isFree() {
		int i;

		for (i=0; i<BITMAP_ARRAY_SIZE; i++) {
			if (_bitmap[i] != 0) {
				return false;
			}
		}
	
		return true;
	}

	void setBlockSize(int blksize) {
		assert(isFree() == true); // block must be free to change its size class 
		_blksize = blksize;
	}

	int getBlockSize() {
		return _blksize;
	}

	void makePersistentSuperblock(int blksize);

	superblock *getSuperblock() {
		return _superblock;
	}

	void setSuperblock(superblock *sb) {
		_superblock = sb;
	}

private:
	int      _blksize;
	uint64_t _bitmap[BITMAP_ARRAY_SIZE];
	char     *_pregion;

	// volatile members
	bool       _cached;
	bool       _acquired;
	superblock *_superblock; // points to the volatile superblock caching this persistent superblock
};

#endif
