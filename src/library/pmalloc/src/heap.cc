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
    alps::DebugOptions dbgopt;
    dbgopt.log_level = "error"; // Disable logging output
    alps::init_log(dbgopt);

    Context ctx;
    /*
     * Size of the persistent heap region, in MiB. Defaults to 8 GiB, but is
     * overridable via the MNEMOSYNE_PHEAP_SIZE_MB environment variable so that
     * test / CI environments with a small /dev/shm (where the region is backed)
     * can use a modest region without exhausting tmpfs.
     */
    unsigned long long region_mb = 8ULL * 1024;   /* 8 GiB default */
    if (const char *env = getenv("MNEMOSYNE_PHEAP_SIZE_MB")) {
        unsigned long long v = strtoull(env, nullptr, 10);
        if (v > 0)
            region_mb = v;
    }
    unsigned long long region_size = region_mb * 1024ULL * 1024ULL;
    size_t block_log2size = 13;

    slabsize_ = 1 << block_log2size;
    //slabsize /= 2; 

    /* Max block allocated from slabheap must be smaller than the slab extent size 
     * to ensure slab data and metadata fit within the slab extent */
    bigsize_ = slabsize_/2;

    if (PREGION_BASE == 0) {
        PREGION_BASE = (void*) m_pmap((void *) PREGION_BASE, region_size, PROT_READ|PROT_WRITE, 0);
        void* region = PREGION_BASE;
        exheap_ = ExtentHeap_t::make(region, region_size, block_log2size);
    } else {
        void* region = PREGION_BASE;
        exheap_ = ExtentHeap_t::load(region);
    }

    slheap_ = new SlabHeap_t(slabsize_, NULL, exheap_);
    slheap_->init(ctx);
    return 0;
}

ThreadHeap* Heap::threadheap()
{
    Context ctx;

    SlabHeap_t* slheap = new SlabHeap_t(slabsize_, NULL, exheap_);
    slheap_->init(ctx);

    HybridHeap_t* hheap = new HybridHeap_t(bigsize_, slheap, exheap_);
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
