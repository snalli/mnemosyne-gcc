//metadoc PointerSetHash copyright Steve Dekorte 2002, Marc Fauconneau 2007
//metadoc PointerSetHash license BSD revised
/*metadoc PointerSetHash description
	PointerSetHash - Cuckoo Hash
	keys and values are references (they are not copied or freed)
	key pointers are assumed unique
*/

#ifdef POINTERHASH_C
#define IO_IN_C_FILE
#error
#endif
#include "Common_inline.h"
#ifdef IO_DECLARE_INLINES

#define PointerSetHashRecords_recordAt_(records, pos) (PointerSetHashRecord *)(records + (pos * sizeof(PointerSetHashRecord)))

/*
IOINLINE unsigned int PointerSetHash_hash(PointerSetHash *self, void *key)
{
	intptr_t k = (intptr_t)PointerSetHashKey_value(key);
	return k^(k>>4);
}

IOINLINE unsigned int PointerSetHash_hash_more(PointerSetHash *self, unsigned int hash)
{
	return hash ^ (hash >> self->log2tableSize);
}
*/

// -----------------------------------

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
	PM_MEMSET(self->records, 0, sizeof(PointerSetHashRecord) * self->size);
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

#undef IO_IN_C_FILE
#endif
