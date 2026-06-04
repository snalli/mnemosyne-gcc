//metadoc PointerSetHash copyright Steve Dekorte 2002
//metadoc PointerSetHash license BSD revised
//metadoc PointerSetHash notes Suggestion to use cuckoo hash and original implementation by Marc Fauconneau 

#define POINTERHASH_C
#include "PointerSetHash.h"
#undef POINTERHASH_C
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


PointerSetHash *PointerSetHash_new(void)
{
	PointerSetHash *self = (PointerSetHash *)io_calloc(1, sizeof(PointerSetHash));
	PointerSetHash_setSize_(self, 8);
	return self;
}

void PointerSetHash_copy_(PointerSetHash *self, const PointerSetHash *other)
{
	io_free(self->records);
	PM_MEMCPY(self, other, sizeof(PointerSetHash));
	self->records = malloc(self->size * sizeof(PointerSetHashRecord));
	PM_MEMCPY(self->records, other->records, self->size * sizeof(PointerSetHashRecord));
}

PointerSetHash *PointerSetHash_clone(PointerSetHash *self)
{
	PointerSetHash *other = PointerSetHash_new();
	PointerSetHash_copy_(other, self);
	return other;
}

void PointerSetHash_setSize_(PointerSetHash *self, size_t size)
{
	self->records = realloc(self->records, size * sizeof(PointerSetHashRecord));
	
	if(size > self->size)
	{		
		PM_MEMSET(self->records + self->size * sizeof(PointerSetHashRecord), 
			0x0, (size - self->size) * sizeof(PointerSetHashRecord));
	}
	
	self->size = size;
	
	PointerSetHash_updateMask(self);
}

void PointerSetHash_updateMask(PointerSetHash *self)
{
	self->mask = (intptr_t)(self->size - 1);
}

void PointerSetHash_show(PointerSetHash *self)
{
	size_t i;
	
	printf("PointerSetHash records:\n");
	for(i = 0; i < self->size; i++)
	{
		PointerSetHashRecord *r = PointerSetHashRecords_recordAt_(self->records, i);
		printf("  %i: %p\n", (int)i, r->k);
	}
}

void PointerSetHash_free(PointerSetHash *self)
{
	io_free(self->records);
	io_free(self);
}

void PointerSetHash_insert_(PointerSetHash *self, PointerSetHashRecord *x)
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

void PointerSetHash_insertRecords(PointerSetHash *self, unsigned char *oldRecords, size_t oldSize)
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

void PointerSetHash_resizeTo_(PointerSetHash *self, size_t newSize)
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

void PointerSetHash_grow(PointerSetHash *self)
{
	PointerSetHash_resizeTo_(self, self->size * 2);
}

void PointerSetHash_shrink(PointerSetHash *self)
{
	PointerSetHash_resizeTo_(self, self->size / 2);
}

void PointerSetHash_removeKey_(PointerSetHash *self, void *k)
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

size_t PointerSetHash_size(PointerSetHash *self) // actually the keyCount
{
	return self->keyCount;
}

// ----------------------------

size_t PointerSetHash_memorySize(PointerSetHash *self)
{
	return sizeof(PointerSetHash) + self->size * sizeof(PointerSetHashRecord);
}

void PointerSetHash_compact(PointerSetHash *self)
{
}
