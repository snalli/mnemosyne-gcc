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

# include <atomic_ops.h>

typedef AO_t atomic_t;

# ifdef NO_AO
/* Use only for testing purposes (single thread benchmarks) */
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
# else /* ! NO_AO */
#  define ATOMIC_CAS_FULL(a, e, v)      (AO_compare_and_swap_full((volatile AO_t *)(a), (AO_t)(e), (AO_t)(v)))
#  define ATOMIC_FETCH_INC_FULL(a)      (AO_fetch_and_add1_full((volatile AO_t *)(a)))
#  define ATOMIC_FETCH_DEC_FULL(a)      (AO_fetch_and_sub1_full((volatile AO_t *)(a)))
#  define ATOMIC_FETCH_ADD_FULL(a, v)   (AO_fetch_and_add_full((volatile AO_t *)(a), (AO_t)(v)))
#  ifdef SAFE
#   define ATOMIC_LOAD_ACQ(a)           (AO_load_full((volatile AO_t *)(a)))
#   define ATOMIC_LOAD(a)               (AO_load_full((volatile AO_t *)(a)))
#   define ATOMIC_STORE_REL(a, v)       (AO_store_full((volatile AO_t *)(a), (AO_t)(v)))
#   define ATOMIC_STORE(a, v)           (AO_store_full((volatile AO_t *)(a), (AO_t)(v)))
#   define ATOMIC_MB_READ               AO_nop_full()
#   define ATOMIC_MB_WRITE              AO_nop_full()
#   define ATOMIC_MB_FULL               AO_nop_full()
#  else /* ! SAFE */
#   define ATOMIC_LOAD_ACQ(a)           (AO_load_acquire_read((volatile AO_t *)(a)))
#   define ATOMIC_LOAD(a)               (*((volatile AO_t *)(a)))
#   define ATOMIC_STORE_REL(a, v)       (AO_store_release((volatile AO_t *)(a), (AO_t)(v)))
#   define ATOMIC_STORE(a, v)           (*((volatile AO_t *)(a)) = (AO_t)(v))
#   define ATOMIC_MB_READ               AO_nop_read()
#   define ATOMIC_MB_WRITE              AO_nop_write()
#   define ATOMIC_MB_FULL               AO_nop_full()
#  endif /* ! SAFE */
# endif /* ! NO_AO */

// 1
#endif /* _ATOMIC_H_ */
