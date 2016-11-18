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
 * Description:
 *   STM functions.
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


#include <mtm_i.h>
#include <useraction.h>
#include <pwb_i.h>
#include <rwset.h>
#include <cm.h>

//#define PRINT_DEBUG printf
//#define MTM_DEBUG_PRINT printf

static inline 
bool
pwb_trycommit (mtm_tx_t *tx, int enable_isolation)
{
	assert((tx->mode == MTM_MODE_pwbnl && !enable_isolation) || 
	       (tx->mode == MTM_MODE_pwbetl && enable_isolation));

	mode_data_t *modedata = (mode_data_t *) tx->modedata[tx->mode];
	w_entry_t   *w;
	mtm_word_t  t;
	int         i;
#ifdef READ_LOCKED_DATA
	mtm_word_t  id;
#endif /* READ_LOCKED_DATA */

	PRINT_DEBUG("==> mtm_commit(%p[%lu-%lu] nest_level:%d-->%d)\n", tx, 
	            (unsigned long)modedata->start, (unsigned long)modedata->end, tx->nesting, tx->nesting-1);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

	/* Decrement nesting level */
	if (--tx->nesting > 0) {
		return true;
	}	

	if (modedata->w_set.nb_entries > 0) {
		/* Update transaction */

		/* Get commit timestamp */
		t = FETCH_INC_CLOCK + 1;
		if (t >= VERSION_MAX) {
#ifdef ROLLOVER_CLOCK
			/* Abort: will reset the clock on next transaction start or delete */
#ifdef _M_STATS_BUILD
			m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
# ifdef INTERNAL_STATS
			tx->aborts_rollover++;
# endif /* INTERNAL_STATS */
			return false;
#else /* ! ROLLOVER_CLOCK */
			fprintf(stderr, "Exceeded maximum version number: 0x%lx\n", (unsigned long)t);
			exit(1);
#endif /* ! ROLLOVER_CLOCK */
		}

		/* Try to validate (only if a concurrent transaction has committed since tx->start) */
		if (enable_isolation) {
			if (modedata->start != t - 1 && !mtm_validate(tx, modedata)) {
				/* Cannot commit */
				/* Abort caused by invisible reads. */
				cm_visible_read(tx);
#ifdef _M_STATS_BUILD
				m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
#ifdef INTERNAL_STATS
				tx->aborts_validate_commit++;
#endif /* INTERNAL_STATS */
				return false;
			}
		}

# ifdef READ_LOCKED_DATA
		/* Update instance number (becomes odd) */
		id = tx->id;
		assert(id % 2 == 0);
		ATOMIC_STORE_REL(&tx->id, id + 1);
# endif /* READ_LOCKED_DATA */

		/* Make sure the persistent tm log is made stable */
		M_TMLOG_COMMIT(tx->pcm_storeset, modedata->ptmlog, t);

		/* Make sure previous stores are not reordered with the cl-flushes below  freud : unnecessary fence */
		/* PCM_WB_FENCE(tx->pcm_storeset);  moved this info M_TMLOG_COMMIT. It replaces PCM_NT_FLUSH in m_tmlog_base_? */

		/* Install new versions, drop locks and set new timestamp */
		/* In the case when isolation is off, the write set contains entries 
		 * that point to private pseudo-locks. */
		w = modedata->w_set.entries;
		int wbflush_cnt=0;
		for (i = modedata->w_set.nb_entries; i > 0; i--, w++) {
			MTM_DEBUG_PRINT("==> write(t=%p[%lu-%lu],a=%p,d=%p-%d,m=%llx,v=%d)\n", tx,
			                (unsigned long)modedata->start, (unsigned long)modedata->end,
			                w->addr, (void *)w->value, (int)w->value, (unsigned long long) w->mask, (int)w->version);
			/* Write the value in this entry to memory (it will probably land in the cache; that's okay.) */
			if (w->mask != 0) {
				PCM_WB_STORE_ALIGNED_MASKED(tx->pcm_storeset, w->addr, w->value, w->mask);
			}	
# ifdef	SYNC_TRUNCATION
			/* Flush the cacheline to persistent memory if this is the last entry in this cache line. */
			if (w->next_cache_neighbor == NULL) {
				/* If isolation is enabled, then the write set may contain non-persistent 
				 * writes as well. Need to filter those out as we don't need to flush them 
				 * out of the cache.
				 * FIXME: Would it be better if we had marked them as non-persistent and
				 *        avoid the bounds checking?
				 */
				if (enable_isolation) {
					if (((uintptr_t) w->addr >= PSEGMENT_RESERVED_REGION_START &&
						 (uintptr_t) w->addr < (PSEGMENT_RESERVED_REGION_START + PSEGMENT_RESERVED_REGION_SIZE)))
					{
						/* access is persistent -- flush, freud : why flush individual entries ? */
						PCM_WB_FLUSH(tx->pcm_storeset, w->addr);
						wbflush_cnt++;
					}
				} else {
					PCM_WB_FLUSH(tx->pcm_storeset, w->addr);
					wbflush_cnt++;
				}
			}	
# endif
			/* Only drop lock for last covered address in write set */
			if (w->next == NULL) {
				ATOMIC_STORE_REL(w->lock, LOCK_SET_TIMESTAMP(t));
			}	
		}
		PCM_WB_FENCE(tx->pcm_storeset);
#ifdef _M_STATS_BUILD
		m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, wbflush, wbflush_cnt);
#endif		
		//printf("w_set.nb_entries= %d\n", modedata->w_set.nb_entries);
		//printf("cachelines flushed= %d\n", wbflush_cnt);
# ifdef READ_LOCKED_DATA
		/* Update instance number (becomes even) */
		ATOMIC_STORE_REL(&tx->id, id + 2);
# endif /* READ_LOCKED_DATA */

# ifdef	SYNC_TRUNCATION
			M_TMLOG_TRUNCATE_SYNC(tx->pcm_storeset, modedata->ptmlog);
# endif
	}

#ifdef _M_STATS_BUILD	
	m_stats_threadstat_aggregate(tx->threadstat, tx->statset);
	assert(m_stats_statset_destroy(&tx->statset) == M_R_SUCCESS);
#endif	

	cm_reset(tx);
	return true;
}


/*
 * Rollback transaction.
 */
static inline 
void 
pwb_rollback(mtm_tx_t *tx)
{
	assert(tx->mode == MTM_MODE_pwbnl || MTM_MODE_pwbetl);
	mode_data_t   *modedata = (mode_data_t *) tx->modedata[tx->mode];
	w_entry_t     *w;
	int           i;
#ifdef READ_LOCKED_DATA
	mtm_word_t    id;
#endif /* READ_LOCKED_DATA */

	MTM_DEBUG_PRINT("==> pwb_rollback(%p[%lu-%lu])\n", tx,
	                (unsigned long)modedata->start,
	                (unsigned long)modedata->end);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

	/* Mark the transaction in the persistent log as aborted. */
	M_TMLOG_ABORT(tx->pcm_storeset, modedata->ptmlog, 0);
# ifdef	SYNC_TRUNCATION
	M_TMLOG_TRUNCATE_SYNC(tx->pcm_storeset, modedata->ptmlog);
# endif

	/* Drop locks */
	i = modedata->w_set.nb_entries;
	if (i > 0) {
# ifdef READ_LOCKED_DATA
		/* Update instance number (becomes odd) */
		id = tx->id;
		assert(id % 2 == 0);
		ATOMIC_STORE_REL(&tx->id, id + 1);
# endif /* READ_LOCKED_DATA */
		w = modedata->w_set.entries;
		for (; i > 0; i--, w++) {
			if (w->next == NULL) {
				/* Only drop lock for last covered address in write set */
				ATOMIC_STORE(w->lock, LOCK_SET_TIMESTAMP(w->version));
			}
			PRINT_DEBUG2("==> discard(t=%p[%lu-%lu],a=%p,d=%p-%lu,v=%lu)\n", tx,
			             (unsigned long)modedata->start, 
			             (unsigned long)modedata->end,
			             w->addr, 
			             (void *)w->value, 
			             (unsigned long)w->value,
			             (unsigned long)w->version);
		}
		/* Make sure that all lock releases become visible */
		ATOMIC_MB_WRITE;
# ifdef READ_LOCKED_DATA
		/* Update instance number (becomes even) */
		ATOMIC_STORE_REL(&tx->id, id + 2);
# endif /* READ_LOCKED_DATA */
	}

	tx->retries++;
#ifdef INTERNAL_STATS
	tx->aborts++;
	if (tx->max_retries < tx->retries) {
		tx->max_retries = tx->retries;
	}	
#endif /* INTERNAL_STATS */

	/* Set status (no need for CAS or atomic op) */
	tx->status = TX_ABORTED;

	/* 
	 * Reset nesting level 
	 * 
	 * CAUTION!!! Original TinySTM sets nesting to zero because on rollback 
	 * it restarts the transaction at the begin_transaction call. Intel STM
	 * does not make a call into begin_transaction call, so we need to be 
	 * careful to ensure that nesting value equals 1 to represent transactional 
	 * nesting. We do this later at prepare_transaction which is called before
	 * restart. We don't do this here because pwb_rollback may also be called 
	 * due to a user initiated abort.
	 */
	tx->nesting = 0;

}


static void
rollback_transaction (mtm_tx_t *tx)
{
	pwb_rollback (tx);
	switch (tx->mode) {
		case MTM_MODE_pwbnl:
			assert(0);
#if (defined(ALLOW_ABORTS))
			mtm_local_rollback (tx);
#endif
			break;
		case MTM_MODE_pwbetl:
			mtm_local_rollback (tx);
			break;
		default:
			assert(0);
	}
	mtm_useraction_list_run (tx->undo_action_list, 1);

	//FIXME: revert exceptions
	/*
	mtm_revert_cpp_exceptions ();
	if (tx->eh_in_flight)
	{
		_Unwind_DeleteException (tx->eh_in_flight);
		tx->eh_in_flight = NULL;
	}
	*/
}


static inline
void pwb_prepare_transaction(mtm_tx_t *tx)
{
	assert(tx->mode == MTM_MODE_pwbnl || MTM_MODE_pwbetl);
	mode_data_t *modedata = (mode_data_t *) tx->modedata[tx->mode];

start:
	/* Start timestamp */
	modedata->start = modedata->end = GET_CLOCK; /* OPT: Could be delayed until first read/write, why bother ? */
	/* Allow extensions */
	tx->can_extend = 1;
#ifdef ROLLOVER_CLOCK
	if (modedata->start >= VERSION_MAX) {
		/* Overflow: we must reset clock */
		mtm_overflow(tx);
		goto start;
	}
#endif /* ROLLOVER_CLOCK */
	/* Read/write set */
	
	/* Because of the current state of the recoverable write-set block, reallocation is not
	   possible. This program cannot be run until the recoverable blocks are
	   somehow extendable. */
	   //FIXME: What does the above comment mean?
	assert(modedata->w_set.reallocate == 0);
	
	modedata->w_set.nb_entries = 0;
	modedata->r_set.nb_entries = 0;
	mtm_useraction_clear (tx->commit_action_list);
	mtm_useraction_clear (tx->undo_action_list);

	M_TMLOG_BEGIN(modedata->ptmlog);

#ifdef EPOCH_GC
	gc_set_epoch(modedata->start);
#endif /* EPOCH_GC */

	tx->nesting = 1;
	tx->status = TX_ACTIVE;	/* Set status (no need for CAS or atomic op) */
	                        /* FIXME: TinySTM 1.0.0 uses atomic op when
	                         * using the modular contention manager */							

}

static inline
uint32_t
beginTransaction_internal (mtm_tx_t *tx, 
                           uint32_t prop, 
                           _ITM_srcLocation *srcloc,
                           int enable_isolation, jmp_buf **__env)
{
	assert(tx->mode == MTM_MODE_pwbnl || MTM_MODE_pwbetl);

	MTM_DEBUG_PRINT("==> mtm_pwb_beginTransaction(%p) nest_level: %d-->%d\n", tx,
	                tx->nesting, tx->nesting+1);

	/* Increment nesting level, freud : we did not enter here */
	if (tx->nesting++ > 0) {
		*__env = NULL;
		return a_runInstrumentedCode | a_saveLiveVariables;
	}	

	memcpy(&tx->jb, &tx->tmp_jb, sizeof(jmp_buf));
	*__env = &(tx->jb);
	tx->prop = prop;

	/* Initialize transaction descriptor */
	pwb_prepare_transaction(tx);

#ifdef _M_STATS_BUILD	
	assert(m_stats_statset_create(&tx->statset) == M_R_SUCCESS);
	assert(m_stats_statset_init(tx->statset, NULL /*srcloc->psource*/) == M_R_SUCCESS);
#endif	

	if ((prop & pr_doesGoIrrevocable) || !(prop & pr_instrumentedCode))
	{
		// TODO: Implement serial mode 
		return (prop & pr_uninstrumentedCode
		        ? a_runUninstrumentedCode : a_runInstrumentedCode);
	}

	if (enable_isolation) {
		// TODO: Implement serial mode 
	}

	return a_runInstrumentedCode | a_saveLiveVariables;
}


static 
bool
trycommit_transaction (mtm_tx_t *tx, int enable_isolation)
{
	if (pwb_trycommit(tx, enable_isolation)) {
		if (tx->nesting > 0) {
			return true;
		}
		switch (tx->mode) {
			case MTM_MODE_pwbnl:
				assert(0);
#if (defined(ALLOW_ABORTS))
				mtm_local_commit (tx);
#endif
				break;
			case MTM_MODE_pwbetl:
				mtm_local_commit (tx);
				break;
			default:
				assert(0);
		}

		mtm_useraction_list_run (tx->commit_action_list, 0);

		/* Set status (no need for CAS or atomic op) */
		tx->status = TX_COMMITTED;
		return true;
	}
	return false;
}
