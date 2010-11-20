#include "mtm_i.h"
#include "useraction.h"
#include "mode/pwb/pwb_i.h"
#include "mode/common/rwset.h"
#include "cm.h" 

static inline 
bool
pwb_trycommit (mtm_tx_t *tx, int enable_isolation)
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
		if (enable_isolation) {
			if (modedata->start != t - 1 && !mtm_validate(tx, modedata)) {
				/* Cannot commit */
				/* Abort caused by invisible reads. */
				cm_visible_read(tx);
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

		/* Install new versions, drop locks and set new timestamp */
		/* In the case when isolation is off, the write set contains entries 
		 * that point to private pseudo-locks. */
		w = modedata->w_set.entries;
		int tempcnt=0;
		for (i = modedata->w_set.nb_entries; i > 0; i--, w++) {
			MTM_DEBUG_PRINT("==> write(t=%p[%lu-%lu],a=%p,d=%p-%d,m=%llx,v=%d)\n", tx,
			                (unsigned long)modedata->start, (unsigned long)modedata->end,
			                w->addr, (void *)w->value, (int)w->value, (unsigned long long) w->mask, (int)w->version);
			/* Write the value in this entry to memory (it will probably land in the cache; that's okay.) */
			if (w->mask != 0) {
				PCM_WB_STORE_ALIGNED_MASKED(tx->pcm_storeset, w->addr, w->value, w->mask);
			}	
# ifdef	SYNC_TRUNCATION
				/* Flush the cacheline to persistent memory if this is the last entry in this line. */
				if (w->next_cache_neighbor == NULL) {
					PCM_WB_FLUSH(tx->pcm_storeset, w->addr);
					tempcnt++;
				}
# endif
			/* Only drop lock for last covered address in write set */
			if (w->next == NULL) {
				ATOMIC_STORE_REL(w->lock, LOCK_SET_TIMESTAMP(t));
			}	
		}
		//printf("w_set.nb_entries= %d\n", modedata->w_set.nb_entries);
		//printf("cachelines flusthed= %d\n", tempcnt);
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

	/* Mark the transaction in the persistent log as aborted. */
	M_TMLOG_ABORT(tx->pcm_storeset, modedata->ptmlog, 0);

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

	/* Reset nesting level */
	tx->nesting = 0;

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
		return a_runInstrumentedCode | a_saveLiveVariables;
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
	   //FIXME: What does the above comment mean?
	assert(modedata->w_set.reallocate == 0);
	
	modedata->w_set.nb_entries = 0;
	modedata->r_set.nb_entries = 0;

	M_TMLOG_BEGIN(modedata->ptmlog);

#ifdef EPOCH_GC
	gc_set_epoch(modedata->start);
#endif /* EPOCH_GC */


#ifdef _M_STATS_BUILD	
	assert(m_stats_statset_create(&tx->statset) == M_R_SUCCESS);
	assert(m_stats_statset_init(tx->statset, srcloc->psource) == M_R_SUCCESS);
#endif	

	if ((prop & pr_doesGoIrrevocable) || !(prop & pr_instrumentedCode))
	{
		// FIXME: Currently we don't implement serial mode 
		//MTM_serialmode (true, true);
		return (prop & pr_uninstrumentedCode
		        ? a_runUninstrumentedCode : a_runInstrumentedCode);
	}

#if defined(ENABLE_ISOLATION)
	// FIXME: Currently we don't implement serial mode 
	//mtm_rwlock_read_lock (&mtm_serial_lock);
#endif

	return a_runInstrumentedCode | a_saveLiveVariables;
}


static void
rollback_transaction (mtm_tx_t *tx)
{
	pwb_rollback (tx);
#if defined(ENABLE_ISOLATION) ||                                              \
    (!defined(ENABLE_ISOLATION) && defined(ALLOW_ABORTS))
	mtm_local_rollback (tx);
#endif
	mtm_useraction_freeActions (&tx->commit_actions);
	mtm_useraction_runActions (&tx->undo_actions);

/*
	//FIXME: revert exceptions
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
#if (!defined(ALLOW_ABORTS))
	assert(0 && "Aborts disabled but want to abort.\n");
#endif

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

#if (!defined(ALLOW_ABORTS))
	assert(0 && "Aborts disabled but want to abort.\n");
#endif

	rollback_transaction(tx);
	cm_delay(tx);
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
#if (!defined(ALLOW_ABORTS))
	assert(0 && "Aborts disabled but want to abort.\n");
#endif

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

#if defined(ENABLE_ISOLATION)
		// FIXME: Currently we don't implement serial mode 
		//if (tx->status & TX_SERIAL) {
		//	mtm_rwlock_write_unlock (&mtm_serial_lock);
		//} else {
		//	mtm_rwlock_read_unlock (&mtm_serial_lock);
		//}	
#endif		

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
#ifdef ENABLE_ISOLATION
	if (pwb_trycommit(tx, 1)) {
#else 
	if (pwb_trycommit(tx, 0)) {
#endif
		if (tx->nesting > 0) {
			return true;
		}
#if defined(ENABLE_ISOLATION) ||                                              \
    (!defined(ENABLE_ISOLATION) && defined(ALLOW_ABORTS))
		mtm_local_commit(tx);
#endif
		mtm_useraction_freeActions (&tx->undo_actions);
		mtm_useraction_runActions (&tx->commit_actions);

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
		mtm_pwb_restart_transaction(tx, RESTART_VALIDATE_COMMIT);
	}
	if (tx->status == TX_COMMITTED) {
		tx->status = TX_IDLE; //FIXME: Can we set IDLE here ? Don't others depend
		                      // on seeing our state as TX_COMMITTED. For example, 
		                      // do we need to quiesce first? TinySTM 1.0.0 has 
		                      // quiescence support.
	}							  

	MTM_DEBUG_PRINT("==> mtm_pwb_commitTransaction(%p): DONE\n", tx);
}


void _ITM_CALL_CONVENTION
mtm_pwb_commitTransactionToId(mtm_tx_t *tx, 
                                const _ITM_transactionId tid,
                                _ITM_srcLocation *loc)
{
	//TODO
	assert(0);
}
