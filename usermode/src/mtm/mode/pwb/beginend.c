#include "mtm_i.h"
#include "useraction.h"
#include "mode/pwb/pwb_i.h"
#include "mode/common/rwset.h"


static inline 
bool
pwb_trycommit (mtm_tx_t *tx)
{
	assert(tx->mode == MTM_MODE_pwb);
	mode_data_t *modedata = (mode_data_t *) tx->modedata[tx->mode];
	w_entry_t   *w;
	mtm_word_t  t;
	int         i;
#ifdef READ_LOCKED_DATA
	mtm_word_t  id;
#endif /* READ_LOCKED_DATA */

	PRINT_DEBUG("==> mtm_commit(%p[%lu-%lu])\n", tx, 
	            (unsigned long)modedata->start, (unsigned long)modedata->end);

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
		if (modedata->start != t - 1 && !mtm_validate(tx, modedata)) {
			/* Cannot commit */
#if CM == CM_PRIORITY
			/* Abort caused by invisible reads */
			tx->visible_reads++;
#endif /* CM == CM_PRIORITY */
#ifdef INTERNAL_STATS
			tx->aborts_validate_commit++;
#endif /* INTERNAL_STATS */
			return false;
		}

# ifdef READ_LOCKED_DATA
		/* Update instance number (becomes odd) */
		id = tx->id;
		assert(id % 2 == 0);
		ATOMIC_STORE_REL(&tx->id, id + 1);
# endif /* READ_LOCKED_DATA */

		nonvolatile_write_set_make_persistent(modedata->w_set_nv);
	
		/* Install new versions, drop locks and set new timestamp */
		w = modedata->w_set.entries;
		for (i = modedata->w_set.nb_entries; i > 0; i--, w++) {

			MTM_DEBUG_PRINT("==> write(t=%p[%lu-%lu],a=%p,d=%p-%d,v=%d)\n", tx,
			                (unsigned long)modedata->start, (unsigned long)modedata->end,
			                w->addr, (void *)w->value, (int)w->value, (int)w->version);
			/* Write the value in this entry to memory (it will probably land in the cache; that's okay.) */
			if (w->mask != 0)
				pcm_wb_store(modedata->pcm_storeset, w->w_entry_nv->address, w->w_entry_nv->value);
			
			/* Flush the cacheline to persistent memory if this is the last entry in this line. */
			if (w->w_entry_nv->next_cache_neighbor == NULL) {
				pcm_wb_flush(modedata->pcm_storeset, w->addr);
			}
			/* Only drop lock for last covered address in write set */
			if (w->next == NULL) {
				ATOMIC_STORE_REL(w->lock, LOCK_SET_TIMESTAMP(t));
			}	
		}
# ifdef READ_LOCKED_DATA
		/* Update instance number (becomes even) */
		ATOMIC_STORE_REL(&tx->id, id + 2);
# endif /* READ_LOCKED_DATA */
	}

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

	return true;
}


/*
 * Rollback transaction.
 */
static inline 
void 
pwb_rollback(mtm_tx_t *tx)
{
	assert(tx->mode == MTM_MODE_pwb);
	mode_data_t   *modedata = (mode_data_t *) tx->modedata[tx->mode];
	w_entry_t     *w;
	int           i;
#ifdef READ_LOCKED_DATA
	mtm_word_t    id;
#endif /* READ_LOCKED_DATA */

	PRINT_DEBUG("==> pwb_rollback(%p[%lu-%lu])\n", tx, 
	            (unsigned long)tx->start, (unsigned long)tx->end);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

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

#if CM == CM_PRIORITY || defined(INTERNAL_STATS)
	tx->retries++;
#endif /* CM == CM_PRIORITY || defined(INTERNAL_STATS) */
#ifdef INTERNAL_STATS
	tx->aborts++;
	if (tx->max_retries < tx->retries) {
		tx->max_retries = tx->retries;
	}	
#endif /* INTERNAL_STATS */

	/* Set status (no need for CAS or atomic op) */
	tx->status = TX_ABORTED;

	/* Reset nesting level */
	tx->nesting = 0;

}


void
pwb_cm(mtm_tx_t *tx)
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


uint32_t
mtm_pwb_beginTransaction_internal (mtm_tx_t *tx, 
                                     uint32_t prop, 
                                     _ITM_srcLocation *srcloc)
{
	assert(tx->mode == MTM_MODE_pwb);
	mode_data_t *modedata = (mode_data_t *) tx->modedata[tx->mode];

	MTM_DEBUG_PRINT("==> mtm_pwb_beginTransaction(%p)\n", tx);

	/* Increment nesting level */
	if (tx->nesting++ > 0) {
		return;
	}	

	tx->jb = tx->tmp_jb;
	tx->prop = prop;
	tx->status = TX_ACTIVE;	/* Set status (no need for CAS or atomic op) */
start:
	/* Start timestamp */
	modedata->start = modedata->end = GET_CLOCK; /* OPT: Could be delayed until first read/write */
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
	assert(modedata->w_set.reallocate == 0);
	
	modedata->w_set.nb_entries = 0;
	modedata->w_set_nv->nb_entries = 0;
	modedata->r_set.nb_entries = 0;

#ifdef EPOCH_GC
	gc_set_epoch(modedata->start);
#endif /* EPOCH_GC */

	//FIXME
	if ((prop & pr_doesGoIrrevocable) || !(prop & pr_instrumentedCode))
	{
		//MTM_serialmode (true, true);
		return (prop & pr_uninstrumentedCode
		        ? a_runUninstrumentedCode : a_runInstrumentedCode);
	}

	mtm_rwlock_read_lock (&mtm_serial_lock);

	return a_runInstrumentedCode | a_saveLiveVariables;
}


static void
rollback_transaction (mtm_tx_t *tx)
{
	pwb_rollback (tx);
	mtm_local_rollback (tx);

	//FIXME

	//mtm_useraction_freeActions (&tx->commit_actions);
	//mtm_useraction_runActions (&tx->undo_actions);
	//mtm_alloc_commit_allocations (true);

/*
	mtm_revert_cpp_exceptions ();
	if (tx->eh_in_flight)
	{
		_Unwind_DeleteException (tx->eh_in_flight);
		tx->eh_in_flight = NULL;
	}
	*/
}


void _ITM_CALL_CONVENTION
mtm_pwb_rollbackTransaction (mtm_tx_t *tx, const _ITM_srcLocation *loc)
{
	assert ((tx->prop & pr_hasNoAbort) == 0);
	//assert ((mtm_tx()->state & STATE_ABORTING) == 0);

	rollback_transaction (tx);
	//tx->status |= STATE_ABORTING;
}


void
mtm_pwb_print_properties(uint32_t prop) 
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


void ITM_NORETURN
mtm_pwb_restart_transaction (mtm_tx_t *tx, mtm_restart_reason r)
{
	uint32_t actions;

	rollback_transaction(tx);
	pwb_cm(tx);
	mtm_decide_retry_strategy (r); 

	actions = a_runInstrumentedCode | a_restoreLiveVariables;
	//if ((tx->prop & pr_uninstrumentedCode) && (tx->status & TX_IRREVOCABLE)) {
	//	actions = a_runUninstrumentedCode | a_restoreLiveVariables;
	//}	
	
	mtm_longjmp (&tx->jb, actions);
}



void _ITM_CALL_CONVENTION
mtm_pwb_abortTransaction (mtm_tx_t *tx, 
                            _ITM_abortReason reason,
                            const _ITM_srcLocation *loc)
{
	assert (reason == userAbort || reason == userRetry);
	/* Compiler Bug: pr_hasNoRetry seems to be wrong */
	assert ((reason == userAbort && (tx->prop & pr_hasNoAbort) == 0) || 
	        (reason == userRetry && 1));
	//assert ((tx->state & STATE_ABORTING) == 0);

	if (tx->status & TX_IRREVOCABLE) {
		abort ();
	}	

	if (reason == userAbort) {
		rollback_transaction (tx);
		//pwb_fini (td);

		if (tx->status & TX_SERIAL) {
			mtm_rwlock_write_unlock (&mtm_serial_lock);
		} else {
			mtm_rwlock_read_unlock (&mtm_serial_lock);
		}	

		//set_mtm_tx (tx->prev);
		//free_tx (td, tx);

		mtm_longjmp (&tx->jb, a_abortTransaction | a_restoreLiveVariables);
	} else if (reason == userRetry) {
		mtm_pwb_restart_transaction(tx, RESTART_USER_RETRY);
	}
}


static 
bool
trycommit_transaction (mtm_tx_t *tx)
{
	if (pwb_trycommit(tx)) {
		mtm_local_commit(tx);

		//FIXME
		//mtm_useraction_freeActions (&mtm_tx()->undo_actions);
		//mtm_useraction_runActions (&mtm_tx()->commit_actions);
		//mtm_alloc_commit_allocations (false);

		/* Set status (no need for CAS or atomic op) */
		tx->status = TX_COMMITTED;
		return true;
	}
	return false;
}


bool _ITM_CALL_CONVENTION
mtm_pwb_tryCommitTransaction (mtm_tx_t *tx, _ITM_srcLocation *loc)
{
	return trycommit_transaction(tx);
}


void _ITM_CALL_CONVENTION
mtm_pwb_commitTransaction(mtm_tx_t *tx, _ITM_srcLocation *loc)
{
	MTM_DEBUG_PRINT("==> mtm_pwb_commitTransaction(%p)\n", tx);
	if (!trycommit_transaction(tx)) {
		mtm_pwb_restart_transactions(tx, RESTART_VALIDATE_COMMIT);
	}
	MTM_DEBUG_PRINT("==> mtm_pwb_commitTransaction(%p): DONE\n", tx);
}


void _ITM_CALL_CONVENTION
mtm_pwb_commitTransactionToId(mtm_tx_t *tx, 
                                const _ITM_transactionId tid,
                                _ITM_srcLocation *loc)
{
	//TODO
}
