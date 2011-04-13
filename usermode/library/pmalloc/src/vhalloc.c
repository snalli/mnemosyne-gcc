/**
 * \file vhalloc.c 
 * 
 * \brief A simple persistent memory allocator based on Vistaheap
 *
 */

#define VHALLOC_PREGION_BASE          0xc00000000
#define VHALLOC_PREGION_METADATA_SIZE (1024*1024)        /* Bytes */
#define VHALLOC_PREGION_HOLE_SIZE     0
#define VHALLOC_PREGION_HEAP_SIZE     (1024*1024*1024)   /* Bytes */

/*
 * A single persistent region is allocated to store the two vistaheap_s 
 * structures, the nugget metadata, and the allocated chunks. 
 *
 * +-------------------------------------------------------------------------+
 * |                                                                         |
 * |  +------------------+  +--------------------+                           |
 * |  | main vistaheap_s |  | nugget vistaheap_s |                           |
 * |  +------------------+  +--------------------+                           |
 * |                                                                         |
 * +-------------------------------------------------------------------------+  
 * | Nugget metadata kept here                                               |
 * |                                                                         |
 * +-------------------------------------------------------------------------+  
 * | Chunks allocated here                                                   |
 * |                                                                         |
 * |                                                                         |
 *                                      ... 
 * |                                                                         |
 * +-------------------------------------------------------------------------+  
 *
 */

#include <mnemosyne.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pcm.h>
#include "vistaheap.h"
#include "genalloc.h"
#include "vhalloc.h"


__attribute__ ((section("PERSISTENT"))) int      vhalloc_init_done = 0; 
__attribute__ ((section("PERSISTENT"))) uint64_t vhalloc_pregion = 0x0;

static int       volatile_init_done = 0; 
static vistaheap *vistaheap_main;
static vistaheap *vistaheap_nugget;

TM_PURE static inline
void
_init(void)
{
	size_t         size;
	void           *metadata_base;           
	void           *metadata_hardlimit;
	void           *heap_base;           
	void           *heap_hardlimit;

	if (vhalloc_init_done) {
		if (!volatile_init_done) {
			vistaheap_main = (vistaheap *) vhalloc_pregion;
			vistaheap_nugget = (vistaheap *) (vhalloc_pregion + sizeof(vistaheap));
			volatile_init_done = 1;
		}	
		return;
	}

	if (vhalloc_pregion == 0x0) {
		size = sizeof(vistaheap) * 2 + 
			   VHALLOC_PREGION_METADATA_SIZE + 
			   VHALLOC_PREGION_HOLE_SIZE + 
			   VHALLOC_PREGION_HEAP_SIZE;
	    TM_WAIVER {
			vhalloc_pregion = (uint64_t) m_pmap((void *) VHALLOC_PREGION_BASE, size, PROT_READ|PROT_WRITE, 0);
		}	
		if ((void *) vhalloc_pregion == (void *) -1) {
			TM_WAIVER {
				abort();
			}	
		}
	}

	vistaheap_main = (vistaheap *) vhalloc_pregion;
	vistaheap_nugget = (vistaheap *) (vhalloc_pregion + sizeof(vistaheap));
	metadata_base = (void *) (vhalloc_pregion + sizeof(vistaheap)*2);
	metadata_hardlimit = (void *) (((uintptr_t) metadata_base) + VHALLOC_PREGION_METADATA_SIZE);
	heap_base = (void *) (((uintptr_t) metadata_hardlimit) + VHALLOC_PREGION_HOLE_SIZE);
	heap_hardlimit = (void *) (((uintptr_t) heap_base) + VHALLOC_PREGION_HEAP_SIZE);

	__tm_atomic 
	{
		vistaheap_init(vistaheap_nugget, metadata_base, metadata_hardlimit, NULL);
		vistaheap_init(vistaheap_main, heap_base, heap_hardlimit, vistaheap_nugget);
		vhalloc_init_done = 1;
	} 
	volatile_init_done = 1;
}

struct header_s {
	size_t sz;
};


size_t VHALLOC_OBJSIZE(void *ptr)
{
	struct header_s* header_ptr;
	header_ptr = (struct header_s *) ((char *)ptr - sizeof(struct header_s));
	return header_ptr->sz;
}


void *
VHALLOC_PMALLOC(size_t sz)
{
	void *ptr;
	struct header_s* header_ptr;
	_init();
	/* HACK: Vistaheap does not track size so we track it in a header ourselves */
	ptr = vistaheap_malloc(vistaheap_main, sz + sizeof(struct header_s));
	header_ptr = (struct header_s *) ptr;
	header_ptr->sz = sz;
	return (void *) ((char *) ptr + sizeof(struct header_s));
}


void 
vhalloc_pfree(void *ptr)
{
	//TODO
}
