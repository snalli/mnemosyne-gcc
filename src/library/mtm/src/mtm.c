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

/**
 * \file mtm.c
 *
 * \brief Implements global utilities which are required by the transactional 
 * functions of MTM.
 *
 */
#include <mtm_i.h>

#ifdef TLS
__thread mtm_tx_t* _mtm_thread_tx;
#else /* ! TLS */
pthread_key_t _mtm_thread_tx;
#endif /* ! TLS */

volatile mtm_word_t locks[LOCK_ARRAY_SIZE];

#ifdef CLOCK_IN_CACHE_LINE
/* At least twice a cache line (512 bytes to be on the safe side) */
volatile mtm_word_t gclock[1024 / sizeof(mtm_word_t)];
#else /* ! CLOCK_IN_CACHE_LINE */
volatile mtm_word_t gclock;
#endif /* ! CLOCK_IN_CACHE_LINE */


int vr_threshold;
int cm_threshold;

mtm_rwlock_t mtm_serial_lock;
