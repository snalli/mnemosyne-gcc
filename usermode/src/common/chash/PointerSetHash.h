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

#ifdef __cplusplus
extern "C" {
#endif

#define POINTERHASH_MAXLOOP 10

#include "PointerSetHash_struct.h"

BASEKIT_API PointerSetHash *PointerSetHash_new(void);
BASEKIT_API void PointerSetHash_copy_(PointerSetHash *self, const PointerSetHash *other);
BASEKIT_API PointerSetHash *PointerSetHash_clone(PointerSetHash *self);
BASEKIT_API void PointerSetHash_free(PointerSetHash *self);

BASEKIT_API void PointerSetHash_at_put_(PointerSetHash *self, void *k);
BASEKIT_API void PointerSetHash_removeKey_(PointerSetHash *self, void *k);
BASEKIT_API size_t PointerSetHash_size(PointerSetHash *self); // actually the keyCount

BASEKIT_API size_t PointerSetHash_memorySize(PointerSetHash *self);
BASEKIT_API void PointerSetHash_compact(PointerSetHash *self);

// --- private methods ----------------------------------------

BASEKIT_API void PointerSetHash_setSize_(PointerSetHash *self, size_t size); 
BASEKIT_API void PointerSetHash_insert_(PointerSetHash *self, PointerSetHashRecord *x); 
BASEKIT_API void PointerSetHash_grow(PointerSetHash *self); 
BASEKIT_API void PointerSetHash_shrinkIfNeeded(PointerSetHash *self); 
BASEKIT_API void PointerSetHash_shrink(PointerSetHash *self); 
BASEKIT_API void PointerSetHash_show(PointerSetHash *self);
BASEKIT_API void PointerSetHash_updateMask(PointerSetHash *self); 

#include "PointerSetHash_inline.h"

#define PointerSetHash_cleanSlots(self)
#define PointerSetHash_hasDirtyKey_(self, k) 0

#ifdef __cplusplus
}
#endif
#endif
