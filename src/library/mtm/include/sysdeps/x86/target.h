/*
    Copyright (C) 2011 Computer Sciences Department,
    University of Wisconsin -- Madison

    This file is part of Mnemosyne: Lightweight Persistent Memory.

    Portable replacement for the x86-specific target.h.
    x86 inline asm replaced with compiler built-ins / standard C.
*/

#ifndef TARGET_H_HBX9MKK6
#define TARGET_H_HBX9MKK6

#include <stdint.h>

/* Cache-line size: use 64 bytes everywhere (safe for all modern CPUs). */
#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif
#ifndef CACHELINE_SIZE_LOG
#define CACHELINE_SIZE_LOG 6
#endif

/* -----------------------------------------------------------------------
 * mtm_jmpbuf_t
 * The layout below matches the original x86-64 definition so the rest of
 * the MTM code that accesses individual fields continues to compile.
 * On non-x86 targets the GPR fields (rbx, rbp, r12-r15) are unused; they
 * are captured / restored via the normal C calling convention instead.
 * ----------------------------------------------------------------------- */
typedef struct mtm_jmpbuf_s mtm_jmpbuf_t;

struct mtm_jmpbuf_s {
    uint64_t sp;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t abendPC;
    uint32_t mxcsr;
    uint32_t txn_flags;
    uint16_t fpcsr;
};

/* -----------------------------------------------------------------------
 * Barrier / relax helpers (from GCC's libitm)
 * ----------------------------------------------------------------------- */

/* Hint to the CPU that we are in a spin-wait loop */
static inline void cpu_relax(void) {
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

static inline void atomic_read_barrier(void) {
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

static inline void atomic_write_barrier(void) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

/* -----------------------------------------------------------------------
 * Stack / frame pointer helpers
 * Implemented via GCC built-ins so they work on every architecture.
 * ----------------------------------------------------------------------- */

static inline uintptr_t *get_stack_pointer(void) {
    return (uintptr_t *)__builtin_frame_address(0);
}

static inline uintptr_t *get_frame_pointer(void) {
    return (uintptr_t *)(uintptr_t)__builtin_frame_address(0);
}

/* Walk the frame-pointer chain to find the stack base.
 * Requires -fno-omit-frame-pointer (already set in mnemosyne_iface). */
static inline uintptr_t get_stack_base(void) {
    uintptr_t fp = (uintptr_t)__builtin_frame_address(0);
    uintptr_t base = fp;
    while (fp) {
        base = fp;
        uintptr_t next = *(uintptr_t *)fp;
        if (next <= fp)
            break;
        fp = next;
    }
    return base;
}

#endif /* TARGET_H_HBX9MKK6 */
