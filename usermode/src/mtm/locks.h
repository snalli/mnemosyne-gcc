#ifndef _LOCKS_H
#define _LOCKS_H

/* ################################################################### *
 * LOCKS
 * ################################################################### */

/*
 * A lock is a unsigned int of the size of a pointer.
 * The LSB is the lock bit. If it is set, this means:
 * - At least some covered memory addresses is being written.
 * - Write-back (ETL): all bits of the lock apart from the lock bit form
 *   a pointer that points to the write log entry holding the new
 *   value. Multiple values covered by the same log entry and orginized
 *   in a linked list in the write log.
 * - Write-through and write-back (CTL): all bits of the lock apart from
 *   the lock bit form a pointer that points to the transaction
 *   descriptor containing the write-set.
 * If the lock bit is not set, then:
 * - All covered memory addresses contain consistent values.
 * - Write-back (ETL and CTL): all bits of the lock besides the lock bit
 *   contain a version number (timestamp).
 * - Write-through: all bits of the lock besides the lock bit contain a
 *   version number.
 *   - The high order bits contain the commit time.
 *   - The low order bits contain an incarnation number (incremented
 *     upon abort while writing the covered memory addresses).
 * When using the PRIORITY contention manager, the format of locks is
 * slightly different. It is documented elsewhere.
 */

#define OWNED_MASK                      0x01                /* 1 bit */
#if CM == CM_PRIORITY
# define WAIT_MASK                      0x02                /* 1 bit */
# define PRIORITY_BITS                  3                   /* 3 bits */
# define PRIORITY_MAX                   ((1 << PRIORITY_BITS) - 1)
# define PRIORITY_MASK                  (PRIORITY_MAX << 2)
# define ALIGNMENT                      (1 << (PRIORITY_BITS + 2))
# define ALIGNMENT_MASK                 (ALIGNMENT - 1)
#endif /* CM == CM_PRIORITY */

#define LOCK_GET_OWNED(l)               (l & OWNED_MASK)
#if CM == CM_PRIORITY
# define LOCK_SET_ADDR(a, p)            (a | (p << 2) | OWNED_MASK)
# define LOCK_GET_WAIT(l)               (l & WAIT_MASK)
# define LOCK_GET_PRIORITY(l)           ((l & PRIORITY_MASK) >> 2)
# define LOCK_SET_PRIORITY_WAIT(l, p)   ((l & ~(mtm_word_t)PRIORITY_MASK) | (p << 2) | WAIT_MASK)
# define LOCK_GET_ADDR(l)               (l & ~(mtm_word_t)(OWNED_MASK | WAIT_MASK | PRIORITY_MASK))
#else /* CM != CM_PRIORITY */
# define LOCK_SET_ADDR(a)               (a | OWNED_MASK)    /* OWNED bit set */
# define LOCK_GET_ADDR(l)               (l & ~(mtm_word_t)OWNED_MASK)
#endif /* CM != CM_PRIORITY */
#define LOCK_UNIT                       (~(mtm_word_t)0)

/*
 * We use the very same hash functions as TL2 for degenerate Bloom
 * filters on 32 bits.
 */
#ifdef USE_BLOOM_FILTER
# define FILTER_HASH(a)                 (((mtm_word_t)a >> 2) ^ ((mtm_word_t)a >> 5))
# define FILTER_BITS(a)                 (1 << (FILTER_HASH(a) & 0x1F))
#endif /* USE_BLOOM_FILTER */

/*
 * We use an array of locks and hash the address to find the location of the lock.
 * We try to avoid collisions as much as possible (two addresses covered by the same lock).
 */
#define LOCK_ARRAY_SIZE                 (1 << LOCK_ARRAY_LOG_SIZE)
#define LOCK_MASK                       (LOCK_ARRAY_SIZE - 1)
#define LOCK_SHIFT                      (((sizeof(mtm_word_t) == 4) ? 2 : 3) + LOCK_SHIFT_EXTRA)
#define LOCK_IDX(a)                     (((mtm_word_t)(a) >> LOCK_SHIFT) & LOCK_MASK)
#ifdef LOCK_IDX_SWAP
# if LOCK_ARRAY_LOG_SIZE < 16
#  error "LOCK_IDX_SWAP requires LOCK_ARRAY_LOG_SIZE to be at least 16"
# endif /* LOCK_ARRAY_LOG_SIZE < 16 */
# define GET_LOCK(a)                    (locks + lock_idx_swap(LOCK_IDX(a)))
#else /* ! LOCK_IDX_SWAP */
# define GET_LOCK(a)                    (locks + LOCK_IDX(a))
#endif /* ! LOCK_IDX_SWAP */


#endif
