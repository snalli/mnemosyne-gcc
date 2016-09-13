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


#ifndef _CM_H
#define _CM_H

#define CM_RESTART          1
#define CM_RESTART_NO_LOAD  2
#define CM_RESTART_LOCKED   3

static inline
int 
cm_conflict(mtm_tx_t *tx, volatile mtm_word_t *lock, mtm_word_t *l)
{
	mode_data_t *modedata = (mode_data_t *) tx->modedata[tx->mode];
	w_entry_t   *w;

#if CM == CM_PRIORITY
	if (tx->retries >= cm_threshold) {
		if (LOCK_GET_PRIORITY(*l) < tx->priority ||
			(LOCK_GET_PRIORITY(*l) == tx->priority &&
			*l < (mtm_word_t)modedata->w_set.entries
			&& !LOCK_GET_WAIT(*l))) 
		{
			/* We have higher priority */
			if (ATOMIC_CAS_FULL(lock, *l, LOCK_SET_PRIORITY_WAIT(*l, tx->priority)) == 0) {
				return CM_RESTART;
			}
			*l = LOCK_SET_PRIORITY_WAIT(*l, tx->priority);
		}
		/* Wait until lock is free or another transaction waits for one of our locks */
		while (1) {
			int        nb;
			mtm_word_t lw;

			w = modedata->w_set.entries;
			for (nb = modedata->w_set.nb_entries; nb > 0; nb--, w++) {
				lw = ATOMIC_LOAD(w->lock);
				if (LOCK_GET_WAIT(lw)) {
					/* Another transaction waits for one of our locks */
					goto give_up;
				}
			}
			/* Did the lock we are waiting for get updated? */
			lw = ATOMIC_LOAD(lock);
			if (*l != lw) {
				*l = lw;
				return CM_RESTART_NO_LOAD;
			}
		}
give_up:
			if (tx->priority < PRIORITY_MAX) {
				tx->priority++;
			} else {
				PRINT_DEBUG("Reached maximum priority\n");
			}
			tx->c_lock = lock;
		}
#elif CM == CM_DELAY
		tx->c_lock = lock;
#endif /* CM == CM_DELAY */
		return CM_RESTART_LOCKED;
}


static inline
int
cm_upgrade_lock(mtm_tx_t *tx)
{
#if CM == CM_PRIORITY
	if (tx->visible_reads >= vr_threshold && vr_threshold >= 0) {
		return 1;
	} else {
		return 0;
	}
#endif /* CM == CM_PRIORITY */
	return 0;
}


static inline
void
cm_delay(mtm_tx_t *tx)
{
#if CM == CM_BACKOFF
	unsigned long wait;
	volatile int  j;

	/* Simple RNG (good enough for backoff) */
	tx->seed ^= (tx->seed << 17);
	tx->seed ^= (tx->seed >> 13);
	tx->seed ^= (tx->seed << 5);
	wait = tx->seed % tx->backoff;
	for (j = 0; j < wait; j++) {
		/* Do nothing */
	}
	if (tx->backoff < MAX_BACKOFF) {
		tx->backoff <<= 1;
	}
#endif /* CM == CM_BACKOFF */

#if CM == CM_DELAY || CM == CM_PRIORITY
	/* Wait until contented lock is free */
	if (tx->c_lock != NULL) {
		/* Busy waiting (yielding is expensive) */
		while (LOCK_GET_OWNED(ATOMIC_LOAD(tx->c_lock))) {
# ifdef WAIT_YIELD
			sched_yield();
# endif /* WAIT_YIELD */
		}
		tx->c_lock = NULL;
	}
#endif /* CM == CM_DELAY || CM == CM_PRIORITY */
}


static inline
void
cm_visible_read(mtm_tx_t *tx)
{
#if CM == CM_PRIORITY
	tx->visible_reads++;
#endif /* CM == CM_PRIORITY */
}


static inline
void
cm_reset(mtm_tx_t *tx)
{
#if CM == CM_PRIORITY || defined(INTERNAL_STATS)
	tx->retries = 0;
#endif /* CM == CM_PRIORITY || defined(INTERNAL_STATS) */

#if CM == CM_BACKOFF
	/* Reset backoff */
	tx->backoff = MIN_BACKOFF;
#endif /* CM == CM_BACKOFF */

#if CM == CM_PRIORITY
	/* Reset priority */
	tx->priority = 0;
	tx->visible_reads = 0;
#endif /* CM == CM_PRIORITY */
}


#endif
