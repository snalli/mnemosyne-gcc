/*
 * Copyright (c) 1997, 1998, 1999, 2000, David E. Lowell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Vista library, version 0.6.1, September 2000
 */

#ifndef HEAP_H
#define HEAP_H

#include <sys/types.h>
#include <sys/mman.h>


#define NBUCKETS 32
#define BUCKET_MIN 3 /* log base 2 of smallest bucket size */
#define EXTENDSIZE 131072 /* minimum amount that morecore extends the file */
#define DEFAULT_PROT (PROT_READ | PROT_WRITE)
#define MMAP_FLAGS (MAP_FILE | MAP_FIXED | MAP_SHARED)
#define UP_TO_PAGE(x) (x%PAGESIZE==0?x:(x-(x%PAGESIZE)+PAGESIZE))
#define DOWN_TO_PAGE(x) (x%PAGESIZE==0?x:(x-(x%PAGESIZE)))
#define MAP_ADDR ((void*)0x20000000000L)
#define MAP_ADDR2 ((void*)0x21000000000L)

typedef struct nugget_s {
	void*            addr;
	struct nugget_s* next;
} nugget;

typedef struct vistaheap_s {
	nugget*             bucketlists[NBUCKETS];
	nugget*             nlist; /* list of free nuggets for internals */
	char*               base;
	char*               limit;
	char*               hardlimit;
	volatile void*      key;	  /* can point to a vista_segment (volatile) */
	struct vistaheap_s* allocator; /* vistaheap from which to alloc internal data */
} vistaheap;

__attribute__((transaction_safe)) extern void* vistaheap_init(vistaheap*, void*, void*, vistaheap*);
__attribute__((transaction_safe)) extern void* vistaheap_malloc(vistaheap*, int);
__attribute__((transaction_safe)) extern void  vistaheap_free(vistaheap* h, void* p, int size);
__attribute__((transaction_safe)) extern void* morecore(vistaheap* h, int pages);
__attribute__((transaction_safe)) extern void  vistaheap_reinit_mutex(vistaheap* h);

#endif

