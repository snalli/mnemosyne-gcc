/*
 * Portable spinlock implementation.
 *
 * Original x86 ticket-lock using inline asm (xaddw / incb) replaced with
 * GCC __atomic builtins so the code compiles on any architecture.
 *
 * The public API surface (arch_spinlock_t, __ticket_spin_lock,
 * __ticket_spin_unlock) is preserved unchanged so callers need no edits.
 */

#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include <stdint.h>

/*
 * Ticket-lock implemented with two 16-bit counters packed into a 32-bit word.
 *   bits  7:0  – current serving ticket (head)
 *   bits 15:8  – next ticket to hand out (tail)
 */
#define TICKET_SHIFT 8

typedef struct arch_spinlock {
    uint32_t slock;
} arch_spinlock_t;

static __attribute__((always_inline)) inline void
__ticket_spin_lock(volatile arch_spinlock_t *lock)
{
    /* Atomically increment the tail (high byte) and retrieve the old value.
     * The low 16 bits of the increment add 0x100 to bump only the tail. */
    uint32_t inc  = 0x00000100u;
    uint32_t old  = __atomic_fetch_add(&lock->slock, inc, __ATOMIC_SEQ_CST);
    uint16_t ticket = (uint16_t)(old >> TICKET_SHIFT); /* our ticket */

    /* Spin until the head (low byte) matches our ticket */
    while ((uint8_t)(ticket) != (uint8_t)(__atomic_load_n(&lock->slock, __ATOMIC_ACQUIRE) & 0xFFu)) {
        /* Compiler barrier + yield hint — portable across all GCC targets */
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
}

static __attribute__((always_inline)) inline void
__ticket_spin_unlock(volatile arch_spinlock_t *lock)
{
    /* Increment the serving counter (head, low byte) */
    __atomic_fetch_add(&lock->slock, 1u, __ATOMIC_RELEASE);
}

#endif /* _SPINLOCK_H */
