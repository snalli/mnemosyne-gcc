/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

/*
 * Source code is partially derived from TinySTM (license is attached)
 *
 *
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * Copyright (c) 2007-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


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
#else
# define ALIGNMENT                      1 /* no alignment requirement */  
#endif /* CM == CM_PRIORITY */
#define ALIGNMENT_MASK                 (ALIGNMENT - 1)

/*
 * Everything hereafter relates to the actual locking protection mechanism.
 */
#define LOCK_GET_OWNED(l)               ((l) & OWNED_MASK)
#if CM == CM_PRIORITY
# define LOCK_SET_ADDR(a, p)            ((a) | ((p) << 2) | OWNED_MASK)
# define LOCK_GET_WAIT(l)               ((l) & WAIT_MASK)
# define LOCK_GET_PRIORITY(l)           (((l) & PRIORITY_MASK) >> 2)
# define LOCK_SET_PRIORITY_WAIT(l, p)   (((l) & ~(mtm_word_t)PRIORITY_MASK) | ((p) << 2) | WAIT_MASK)
# define LOCK_GET_ADDR(l)               ((l) & ~(mtm_word_t)(OWNED_MASK | WAIT_MASK | PRIORITY_MASK))
#else /* CM != CM_PRIORITY */
# define LOCK_SET_ADDR(a)               ((a) | OWNED_MASK)    /* OWNED bit set */
# define LOCK_GET_ADDR(l)               ((l) & ~(mtm_word_t)OWNED_MASK)
#endif /* CM != CM_PRIORITY */
#define LOCK_UNIT                       (~(mtm_word_t)0)

/*
 * We use an array of locks and hash the address to find the location of the lock.
 * We try to avoid collisions as much as possible (two addresses covered by the same lock).
 */
#define LOCK_ARRAY_SIZE                 (1 << LOCK_ARRAY_LOG_SIZE)
#define LOCK_MASK                       (LOCK_ARRAY_SIZE - 1)
//#define LOCK_SHIFT                      (((sizeof(mtm_word_t) == 4) ? 2 : 3) + LOCK_SHIFT_EXTRA)
// Map the words of a cacheline on the same lock
#define LOCK_SHIFT                      6
#define LOCK_IDX(a)                     (((mtm_word_t)((a)) >> LOCK_SHIFT) & LOCK_MASK)
#ifdef LOCK_IDX_SWAP
# if LOCK_ARRAY_LOG_SIZE < 16
#  error "LOCK_IDX_SWAP requires LOCK_ARRAY_LOG_SIZE to be at least 16"
# endif /* LOCK_ARRAY_LOG_SIZE < 16 */
# define GET_LOCK(a)                    (locks + lock_idx_swap(LOCK_IDX((a))))
#else /* ! LOCK_IDX_SWAP */
# define GET_LOCK(a)                    (locks + LOCK_IDX((a)))
#endif /* ! LOCK_IDX_SWAP */



/* ################################################################### *
 * PRIVATE PSEUDO-LOCKS
 * ################################################################### */

/* 
 * For write-back, the original TinySTM keeps new values into linked lists
 * accessible through the global lock hash-table. 
 * 
 * For persistent write-back, when isolation is disabled, going through
 * the global lock hash-table would require fine-grain synchronization 
 * so that locks are not held for the duration of a transaction. To avoid 
 * paying the cost of such unnecessary synchronization we use a private lock 
 * hash-table instead. The structure of the private table is the same as 
 * the global one's to keep code changes minimal; its size may be different 
 * though.
 *
 * Private pseudo-locks are not actually locks but a hack to keep the code 
 * that deals with version-management common between the isolation and 
 * no-isolation path. Since never two threads may compete for these pseudo-locks, 
 * we don't need to use CAS to atomically acquire them but instead we can just 
 * set them using a normal STORE.
 */
 
#define PRIVATE_LOCK_ARRAY_SIZE         (1 << PRIVATE_LOCK_ARRAY_LOG_SIZE)
#define PRIVATE_LOCK_MASK               (PRIVATE_LOCK_ARRAY_SIZE - 1)
#define PRIVATE_LOCK_SHIFT              6
#define PRIVATE_LOCK_IDX(a)             (((mtm_word_t)((a)) >> PRIVATE_LOCK_SHIFT) & PRIVATE_LOCK_MASK)
#define PRIVATE_GET_LOCK(tx, a)         (tx->wb_table + PRIVATE_LOCK_IDX((a)))


#endif /* _LOCKS_H */
