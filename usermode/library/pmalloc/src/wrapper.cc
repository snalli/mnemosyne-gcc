#include <stdlib.h>
#include <string.h>

#include <mutex>

#include "heap.hh"

#include <mtm_i.h>
#include <itm.h>

thread_local ThreadHeap* threadheap = NULL;
static Heap* heap;
std::mutex heapmtx;

inline static Heap * getHeap (void) 
{
    heapmtx.lock();
    if (heap) {
        heapmtx.unlock();
        return heap;
    }
    heap = new Heap();
    heap->init();
    heapmtx.unlock();
    return heap;
}

inline static ThreadHeap* getThreadHeap (void)
{
    if (threadheap) {
        return threadheap;
    }
    Heap* heap = getHeap();
    return heap->threadheap();
}

extern "C"
void * mtm_pmalloc (size_t sz)
{
    void* addr;

    ThreadHeap* heap = getThreadHeap();
    addr = heap->pmalloc(sz);
    
	return addr;
}

extern "C"
void mtm_pmalloc_undo (void* ptr)
{
    ThreadHeap* heap = getThreadHeap();
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
    ThreadHeap* heap = getThreadHeap();
    heap->pfree_prepare(ptr);
}

extern "C"
void mtm_pfree_commit (void* ptr)
{
    ThreadHeap* heap = getThreadHeap();
    heap->pfree_commit(ptr);
}

extern "C"
size_t mtm_get_obj_size(void *ptr)
{
    ThreadHeap* heap = getThreadHeap();
    return heap->getsize(ptr);
}

extern "C" void * mtm_prealloc (void * ptr, size_t sz)
{
    //TODO

	return NULL;
}
