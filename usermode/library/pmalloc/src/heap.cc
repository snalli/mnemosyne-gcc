#include "heap.hh"

#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <mnemosyne.h>


//MNEMOSYNE_PERSISTENT void *psegment = 0;
//_enum {PERSISTENTHEAP_BASE = 0xb00000000};


//MNEMOSYNE_PERSISTENT void* PREGION_BASE = 0;
//__attribute__ ((section("PERSISTENT"))) void* PREGION_BASE = 0;
static void* PREGION_BASE = 0;

int Heap::init()
{
    Context ctx;
    size_t region_size = 1024*1024;
    size_t block_log2size = 13;
    size_t slabsize = 1 << block_log2size;
    size_t bigsize = slabsize;

    PREGION_BASE = (void*) m_pmap((void *) PREGION_BASE, region_size, PROT_READ|PROT_WRITE, 0);

    void* region = PREGION_BASE;

    exheap_ = ExtentHeap_t::make(region, region_size, block_log2size);

    slheap_ = new SlabHeap_t(slabsize, NULL, exheap_);
    slheap_->init(ctx);

    hheap_ = new HybridHeap_t(bigsize, slheap_, exheap_);
}



void* Heap::pmalloc(size_t sz)
{
    Context ctx(true, true);
    
    alps::TPtr<void> ptr;
    alps::ErrorCode rc = hheap_->malloc(ctx, sz, &ptr);
    if (rc != alps::kErrorCodeOk) {
        return NULL;
    }
    return ptr.get();
}

void Heap::pmalloc_undo(void* ptr) 
{
    Context ctx(true, false);
    
    hheap_->free(ctx, ptr);
}

void Heap::pfree_prepare(void* ptr) 
{
    Context ctx(false, true);
    
    hheap_->free(ctx, ptr);
}

void Heap::pfree_commit(void* ptr) 
{
    Context ctx(true, false);
    
    hheap_->free(ctx, ptr);
}
