#include <stdlib.h>
#include <string.h>

#include "heap.hh"

#include <mtm_i.h>
#include <itm.h>


static Heap* heap;

inline static Heap * getHeap (void) {
    if (heap) {
        return heap;
    }
    heap = new Heap();
    heap->init();
    return heap;
}


extern "C"
void * mtm_pmalloc (size_t sz)
{
    void* addr;

    Heap* heap = getHeap();
    addr = heap->pmalloc(sz);
    
	return addr;
}

extern "C"
void mtm_pmalloc_undo (void* ptr)
{
    Heap* heap = getHeap();
    heap->pmalloc_undo(ptr);
}


extern "C"
void * mtm_pcalloc (size_t nelem, size_t elsize)
{
    void* addr;
    // TODO

	return addr;
}


extern "C"
void mtm_pfree (void* ptr)
{
    // TODO
    // Do we need this? 
    // pfree like pmalloc shall only be called inside a transaction to ensure 
    // failure atomicity
}

extern "C"
void mtm_pfree_prepare (void* ptr)
{
    Heap* heap = getHeap();
    heap->pfree_prepare(ptr);
}

extern "C"
void mtm_pfree_commit (void* ptr)
{
    Heap* heap = getHeap();
    heap->pfree_commit(ptr);
}

extern "C"
size_t mtm_get_obj_size(void *ptr)
{
    //TODO

	size_t objSize = -1;
	
	return objSize;
}

extern "C" void * mtm_prealloc (void * ptr, size_t sz)
{
    //TODO

	return NULL;
}
