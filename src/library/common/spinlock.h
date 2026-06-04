/*
 * Portable ticket-lock implementation.
 *
 * Original x86 inline asm (xaddw / incb) replaced with GCC __atomic
 * builtins. Uses two separate uint16_t counters so the unlock increment
 * never carries into the tail field — the original packed 32-bit design
 * had a deadlock when the head byte wrapped through 0xFF.
 *
 * The public API (arch_spinlock_t, __ticket_spin_lock, __ticket_spin_unlock)
 * is preserved so callers need no changes.
 */

#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include <stdint.h>

typedef struct arch_spinlock {
    uint16_t head; /* currently-serving ticket */
    uint16_t tail; /* next ticket to hand out  */
} arch_spinlock_t;

static __attribute__((always_inline)) inline void
__ticket_spin_lock(volatile arch_spinlock_t *lock)
{
    /* Claim a ticket by incrementing the tail. */
    uint16_t ticket = __atomic_fetch_add(&lock->tail, 1u, __ATOMIC_SEQ_CST);

    /* Spin until head reaches our ticket. */
    while (__atomic_load_n(&lock->head, __ATOMIC_ACQUIRE) != ticket)
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

static __attribute__((always_inline)) inline void
__ticket_spin_unlock(volatile arch_spinlock_t *lock)
{
    /* Advance head — only touches head, never tail. */
    uint16_t h = __atomic_load_n(&lock->head, __ATOMIC_RELAXED);
    __atomic_store_n(&lock->head, (uint16_t)(h + 1u), __ATOMIC_RELEASE);
}

#endif /* _SPINLOCK_H */
