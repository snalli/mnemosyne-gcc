#ifndef _MNEMOSYNE_HEAP_HEAP_HH
#define _MNEMOSYNE_HEAP_HEAP_HH

#include <alps/layers/pointer.hh>
#include <alps/layers/slabheap.hh>
#include <alps/layers/extentheap.hh>
#include <alps/layers/hybridheap.hh>

#include <mnemosyne.h>
#include <mtm.h>
#include <mtm_i.h>
#include <itm.h>

extern "C" void _ITM_nl_load_bytes(const void *src, void *dest, size_t size);
extern "C" void _ITM_nl_store_bytes(const void *src, void *dest, size_t size);

class Context {
public:
    Context(bool _do_v = true, bool _do_nv = true)
        : do_v(_do_v),
          do_nv(_do_nv)
    { 
        if (_ITM_inTransaction) {
            td = _ITM_getTransaction();
        } else {
            td = NULL;
        }
    }

    void load(uint8_t* src, uint8_t *dest, size_t size)
    {
        if (td) {
            _ITM_nl_load_bytes(src, dest, size);
        } else {
            memcpy(dest, src, size);
        }
    }

    void store(uint8_t* src, uint8_t *dest, size_t size)
    {
        if (td) {
            _ITM_nl_store_bytes(src, dest, size);
        } else {
            memcpy(dest, src, size);
        }
    }

    bool do_v;
    bool do_nv;

    _ITM_transaction * td;
};

typedef alps::SlabHeap<Context, alps::TPtr, alps::PPtr> SlabHeap_t;
typedef alps::ExtentHeap<Context, alps::TPtr, alps::PPtr> ExtentHeap_t;
typedef alps::HybridHeap<Context, alps::TPtr, alps::PPtr, SlabHeap_t, ExtentHeap_t> HybridHeap_t;

class ThreadHeap
{
public:
    ThreadHeap(HybridHeap_t* hheap)
        : hheap_(hheap)
    { }

    void* pmalloc(size_t sz);
    void pmalloc_undo(void* ptr);
    void pfree_prepare(void* ptr);
    void pfree_commit(void* ptr);
    size_t getsize(void* ptr);

private:
    HybridHeap_t* hheap_;
};

class Heap {
public:

    int init();
    ThreadHeap* threadheap();

private:
    ExtentHeap_t* exheap_;
    SlabHeap_t* slheap_;
    size_t bigsize_;
};

#endif // _MNEMOSYNE_HEAP_HEAP_HH
