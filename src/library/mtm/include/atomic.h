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
 * File:
 *   atomic.h
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 * Description:
 *   Atomic operations.
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

#ifndef _ATOMIC_H_
# define _ATOMIC_H_

/* Portable atomics via GCC __atomic builtins — no libatomic-ops dependency */
# include <stdint.h>

typedef uintptr_t atomic_t;

/* Helper: CAS returns 1 on success (matches the AO_compare_and_swap_full contract) */
static inline int _mtm_cas(volatile atomic_t *a, atomic_t expected, atomic_t desired) {
    return __atomic_compare_exchange_n(a, &expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

# ifdef NO_AO
#  define ATOMIC_CAS_FULL(a, e, v)      (*(a) = (v), 1)
#  define ATOMIC_FETCH_INC_FULL(a)      ((*(a))++)
#  define ATOMIC_FETCH_DEC_FULL(a)      ((*(a))--)
#  define ATOMIC_FETCH_ADD_FULL(a, v)   ((*(a)) += (v))
#  define ATOMIC_LOAD_ACQ(a)            (*(a))
#  define ATOMIC_LOAD(a)                (*(a))
#  define ATOMIC_STORE_REL(a, v)        (*(a) = (v))
#  define ATOMIC_STORE(a, v)            (*(a) = (v))
#  define ATOMIC_MB_READ                /* Nothing */
#  define ATOMIC_MB_WRITE               /* Nothing */
#  define ATOMIC_MB_FULL                /* Nothing */
# else
#  define ATOMIC_CAS_FULL(a, e, v)      _mtm_cas((volatile atomic_t *)(a), (atomic_t)(e), (atomic_t)(v))
#  define ATOMIC_FETCH_INC_FULL(a)      __atomic_fetch_add((volatile atomic_t *)(a), 1,    __ATOMIC_SEQ_CST)
#  define ATOMIC_FETCH_DEC_FULL(a)      __atomic_fetch_sub((volatile atomic_t *)(a), 1,    __ATOMIC_SEQ_CST)
#  define ATOMIC_FETCH_ADD_FULL(a, v)   __atomic_fetch_add((volatile atomic_t *)(a), (v),  __ATOMIC_SEQ_CST)
#  ifdef SAFE
#   define ATOMIC_LOAD_ACQ(a)           __atomic_load_n((volatile atomic_t *)(a), __ATOMIC_SEQ_CST)
#   define ATOMIC_LOAD(a)               __atomic_load_n((volatile atomic_t *)(a), __ATOMIC_SEQ_CST)
#   define ATOMIC_STORE_REL(a, v)       __atomic_store_n((volatile atomic_t *)(a), (atomic_t)(v), __ATOMIC_SEQ_CST)
#   define ATOMIC_STORE(a, v)           __atomic_store_n((volatile atomic_t *)(a), (atomic_t)(v), __ATOMIC_SEQ_CST)
#   define ATOMIC_MB_READ               __atomic_thread_fence(__ATOMIC_SEQ_CST)
#   define ATOMIC_MB_WRITE              __atomic_thread_fence(__ATOMIC_SEQ_CST)
#   define ATOMIC_MB_FULL               __atomic_thread_fence(__ATOMIC_SEQ_CST)
#  else
#   define ATOMIC_LOAD_ACQ(a)           __atomic_load_n((volatile atomic_t *)(a), __ATOMIC_ACQUIRE)
#   define ATOMIC_LOAD(a)               __atomic_load_n((volatile atomic_t *)(a), __ATOMIC_RELAXED)
#   define ATOMIC_STORE_REL(a, v)       __atomic_store_n((volatile atomic_t *)(a), (atomic_t)(v), __ATOMIC_RELEASE)
#   define ATOMIC_STORE(a, v)           __atomic_store_n((volatile atomic_t *)(a), (atomic_t)(v), __ATOMIC_RELAXED)
#   define ATOMIC_MB_READ               __atomic_thread_fence(__ATOMIC_ACQUIRE)
#   define ATOMIC_MB_WRITE              __atomic_thread_fence(__ATOMIC_RELEASE)
#   define ATOMIC_MB_FULL               __atomic_thread_fence(__ATOMIC_SEQ_CST)
#  endif
# endif

// 1
#endif /* _ATOMIC_H_ */
