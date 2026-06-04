#ifndef _MNEMOSYNE_PERSISTENCY_
#define _MNEMOSYNE_PERSISTENCY_

#include "pvar.h"
#include <pmalloc.h>

// We have to do this because pmalloc.h defines these as macros
#undef pmalloc
#undef pfree

namespace mnemosyne {


/*
 * <h1> Wrapper for Mnemosyne </h1>
 *
 * Mnemosyne was taken from here
 * https://github.com/snalli/mnemosyne-gcc
 *
 */
class Mnemosyne {
public:
    Mnemosyne()  { }

    ~Mnemosyne() { }


    static std::string className() { return "Mnemosyne"; }


    template <typename T>
    inline T* get_object(int idx) {
        return nullptr;
        //return (T*)per->objects[idx];  // TODO: fix me
    }

    template <typename T>
    inline void put_object(int idx, T* obj) {
        //per->objects[idx] = obj;  // TODO: fix me
        //PWB(&per->objects[idx]);
    }


    inline void begin_transaction() {
    }

    inline void end_transaction() {
    }

    inline void recover_if_needed() {
    }

    inline void abort_transaction(void) {
    }


    // Same as begin/end transaction, but with a lambda.
    // Calling abort_transaction() from within the lambda is not allowed.
    template<typename R, class F>
    R transaction(F&& func) {
        R retval;
        PTx { retval = func(); }
        return retval;
    }

    template<class F>
    void transaction(F&& func) {
        PTx { func(); }
    }

    template<class F>
    void write_transaction(F&& func) {
        PTx { func(); }
    }

    template<typename R, class F>
    R write_transaction(F&& func) {
        R retval;
        PTx { retval = func(); }
        return retval;
    }

    template<class F>
    void read_transaction(F&& func) {
        PTx { func(); }
    }

    template<typename R, class F>
    R read_transaction(F&& func) {
        R retval;
        PTx { func(); }
        return retval;
    }

    /*
     * Allocator
     * Must be called from within a Mnemosyne transaction PTx {}
     */
    template <typename T, typename... Args>
    T* alloc(Args&&... args) {
        void *addr = nullptr;
        addr = ::_ITM_pmalloc(sizeof(T));
        return new (addr) T(std::forward<Args>(args)...); // placement new
    }


    /*
     * De-allocator
     * Must be called from within a Mnemosyne transaction PTx {}
     */
    template<typename T>
    void free(T* obj) {
        if (obj == nullptr) return;
        obj->~T();
        //::_ITM_pfree(obj);
    }

    /* Allocator for C methods (like memcached) */
    void* pmalloc(size_t size) {
        void* ptr = nullptr;
        ptr = ::_ITM_pmalloc(size);
        return ptr;
    }


    /* De-allocator for C methods (like memcached) */
    void pfree(void* ptr) {
        //::_ITM_pfree(ptr);
    }

};



} // end of mnemosyne namespace

#endif   // _MNEMOSYNE_PERSISTENCY_
