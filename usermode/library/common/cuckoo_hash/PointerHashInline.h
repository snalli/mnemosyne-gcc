/*
 *  PointerHash.h
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

#include "PointerHash_struct.h"

static inline BASEKIT_API PointerHash *PointerHash_new(void);
static inline BASEKIT_API void PointerHash_copy_(PointerHash *self, const PointerHash *other);
static inline BASEKIT_API PointerHash *PointerHash_clone(PointerHash *self);
static inline BASEKIT_API void PointerHash_free(PointerHash *self);

static inline BASEKIT_API void PointerHash_at_put_(PointerHash *self, void *k, void *v);
static inline BASEKIT_API void PointerHash_removeKey_(PointerHash *self, void *k);
static inline BASEKIT_API size_t PointerHash_size(PointerHash *self); // actually the keyCount

static inline BASEKIT_API size_t PointerHash_memorySize(PointerHash *self);
static inline BASEKIT_API void PointerHash_compact(PointerHash *self);

// --- private methods ----------------------------------------

static inline BASEKIT_API void PointerHash_setSize_(PointerHash *self, size_t size); 
static inline BASEKIT_API void PointerHash_insert_(PointerHash *self, PointerHashRecord *x); 
static inline BASEKIT_API void PointerHash_grow(PointerHash *self); 
static inline BASEKIT_API void PointerHash_shrinkIfNeeded(PointerHash *self); 
static inline BASEKIT_API void PointerHash_shrink(PointerHash *self); 
static inline BASEKIT_API void PointerHash_show(PointerHash *self);
static inline BASEKIT_API void PointerHash_updateMask(PointerHash *self); 

#undef IOINLINE
#define IOINLINE static inline

// -----------------------------------

#define PointerHashRecords_recordAt_(records, pos) (PointerHashRecord *)(records + (pos * sizeof(PointerHashRecord)))


IOINLINE PointerHashRecord *PointerHash_record1_(PointerHash *self, void *k)
{
	// the ~| 0x1 before the mask ensures an odd pos
	intptr_t kk = (intptr_t)k;
	size_t pos = ((kk^(kk>>4)) | 0x1) & self->mask;
	return PointerHashRecords_recordAt_(self->records, pos);
}

IOINLINE PointerHashRecord *PointerHash_record2_(PointerHash *self, void *k)
{
	// the | 0x1 before the mask ensures an even pos
	intptr_t kk = (intptr_t)k;
	//size_t pos = (((kk^(kk/33)) << 1)) & self->mask;
	size_t pos = (kk << 1) & self->mask;
	return PointerHashRecords_recordAt_(self->records, pos);
}

IOINLINE void *PointerHash_at_(PointerHash *self, void *k)
{
	PointerHashRecord *r;
	
	r = PointerHash_record1_(self, k);
	if(k == r->k) return r->v;
	
	r = PointerHash_record2_(self, k);	
	if(k == r->k) return r->v;
	
	return 0x0;
}

IOINLINE size_t PointerHash_count(PointerHash *self)
{
	return self->keyCount;
}

IOINLINE int PointerHashKey_hasKey_(PointerHash *self, void *key)
{
	return PointerHash_at_(self, key) != NULL;
}

IOINLINE void PointerHash_at_put_(PointerHash *self, void *k, void *v)
{
	PointerHashRecord *r;
	
	r = PointerHash_record1_(self, k);
	
	if(!r->k)
	{
		r->k = k;
		r->v = v;
		self->keyCount ++;
		return;
	}
	
	if(r->k == k)
	{
		r->v = v;
		return;
	}

	r = PointerHash_record2_(self, k);

	if(!r->k)
	{
		r->k = k;
		r->v = v;
		self->keyCount ++;
		return;
	}
	
	if(r->k == k)
	{
		r->v = v;
		return;
	}
	
	{
	PointerHashRecord x;
	x.k = k;
	x.v = v;
	PointerHash_insert_(self, &x);
	}
}

IOINLINE void PointerHash_shrinkIfNeeded(PointerHash *self)
{
	if(self->keyCount < self->size/8)
	{
		PointerHash_shrink(self);
	}
}

IOINLINE void PointerHashRecord_swapWith_(PointerHashRecord *self, PointerHashRecord *other)
{
	PointerHashRecord tmp = *self;
	*self = *other;
	*other = tmp;
}

IOINLINE void PointerHash_clean(PointerHash *self)
{
	memset(self->records, 0, sizeof(PointerHashRecord) * self->size);
	self->keyCount = 0;
}


IOINLINE PointerHash *PointerHash_new(void)
{
	PointerHash *self = (PointerHash *)io_calloc(1, sizeof(PointerHash));
	PointerHash_setSize_(self, 8);
	return self;
}

IOINLINE void PointerHash_copy_(PointerHash *self, const PointerHash *other)
{
	io_free(self->records);
	memcpy(self, other, sizeof(PointerHash));
	self->records = (unsigned char *) malloc(self->size * sizeof(PointerHashRecord));
	memcpy(self->records, other->records, self->size * sizeof(PointerHashRecord));
}

IOINLINE PointerHash *PointerHash_clone(PointerHash *self)
{
	PointerHash *other = PointerHash_new();
	PointerHash_copy_(other, self);
	return other;
}

IOINLINE void PointerHash_setSize_(PointerHash *self, size_t size)
{
	self->records = (unsigned char *) realloc(self->records, size * sizeof(PointerHashRecord));
	
	if(size > self->size)
	{		
		memset(self->records + self->size * sizeof(PointerHashRecord), 
			0x0, (size - self->size) * sizeof(PointerHashRecord));
	}
	
	self->size = size;
	
	PointerHash_updateMask(self);
}

IOINLINE void PointerHash_updateMask(PointerHash *self)
{
	self->mask = (intptr_t)(self->size - 1);
}

IOINLINE void PointerHash_show(PointerHash *self)
{
	size_t i;
	
	printf("PointerHash records:\n");
	for(i = 0; i < self->size; i++)
	{
		PointerHashRecord *r = PointerHashRecords_recordAt_(self->records, i);
		printf("  %i: %p %p\n", (int)i, r->k, r->v);
	}
}

IOINLINE void PointerHash_free(PointerHash *self)
{
	io_free(self->records);
	io_free(self);
}

IOINLINE void PointerHash_insert_(PointerHash *self, PointerHashRecord *x)
{	
	int n;
	
	for (n = 0; n < POINTERHASH_MAXLOOP; n ++)
	{ 
		PointerHashRecord *r;
		
		r = PointerHash_record1_(self, x->k);
		PointerHashRecord_swapWith_(x, r);
		if(x->k == 0x0) { self->keyCount ++; return; }
		 
		r = PointerHash_record2_(self, x->k);
		PointerHashRecord_swapWith_(x, r);
		if(x->k == 0x0) { self->keyCount ++; return; }
	}
	
	PointerHash_grow(self); 
	PointerHash_at_put_(self, x->k, x->v);
}

IOINLINE void PointerHash_insertRecords(PointerHash *self, unsigned char *oldRecords, size_t oldSize)
{
	size_t i;
	
	for (i = 0; i < oldSize; i ++)
	{
		PointerHashRecord *r = PointerHashRecords_recordAt_(oldRecords, i);
		
		if (r->k)
		{
			PointerHash_at_put_(self, r->k, r->v);
		}
	}
}

IOINLINE void PointerHash_resizeTo_(PointerHash *self, size_t newSize)
{
	unsigned char *oldRecords = self->records;
	size_t oldSize = self->size;
	self->size = newSize;
	self->records = (unsigned char *) io_calloc(1, sizeof(PointerHashRecord) * self->size);
	self->keyCount = 0;
	PointerHash_updateMask(self);
	PointerHash_insertRecords(self, oldRecords, oldSize);
	io_free(oldRecords);
}

IOINLINE void PointerHash_grow(PointerHash *self)
{
	PointerHash_resizeTo_(self, self->size * 2);
}

IOINLINE void PointerHash_shrink(PointerHash *self)
{
	PointerHash_resizeTo_(self, self->size / 2);
}

IOINLINE void PointerHash_removeKey_(PointerHash *self, void *k)
{
	PointerHashRecord *r;
	
	r = PointerHash_record1_(self, k);	
	if(r->k == k)
	{
		r->k = 0x0;
		r->v = 0x0;
		self->keyCount --;
		PointerHash_shrinkIfNeeded(self);
		return;
	}
	
	r = PointerHash_record2_(self, k);
	if(r->k == k)
	{
		r->k = 0x0;
		r->v = 0x0;
		self->keyCount --;
		PointerHash_shrinkIfNeeded(self);
		return;
	}
}

IOINLINE size_t PointerHash_size(PointerHash *self) // actually the keyCount
{
	return self->keyCount;
}

// ----------------------------

IOINLINE size_t PointerHash_memorySize(PointerHash *self)
{
	return sizeof(PointerHash) + self->size * sizeof(PointerHashRecord);
}

void PointerHash_compact(PointerHash *self)
{
}


// --- enumeration --------------------------------------------------

#define POINTERHASH_FOREACH(self, pkey, pvalue, code) \
{\
	PointerHash *_self = (self);\
	unsigned char *_records = _self->records;\
	unsigned int _i, _size = _self->size;\
	void *pkey;\
	void *pvalue;\
	\
	for (_i = 0; _i < _size; _i ++)\
	{\
		PointerHashRecord *_record = PointerHashRecords_recordAt_(_records, _i);\
		if (_record->k)\
		{\
			pkey = _record->k;\
			pvalue = _record->v;\
			code;\
		}\
	}\
}


#define PointerHash_cleanSlots(self)
#define PointerHash_hasDirtyKey_(self, k) 0

#ifdef __cplusplus
}
#endif
#endif
