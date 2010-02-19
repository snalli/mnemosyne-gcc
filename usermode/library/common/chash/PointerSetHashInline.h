/*
 *  PointerSetHash.h
 *  CuckooHashTable
 *
 *  Created by Steve Dekorte on 2009 04 28.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef POINTERHASH_DEFINED
#define POINTERHASH_DEFINED 1

#include "Common.h"
#include <stddef.h>
#include "PortableStdint.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif

#define POINTERHASH_MAXLOOP 10

#include "PointerSetHash_struct.h"

static inline BASEKIT_API PointerSetHash *PointerSetHash_new(void);
static inline BASEKIT_API void PointerSetHash_copy_(PointerSetHash *self, const PointerSetHash *other);
static inline BASEKIT_API PointerSetHash *PointerSetHash_clone(PointerSetHash *self);
static inline BASEKIT_API void PointerSetHash_free(PointerSetHash *self);

static inline BASEKIT_API void PointerSetHash_at_put_(PointerSetHash *self, void *k);
static inline BASEKIT_API void PointerSetHash_removeKey_(PointerSetHash *self, void *k);
static inline BASEKIT_API size_t PointerSetHash_size(PointerSetHash *self); // actually the keyCount

static inline BASEKIT_API size_t PointerSetHash_memorySize(PointerSetHash *self);
static inline BASEKIT_API void PointerSetHash_compact(PointerSetHash *self);

// --- private methods ----------------------------------------

static inline BASEKIT_API void PointerSetHash_setSize_(PointerSetHash *self, size_t size); 
static inline BASEKIT_API void PointerSetHash_insert_(PointerSetHash *self, PointerSetHashRecord *x); 
static inline BASEKIT_API void PointerSetHash_grow(PointerSetHash *self); 
static inline BASEKIT_API void PointerSetHash_shrinkIfNeeded(PointerSetHash *self); 
static inline BASEKIT_API void PointerSetHash_shrink(PointerSetHash *self); 
static inline BASEKIT_API void PointerSetHash_show(PointerSetHash *self);
static inline BASEKIT_API void PointerSetHash_updateMask(PointerSetHash *self); 

#undef IOINLINE
#define IOINLINE static inline

// -----------------------------------

#define PointerSetHashRecords_recordAt_(records, pos) (PointerSetHashRecord *)(records + (pos * sizeof(PointerSetHashRecord)))


IOINLINE PointerSetHashRecord *PointerSetHash_record1_(PointerSetHash *self, void *k)
{
	// the ~| 0x1 before the mask ensures an odd pos
	intptr_t kk = (intptr_t)k;
	size_t pos = ((kk^(kk>>4)) | 0x1) & self->mask;
	return PointerSetHashRecords_recordAt_(self->records, pos);
}

IOINLINE PointerSetHashRecord *PointerSetHash_record2_(PointerSetHash *self, void *k)
{
	// the | 0x1 before the mask ensures an even pos
	intptr_t kk = (intptr_t)k;
	//size_t pos = (((kk^(kk/33)) << 1)) & self->mask;
	size_t pos = (kk << 1) & self->mask;
	return PointerSetHashRecords_recordAt_(self->records, pos);
}

IOINLINE void *PointerSetHash_at_(PointerSetHash *self, void *k)
{
	PointerSetHashRecord *r;
	
	r = PointerSetHash_record1_(self, k);
	if(k == r->k) return 1;
	
	r = PointerSetHash_record2_(self, k);	
	if(k == r->k) return 1;
	
	return 0x0;
}

IOINLINE size_t PointerSetHash_count(PointerSetHash *self)
{
	return self->keyCount;
}

IOINLINE int PointerSetHashKey_hasKey_(PointerSetHash *self, void *key)
{
	return PointerSetHash_at_(self, key) != NULL;
}

IOINLINE void PointerSetHash_at_put_(PointerSetHash *self, void *k)
{
	PointerSetHashRecord *r;
	
	r = PointerSetHash_record1_(self, k);
	
	if(!r->k)
	{
		r->k = k;
		self->keyCount ++;
		return;
	}
	
	if(r->k == k)
	{
		return;
	}

	r = PointerSetHash_record2_(self, k);

	if(!r->k)
	{
		r->k = k;
		self->keyCount ++;
		return;
	}
	
	if(r->k == k)
	{
		return;
	}
	
	{
	PointerSetHashRecord x;
	x.k = k;
	PointerSetHash_insert_(self, &x);
	}
}

IOINLINE void PointerSetHash_shrinkIfNeeded(PointerSetHash *self)
{
	if(self->keyCount < self->size/8)
	{
		PointerSetHash_shrink(self);
	}
}

IOINLINE void PointerSetHashRecord_swapWith_(PointerSetHashRecord *self, PointerSetHashRecord *other)
{
	PointerSetHashRecord tmp = *self;
	*self = *other;
	*other = tmp;
}

IOINLINE void PointerSetHash_clean(PointerSetHash *self)
{
	memset(self->records, 0, sizeof(PointerSetHashRecord) * self->size);
	self->keyCount = 0;
}

// --- enumeration --------------------------------------------------

#define POINTERHASH_FOREACH(self, pkey, code) \
{\
	PointerSetHash *_self = (self);\
	unsigned char *_records = _self->records;\
	unsigned int _i, _size = _self->size;\
	void *pkey;\
	void *pvalue;\
	\
	for (_i = 0; _i < _size; _i ++)\
	{\
		PointerSetHashRecord *_record = PointerSetHashRecords_recordAt_(_records, _i);\
		if (_record->k)\
		{\
			pkey = _record->k;\
			code;\
		}\
	}\
}

// -----------------------------------

IOINLINE PointerSetHash *PointerSetHash_new(void)
{
	PointerSetHash *self = (PointerSetHash *)io_calloc(1, sizeof(PointerSetHash));
	PointerSetHash_setSize_(self, 8);
	return self;
}

IOINLINE void PointerSetHash_copy_(PointerSetHash *self, const PointerSetHash *other)
{
	io_free(self->records);
	memcpy(self, other, sizeof(PointerSetHash));
	self->records = malloc(self->size * sizeof(PointerSetHashRecord));
	memcpy(self->records, other->records, self->size * sizeof(PointerSetHashRecord));
}

IOINLINE PointerSetHash *PointerSetHash_clone(PointerSetHash *self)
{
	PointerSetHash *other = PointerSetHash_new();
	PointerSetHash_copy_(other, self);
	return other;
}

IOINLINE void PointerSetHash_setSize_(PointerSetHash *self, size_t size)
{
	self->records = realloc(self->records, size * sizeof(PointerSetHashRecord));
	
	if(size > self->size)
	{		
		memset(self->records + self->size * sizeof(PointerSetHashRecord), 
			0x0, (size - self->size) * sizeof(PointerSetHashRecord));
	}
	
	self->size = size;
	
	PointerSetHash_updateMask(self);
}

IOINLINE void PointerSetHash_updateMask(PointerSetHash *self)
{
	self->mask = (intptr_t)(self->size - 1);
}

IOINLINE void PointerSetHash_show(PointerSetHash *self)
{
	size_t i;
	
	printf("PointerSetHash records:\n");
	for(i = 0; i < self->size; i++)
	{
		PointerSetHashRecord *r = PointerSetHashRecords_recordAt_(self->records, i);
		printf("  %i: %p\n", (int)i, r->k);
	}
}

IOINLINE void PointerSetHash_free(PointerSetHash *self)
{
	io_free(self->records);
	io_free(self);
}

IOINLINE void PointerSetHash_insert_(PointerSetHash *self, PointerSetHashRecord *x)
{	
	int n;
	
	for (n = 0; n < POINTERHASH_MAXLOOP; n ++)
	{ 
		PointerSetHashRecord *r;
		
		r = PointerSetHash_record1_(self, x->k);
		PointerSetHashRecord_swapWith_(x, r);
		if(x->k == 0x0) { self->keyCount ++; return; }
		 
		r = PointerSetHash_record2_(self, x->k);
		PointerSetHashRecord_swapWith_(x, r);
		if(x->k == 0x0) { self->keyCount ++; return; }
	}
	
	PointerSetHash_grow(self); 
	PointerSetHash_at_put_(self, x->k);
}

IOINLINE void PointerSetHash_insertRecords(PointerSetHash *self, unsigned char *oldRecords, size_t oldSize)
{
	size_t i;
	
	for (i = 0; i < oldSize; i ++)
	{
		PointerSetHashRecord *r = PointerSetHashRecords_recordAt_(oldRecords, i);
		
		if (r->k)
		{
			PointerSetHash_at_put_(self, r->k);
		}
	}
}

IOINLINE void PointerSetHash_resizeTo_(PointerSetHash *self, size_t newSize)
{
	unsigned char *oldRecords = self->records;
	size_t oldSize = self->size;
	self->size = newSize;
	self->records = io_calloc(1, sizeof(PointerSetHashRecord) * self->size);
	self->keyCount = 0;
	PointerSetHash_updateMask(self);
	PointerSetHash_insertRecords(self, oldRecords, oldSize);
	io_free(oldRecords);
}

IOINLINE void PointerSetHash_grow(PointerSetHash *self)
{
	PointerSetHash_resizeTo_(self, self->size * 2);
}

void PointerSetHash_shrink(PointerSetHash *self)
{
	PointerSetHash_resizeTo_(self, self->size / 2);
}

IOINLINE void PointerSetHash_removeKey_(PointerSetHash *self, void *k)
{
	PointerSetHashRecord *r;
	
	r = PointerSetHash_record1_(self, k);	
	if(r->k == k)
	{
		r->k = 0x0;
		self->keyCount --;
		PointerSetHash_shrinkIfNeeded(self);
		return;
	}
	
	r = PointerSetHash_record2_(self, k);
	if(r->k == k)
	{
		r->k = 0x0;
		self->keyCount --;
		PointerSetHash_shrinkIfNeeded(self);
		return;
	}
}

IOINLINE size_t PointerSetHash_size(PointerSetHash *self) // actually the keyCount
{
	return self->keyCount;
}

// ----------------------------

IOINLINE size_t PointerSetHash_memorySize(PointerSetHash *self)
{
	return sizeof(PointerSetHash) + self->size * sizeof(PointerSetHashRecord);
}

IOINLINE void PointerSetHash_compact(PointerSetHash *self)
{
}


#define PointerSetHash_cleanSlots(self)
#define PointerSetHash_hasDirtyKey_(self, k) 0

#ifdef __cplusplus
}
#endif
#endif
