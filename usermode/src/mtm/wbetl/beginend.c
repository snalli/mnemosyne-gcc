/* Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include "mtm_i.h"
#include "useraction.h"
#include <wbetl/barrier.h>

//FIXME: all functions are currently getting the descriptor using gettx()
//this is unnecessary since the API provides it.

mtm_rwlock mtm_serial_lock;

mtm_stmlock mtm_stmlock_array[LOCK_ARRAY_SIZE];
mtm_version mtm_clock;

/* ??? Move elsewhere when we figure out library initialization.  */
uint64_t mtm_spin_count_var = 1000;

static _ITM_transactionId global_tid;


/* Commit the transaction.  */

static bool
wbetl_trycommit (mtm_thread_t *td)
{
	struct mtm_method *m = mtm_tx()->m;
	w_entry_t         *w;
	mtm_word          t;
	int               i;

	if (m->w_set.nb_entries > 0) {
		/* Get commit timestamp.  */
		t = mtm_inc_clock ();

		/* Validate only if a concurrent transaction has started since.  */
		if (m->start != t - 1 && !wbetl_validate (td, m)) {
			return false;
		}	

		/* Install new versions, drop locks and set new timestamp.  */
		w = m->w_set.entries;
		for (i = m->w_set.nb_entries; i > 0; i--, w++) {
			mtm_cacheline_copy_mask ((mtm_cacheline *) w->addr,
			                         w->value, *mtm_mask_for_line (w->value));
		}
		/* Only emit barrier after all cachelines are copied.  */
		mtm_ccm_write_barrier ();

		w = m->w_set.entries;
		for (i = m->w_set.nb_entries; i > 0; i--, w++) {
			if (w->next == NULL) {
				*w->lock = mtm_stmlock_set_version (t);
			}	
			__sync_synchronize ();
		}
	}	

	return true;
}

static void
wbetl_rollback (mtm_thread_t *td)
{
	struct mtm_method *m = mtm_tx()->m;
	w_entry_t         *w;
	int               i;

	/* Drop locks.  */
	w = m->w_set.entries;
	for (i = m->w_set.nb_entries; i > 0; i--, w++) {
		if (w->next == NULL) {
			*w->lock = mtm_stmlock_set_version (w->version);
		}
	}
	__sync_synchronize ();
}

static void
wbetl_init (mtm_thread_t *td, bool first)
{
	struct mtm_method *m;

	if (first) {
		mtm_tx()->m = m = calloc (1, sizeof (*m));
		m->r_set.size = RW_SET_SIZE;
		m->r_set.entries = malloc (RW_SET_SIZE * sizeof(r_entry_t));
		m->w_set.size = RW_SET_SIZE;
		m->w_set.entries = malloc (RW_SET_SIZE * sizeof(w_entry_t));
	} else {
		mtm_cacheline_page *page;

		m = mtm_tx()->m;
		m->r_set.nb_entries = 0;

		m->w_set.nb_entries = 0;
		if (m->w_set.reallocate)
		{
			m->w_set.reallocate = 0;
			m->w_set.entries = realloc (m->w_set.entries,
			                            m->w_set.size * sizeof(w_entry_t));
		}

		page = m->cache_page;
		if (page) {
			/* Release all but one of the pages of cachelines.  */
			mtm_cacheline_page *prev = page->prev;
			if (prev) {
				mtm_cacheline_page *tail;
				for (tail = prev; tail->prev; tail = tail->prev) {
					continue;
				}	
				page->prev = NULL;
				mtm_page_release (prev, tail);
			}
			/* Start the next cacheline allocation from the beginning.  */
			m->n_cache_page = 0;
		}
    }

	m->start = m->end = mtm_get_clock ();
}

static void
wbetl_fini (mtm_thread_t *td)
{
	struct mtm_method  *m = mtm_tx()->m;
	mtm_cacheline_page *page;
	mtm_cacheline_page *tail;

	page = m->cache_page;
	if (page) {
		for (tail = page; tail->prev; tail = tail->prev) {
			continue;
		}	
		mtm_page_release (page, tail);
	}

	free (m->r_set.entries);
	free (m->w_set.entries);
	free (m);
}

/* Allocate a transaction structure.  Reuse an old one if possible.  */
static mtm_transaction_t *
alloc_tx (mtm_thread_t *td)
{
	mtm_transaction_t *tx;
	mtm_thread_t *thr = mtm_thr ();

	if (thr->free_tx_count == 0) {
		tx = malloc (sizeof (*tx));
	} else {
		thr->free_tx_count--;
		tx = thr->free_tx[thr->free_tx_idx];
		thr->free_tx_idx = (thr->free_tx_idx + 1) % MAX_FREE_TX;
	}

	return tx;
}

/* Queue a transaction structure for freeing.  We never free the given
   transaction immediately -- this is a requirement of abortTransaction
   as the jmpbuf is used immediately after calling this function.  Thus
   the requirement that this queue be per-thread.  */

static void
free_tx (mtm_thread_t *td, mtm_transaction_t *tx)
{
	mtm_thread_t *thr = mtm_thr ();
	unsigned idx = (thr->free_tx_idx + thr->free_tx_count) % MAX_FREE_TX;

	if (thr->free_tx_count == MAX_FREE_TX) {
		thr->free_tx_idx = (thr->free_tx_idx + 1) % MAX_FREE_TX;
		free (thr->free_tx[idx]);
	} else {
		thr->free_tx_count++;
	}	

	thr->free_tx[idx] = tx;
}


uint32_t
mtm_wbetl_beginTransaction_internal (mtm_thread_t *td, 
                                     uint32_t prop, 
                                     _ITM_srcLocation *srcloc)
{
	mtm_transaction_t *tx;
	
	tx = alloc_tx (td);
	memset (tx, 0, sizeof (*tx));

	tx->prop = prop;
	tx->prev = mtm_tx();
	if (tx->prev) {
		tx->nesting = tx->prev->nesting + 1;
	}	
	tx->id = __sync_add_and_fetch (&global_tid, 1);
	tx->jb = td->tmp_jb;

	set_mtm_tx (tx);

	if ((prop & pr_doesGoIrrevocable) || !(prop & pr_instrumentedCode)) {
		mtm_serialmode (true, true);
		return (prop & pr_uninstrumentedCode
		        ? a_runUninstrumentedCode : a_runInstrumentedCode);
	}

	/* ??? Probably want some environment variable to choose the default
	   STM implementation once we have more than one implemented.  */

	//FIXME : do we have pr_readOnly in ICC?
	//FIXME : if yet, then must restart on readonly mode
//	if (prop & pr_readOnly)
//    disp = &dispatch_readonly;
//  else
 //   disp = &dispatch_wbetl;

	wbetl_init (td, true);

	mtm_rwlock_read_lock (&mtm_serial_lock);

	return a_runInstrumentedCode | a_saveLiveVariables;
}

static void
rollback_transaction (mtm_thread_t *td)
{
	mtm_transaction_t *tx;

	wbetl_rollback (td);
	mtm_rollback_local ();

	tx = mtm_tx();
	mtm_useraction_freeActions (&tx->commit_actions);
	mtm_useraction_runActions (&tx->undo_actions);
	mtm_alloc_commit_allocations (true);

	mtm_revert_cpp_exceptions ();
	if (tx->eh_in_flight)
	{
		_Unwind_DeleteException (tx->eh_in_flight);
		tx->eh_in_flight = NULL;
	}
}

void _ITM_CALL_CONVENTION
mtm_wbetl_rollbackTransaction (mtm_thread_t *td, const _ITM_srcLocation *loc)
{
	assert ((mtm_tx()->prop & pr_hasNoAbort) == 0);
	assert ((mtm_tx()->state & STATE_ABORTING) == 0);

	rollback_transaction (td);
	mtm_tx()->state |= STATE_ABORTING;
}

void
print_properties(uint32_t prop) 
{
	printf("pr_instrumentedCode     = %d\n", (prop & pr_instrumentedCode) ? 1 : 0);
	printf("pr_uninstrumentedCode   = %d\n", (prop & pr_uninstrumentedCode) ? 1 : 0);
	printf("pr_multiwayCode         = %d\n", (prop & pr_multiwayCode) ? 1 : 0);
	printf("pr_hasNoXMMUpdate       = %d\n", (prop & pr_hasNoXMMUpdate) ? 1 : 0);
	printf("pr_hasNoAbort           = %d\n", (prop & pr_hasNoAbort) ? 1 : 0);
	printf("pr_hasNoRetry           = %d\n", (prop & pr_hasNoRetry) ? 1 : 0);
	printf("pr_hasNoIrrevocable     = %d\n", (prop & pr_hasNoIrrevocable) ? 1 : 0);
	printf("pr_doesGoIrrevocable    = %d\n", (prop & pr_doesGoIrrevocable) ? 1 : 0);
	printf("pr_hasNoSimpleReads     = %d\n", (prop & pr_hasNoSimpleReads) ? 1 : 0);
	printf("pr_aWBarriersOmitted    = %d\n", (prop & pr_aWBarriersOmitted) ? 1 : 0);
	printf("pr_RaRBarriersOmitted   = %d\n", (prop & pr_RaRBarriersOmitted) ? 1 : 0);
	printf("pr_undoLogCode          = %d\n", (prop & pr_undoLogCode) ? 1 : 0);
	printf("pr_preferUninstrumented = %d\n", (prop & pr_preferUninstrumented) ? 1 : 0);
	printf("pr_exceptionBlock       = %d\n", (prop & pr_exceptionBlock) ? 1 : 0);
	printf("pr_hasElse              = %d\n", (prop & pr_hasElse) ? 1 : 0);
}

void _ITM_CALL_CONVENTION
mtm_wbetl_abortTransaction (mtm_thread_t *td, 
                            _ITM_abortReason reason,
                            const _ITM_srcLocation *loc)
{
	mtm_transaction_t *tx = mtm_tx();

	assert (reason == userAbort || reason == userRetry);
	/* Compiler Bug: pr_hasNoRetry seems to be wrong */
	assert ((reason == userAbort && (tx->prop & pr_hasNoAbort) == 0) || 
	        (reason == userRetry && 1));
	assert ((tx->state & STATE_ABORTING) == 0);

	if (tx->state & STATE_IRREVOCABLE) {
		abort ();
	}	

	if (reason == userAbort) {
		rollback_transaction (td);
		wbetl_fini (td);

		if (tx->state & STATE_SERIAL) {
			mtm_rwlock_write_unlock (&mtm_serial_lock);
		} else {
			mtm_rwlock_read_unlock (&mtm_serial_lock);
		}	

		set_mtm_tx (tx->prev);
		free_tx (td, tx);

		mtm_longjmp (&tx->jb, a_abortTransaction | a_restoreLiveVariables);
	} else if (reason == userRetry) {
		mtm_wbetl_restart_transaction(td, RESTART_USER_RETRY);
	}
}

static inline bool
trycommit_transaction (mtm_thread_t *td)
{
	if (wbetl_trycommit (td))
	{
		mtm_commit_local ();
		mtm_useraction_freeActions (&mtm_tx()->undo_actions);
		mtm_useraction_runActions (&mtm_tx()->commit_actions);
		mtm_alloc_commit_allocations (false);
		return true;
	}
	return false;
}

static bool
trycommit_and_finalize_transaction (mtm_thread_t *td)
{
	mtm_transaction_t *tx = mtm_tx();

	if ((tx->state & STATE_ABORTING) || trycommit_transaction (td))
    {
		wbetl_fini (td);
		set_mtm_tx (tx->prev);
		free_tx (td, tx);
		return true;
    }

	return false;
}


bool _ITM_CALL_CONVENTION
mtm_wbetl_tryCommitTransaction (mtm_thread_t *td, _ITM_srcLocation *loc)
{
	assert ((mtm_tx()->state & STATE_ABORTING) == 0);
	return trycommit_transaction (td);
}


void ITM_NORETURN
mtm_wbetl_restart_transaction (mtm_thread_t *td, mtm_restart_reason r)
{
	mtm_transaction_t *tx = mtm_tx();
	uint32_t actions;

	rollback_transaction (td);
	mtm_decide_retry_strategy (r);

	actions = a_runInstrumentedCode | a_restoreLiveVariables;
	if ((tx->prop & pr_uninstrumentedCode) && (tx->state & STATE_IRREVOCABLE)) {
		actions = a_runUninstrumentedCode | a_restoreLiveVariables;
	}	

	mtm_longjmp (&tx->jb, actions);
}


void _ITM_CALL_CONVENTION
mtm_wbetl_commitTransaction(mtm_thread_t *td, _ITM_srcLocation *loc)
{
	if (!trycommit_and_finalize_transaction (td)) {
		mtm_wbetl_restart_transaction (td, RESTART_VALIDATE_COMMIT);
	}	
}


void _ITM_CALL_CONVENTION
mtm_wbetl_commitTransactionToId(mtm_thread_t *td, 
                                const _ITM_transactionId tid,
                                _ITM_srcLocation *loc)
{
	//TODO
}
