#include "heap.hh"

#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <mnemosyne.h>


//MNEMOSYNE_PERSISTENT void *psegment = 0;
//_enum {PERSISTENTHEAP_BASE = 0xb00000000};


//MNEMOSYNE_PERSISTENT void* PREGION_BASE = 0;
__attribute__ ((section("PERSISTENT"))) void* PREGION_BASE = 0;

int Heap::init()
{
    Context ctx;
    /* Clean up multiple definitions of PSEGMENT_* */
    /*
     * Do not combine n_gb and region_size in the defn
     * of region_size. it causes compiler to generate wrong
     * (my guess), which can actually be observed as an overflow 
     * during runtime.
     * splitting the values as n_gb and region_size does not
     * cause any absurd overflows during runtime.
     */
    unsigned long long n_gb = 8;
    unsigned long long region_size = 1024*1024*1024;
    region_size *= n_gb; 
    size_t block_log2size = 13;
    size_t slabsize = 1 << block_log2size;
    
    bigsize_ = slabsize;

    if (PREGION_BASE == 0) {
        PREGION_BASE = (void*) m_pmap((void *) PREGION_BASE, region_size, PROT_READ|PROT_WRITE, 0);
        void* region = PREGION_BASE;
        exheap_ = ExtentHeap_t::make(region, region_size, block_log2size);
    } else {
        void* region = PREGION_BASE;
        exheap_ = ExtentHeap_t::load(region);
    }

    slheap_ = new SlabHeap_t(slabsize, NULL, exheap_);
    slheap_->init(ctx);
}

ThreadHeap* Heap::threadheap()
{
    HybridHeap_t* hheap = new HybridHeap_t(bigsize_, slheap_, exheap_);
    ThreadHeap* thp = new ThreadHeap(hheap);
    return thp;
}

void* ThreadHeap::pmalloc(size_t sz)
{
    Context ctx(true, true);
    
    alps::TPtr<void> ptr;
    alps::ErrorCode rc = hheap_->malloc(ctx, sz, &ptr);
    if (rc != alps::kErrorCodeOk) {
        return NULL;
    }
    return ptr.get();
}

void ThreadHeap::pmalloc_undo(void* ptr) 
{
    Context ctx(true, false);
    
    hheap_->free(ctx, ptr);
}

void ThreadHeap::pfree_prepare(void* ptr) 
{
    Context ctx(false, true);
    
    hheap_->free(ctx, ptr);
}

void ThreadHeap::pfree_commit(void* ptr) 
{
    Context ctx(true, false);
    
    hheap_->free(ctx, ptr);
}


size_t ThreadHeap::getsize(void* ptr)
{
    Context ctx(true, true);

    return hheap_->getsize(ptr);
}
