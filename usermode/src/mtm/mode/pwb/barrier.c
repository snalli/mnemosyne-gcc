#include "mtm_i.h"
#include "mode/pwb/pwb_i.h"
#include "mode/pwb/barrier.h"
#include "mode/pwb/rwset.c"
#include "hal/pcm.h"


/*
 * Extend snapshot range.
 */
static inline 
int 
pwb_extend (mtm_tx_t *tx, mode_data_t *modedata)
{
	mtm_word_t now;

	PRINT_DEBUG("==> pwb_extend(%p[%lu-%lu])\n", tx, 
	            (unsigned long)modedata->start,
	            (unsigned long)modedata->end);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

	/* Get current time */
	now = GET_CLOCK;
#ifdef ROLLOVER_CLOCK
	if (now >= VERSION_MAX) {
		/* Clock overflow */
		return 0;
	}
#endif /* ROLLOVER_CLOCK */
	/* Try to validate read set */
	if (mtm_validate(tx, modedata)) {
		/* It works: we can extend until now */
		modedata->end = now;
		return 1;
	}
	return 0;
}


/*
 * Store a word-sized value (return write set entry or NULL).
 */
static inline 
w_entry_t *
pwb_write(mtm_tx_t *tx, 
          volatile mtm_word_t *addr, 
          mtm_word_t value,
          mtm_word_t mask)
{
	assert(tx->mode == MTM_MODE_pwb);
	mode_data_t         *modedata = (mode_data_t *) tx->modedata[tx->mode];
	volatile mtm_word_t *lock;
	mtm_word_t          l;
	mtm_word_t          version;
	w_entry_t           *w;
	w_entry_t           *prev = NULL;
	w_entry_t           *w_entry_same_block_addr = NULL;
	uintptr_t           block_addr;

	MTM_DEBUG_PRINT("==> pwb_write(t=%p[%lu-%lu],a=%p,d=%p-%lu,m=0x%lx)\n", tx,
	                (unsigned long)modedata->start,
	                (unsigned long)modedata->end, 
	                addr, 
	                (void *)value, 
	                (unsigned long)value,
	                (unsigned long)mask);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

	/* Filter out stack accesses */
	if ((uintptr_t) addr <= tx->stack_base && 
	   (uintptr_t) addr > tx->stack_base - tx->stack_size)
	{
		mtm_word_t prev_value;
		prev_value = ATOMIC_LOAD(addr);
		if (mask == 0) {
			return NULL;
		}
		if (mask != ~(mtm_word_t)0) {
			value = (prev_value & ~mask) | (value & mask);
		}	

		mtm_local_LB(tx, addr, sizeof(mtm_word_t));
		ATOMIC_STORE(addr, value);
		return NULL;
	}

	/* Get reference to lock */
	lock = GET_LOCK(addr);

	/* Try to acquire lock */
restart:
	l = ATOMIC_LOAD_ACQ(lock);
restart_no_load:
	if (LOCK_GET_OWNED(l)) {
		/* Locked */
		/* Do we own the lock? */
		w = (w_entry_t *)LOCK_GET_ADDR(l);
		/* Simply check if address falls inside our write set (avoids non-faulting load) */
		if (modedata->w_set.entries <= w &&
		    w < modedata->w_set.entries + modedata->w_set.nb_entries) 
		{
			/* Yes */
			prev = w;
			block_addr = BLOCK_ADDR(addr);
			/* Did we previously write the same address? */
			while (1) {
				if (addr == prev->addr) {
					if (mask == 0) {
						return prev;
					}
					/* No need to add to write set */
					PRINT_DEBUG2("==> mtm_write(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu,m=0x%lx)\n",
								 tx, (unsigned long)modedata->start,
								 (unsigned long)modedata->end,
								 addr,
								 lock,
								 (unsigned long)l,
								 (void *)value,
								 (unsigned long)value,
								 (unsigned long)mask);

					if (mask != ~(mtm_word_t)0) {
						if (prev->mask == 0) {
							prev->value = ATOMIC_LOAD(addr);
						}
						value = (prev->value & ~mask) | (value & mask);
					}
					prev->value = value;
					prev->mask |= mask;
					pcm_wb_store(modedata->pcm_storeset, &prev->w_entry_nv->value, value);
					pcm_wb_store(modedata->pcm_storeset, &prev->w_entry_nv->mask, prev->mask);
					return prev;
				}
				if (block_addr == BLOCK_ADDR(prev->addr)) {
					w_entry_same_block_addr = prev;
				}
				if (prev->next == NULL) {
					/* Remember last entry in linked list (for adding new entry) */
					break;
				}
				prev = prev->next;
			}
			/* Get version from previous write set entry (all entries in linked list have same version) */
			version = prev->version;
			/* Must add to write set */
			if (modedata->w_set.nb_entries == modedata->w_set.size) {
				/* Extend write set (invalidate pointers to write set entries => abort and reallocate) */
				modedata->w_set.size *= 2;
				modedata->w_set.reallocate = 1;
				modedata->w_set_nv.size *= 2;
				modedata->w_set_nv.reallocate = 1;
# ifdef INTERNAL_STATS
				tx->aborts_reallocate++;
# endif /* INTERNAL_STATS */
				mtm_pwb_restart_transaction (tx, RESTART_REALLOCATE);
			}
			w = &modedata->w_set.entries[modedata->w_set.nb_entries];
			w->w_entry_nv = &modedata->w_set_nv.entries[modedata->w_set_nv.nb_entries];
			goto do_write;
		}

		/* Conflict: CM kicks in */
#if CM == CM_PRIORITY
		if (tx->retries >= cm_threshold) {
			if (LOCK_GET_PRIORITY(l) < tx->priority ||
			    (LOCK_GET_PRIORITY(l) == tx->priority &&
			    l < (mtm_word_t)modedata->w_set.entries
			    && !LOCK_GET_WAIT(l))) 
			{
				/* We have higher priority */
				if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_PRIORITY_WAIT(l, tx->priority)) == 0) {
					goto restart;
				}
				l = LOCK_SET_PRIORITY_WAIT(l, tx->priority);
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
				if (l != lw) {
					l = lw;
					goto restart_no_load;
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
		/* Abort */
#ifdef INTERNAL_STATS
		tx->aborts_locked_write++;
#endif /* INTERNAL_STATS */
		mtm_pwb_restart_transaction (tx, RESTART_LOCKED_WRITE);
	} else {
		/* Not locked */
		/* Handle write after reads (before CAS) */
		version = LOCK_GET_TIMESTAMP(l);

		if (version > modedata->end) {
			/* We might have read an older version previously */
			if (!tx->can_extend || mtm_has_read(tx, modedata, lock) != NULL) {
				/* Read version must be older (otherwise, tx->end >= version) */
				/* Not much we can do: abort */
#if CM == CM_PRIORITY
				/* Abort caused by invisible reads */
				tx->visible_reads++;
#endif /* CM == CM_PRIORITY */
#ifdef INTERNAL_STATS
				tx->aborts_validate_write++;
#endif /* INTERNAL_STATS */
				mtm_pwb_restart_transaction (tx, RESTART_VALIDATE_WRITE);
			}
		}
		
		/* Acquire lock (ETL) */
		if (modedata->w_set.nb_entries == modedata->w_set.size) {
			/* Extend write set (invalidate pointers to write set entries => abort and reallocate) */
			modedata->w_set.size *= 2;
			modedata->w_set.reallocate = 1;
			modedata->w_set_nv.size *= 2;
			modedata->w_set_nv.reallocate = 1;
# ifdef INTERNAL_STATS
			tx->aborts_reallocate++;
# endif /* INTERNAL_STATS */
			mtm_pwb_restart_transaction (tx, RESTART_REALLOCATE);
		}
	    w = &modedata->w_set.entries[modedata->w_set.nb_entries];
		w->w_entry_nv = &modedata->w_set_nv.entries[modedata->w_set_nv.nb_entries];
# if CM == CM_PRIORITY
		if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_ADDR((mtm_word_t)w, tx->priority)) == 0) {
			goto restart;
		}
# else /* CM != CM_PRIORITY */
		if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_ADDR((mtm_word_t)w)) == 0) {
			goto restart;
		}
# endif /* CM != CM_PRIORITY */
	}
	
	/* We own the lock here (ETL) */
do_write:
	PRINT_DEBUG2("==> mtm_write(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu,m=0x%lx)\n",
	             tx, 
	             (unsigned long)modedata->start,
	             (unsigned long)modedata->end,
	             addr,
	             lock,
	             (unsigned long)l,
	             (void *)value,
	             (unsigned long)value,
	             (unsigned long)mask);

	/* Add address to write set */
	w->addr = addr;
	w->mask = mask;
	w->lock = lock;
	pcm_wb_store(modedata->pcm_storeset, &w->w_entry_nv->addr, addr);	
	pcm_wb_store(modedata->pcm_storeset, &w->w_entry_nv->mask, mask);	
	if (mask == 0) {
		/* Do not write anything */
#ifndef NDEBUG
		w->value = 0;
		pcm_wb_store(modedata->pcm_storeset, &w->w_entry_nv->value, 0);	
#endif /* ! NDEBUG */
	} else
	{
		/* Remember new value */
		if (mask != ~(mtm_word_t)0) {
			value = (ATOMIC_LOAD(addr) & ~mask) | (value & mask);
		}	
		w->value = value;
		pcm_wb_store(modedata->pcm_storeset, &w->w_entry_nv->value, value);	
	}
	w->version = version;
	w->next = NULL;
	w->next_cacheline = NULL;
	if (prev != NULL) {
		/* Link new entry in list */
		prev->next = w;
	}
	if (w_entry_same_block_addr != NULL) {
		w_entry_same_block_addr->next_cacheline = w;
	}

	modedata->w_set.nb_entries++;
	modedata->w_set_nv.nb_entries++;

	return w;
}



/*
 * Called by the CURRENT thread to load a word-sized value.
 */
//static inline
mtm_word_t 
mtm_pwb_load(mtm_tx_t *tx, volatile mtm_word_t *addr)
{
	assert(tx->mode == MTM_MODE_pwb);
	mode_data_t         *modedata = (mode_data_t *) tx->modedata[tx->mode];
	volatile mtm_word_t *lock;
	mtm_word_t          l;
	mtm_word_t          l2;
	mtm_word_t          value;
	mtm_word_t          version;
	r_entry_t           *r;
	w_entry_t           *w;


	MTM_DEBUG_PRINT("==> mtm_pwb_load(t=%p[%lu-%lu],a=%p)\n", tx, 
	                (unsigned long)modedata->start,
	                (unsigned long)modedata->end, 
	                addr);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

	/* Filter out stack accesses */
	if ((uintptr_t) addr <= tx->stack_base && 
	    (uintptr_t) addr > tx->stack_base - tx->stack_size)
	{
		value = ATOMIC_LOAD(addr);
		return value;
	}


#if CM == CM_PRIORITY
	if (tx->visible_reads >= vr_threshold && vr_threshold >= 0) {
		/* Visible reads: acquire lock first */
		w = mtm_pwb_write(tx, addr, 0, 0);
		/* Make sure we did not abort */
		if(tx->status != TX_ACTIVE) {
			return 0;
		}
		assert(w != NULL);
		/* We now own the lock */
		return w->mask == 0 ? ATOMIC_LOAD(addr) : w->value;
	}
#endif /* CM == CM_PRIORITY */

	/* Get reference to lock */
	lock = GET_LOCK(addr);

	/* Note: we could check for duplicate reads and get value from read set */

	/* Read lock, value, lock */
restart:
	l = ATOMIC_LOAD_ACQ(lock);
restart_no_load:
	if (LOCK_GET_OWNED(l)) {
	    /* Locked */
    	/* Do we own the lock? */
		w = (w_entry_t *)LOCK_GET_ADDR(l);
		/* Simply check if address falls inside our write set (avoids non-faulting load) */
		if (modedata->w_set.entries <= w && 
		    w < modedata->w_set.entries + modedata->w_set.nb_entries)
		{
			/* Yes: did we previously write the same address? */
			while (1) {
				if (addr == w->addr) {
					/* Yes: get value from write set (or from memory if mask was empty) */
					value = (w->mask == 0 ? ATOMIC_LOAD(addr) : w->value);
					MTM_DEBUG_PRINT("==> mtm_load[OWN LOCK|READ FROM WSET]");
					break;
				}
				if (w->next == NULL) {
					/* No: get value from memory */
					value = ATOMIC_LOAD(addr);
					MTM_DEBUG_PRINT("==> mtm_load[OWN LOCK|READ FROM MEMORY]");
					break;
				}
				w = w->next;
			}
			/* No need to add to read set (will remain valid) */
			MTM_DEBUG_PRINT("(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu)\n",
			                tx, (unsigned long)modedata->start, 
			                (unsigned long)modedata->end, 
			                addr, 
			                lock, 
			                (unsigned long)l,
			                (void *)value,
			                (unsigned long)value);
			return value;
		}
		/* Conflict: CM kicks in */
		/* TODO: we could check for duplicate reads and get value from read set (should be rare) */
#if CM == CM_PRIORITY
		if (tx->retries >= cm_threshold) {
			if (LOCK_GET_PRIORITY(l) < tx->priority ||
				(LOCK_GET_PRIORITY(l) == tx->priority &&
			    l < (mtm_word_t)modedata->w_set.entries
			    && !LOCK_GET_WAIT(l))) 
			{
				/* We have higher priority */
				if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_PRIORITY_WAIT(l, tx->priority)) == 0) {
					goto restart;
				}
				l = LOCK_SET_PRIORITY_WAIT(l, tx->priority);
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
				if (l != lw) {
					l = lw;
					goto restart_no_load;
				}
			}
give_up:
			if (tx->priority < PRIORITY_MAX) {
				tx->priority++;
			} else {
				MTM_DEBUG_PRINT("Reached maximum priority\n");
			}
			tx->c_lock = lock;
		}
#elif CM == CM_DELAY
		tx->c_lock = lock;
#endif /* CM == CM_DELAY */
		/* Abort */
#ifdef INTERNAL_STATS
		tx->aborts_locked_read++;
#endif /* INTERNAL_STATS */
		mtm_pwb_restart_transaction (RESTART_LOCKED_READ);
	} else {
		/* Not locked */
		value = ATOMIC_LOAD_ACQ(addr);
		l2 = ATOMIC_LOAD_ACQ(lock);
		if (l != l2) {
			l = l2;
			goto restart_no_load;
		}
		/* Check timestamp */
		version = LOCK_GET_TIMESTAMP(l);
		/* Valid version? */
		if (version > modedata->end) {
			/* No: try to extend first (except for read-only transactions: no read set) */
			if (!tx->can_extend || !pwb_extend(tx, modedata)) {
				/* Not much we can do: abort */
#if CM == CM_PRIORITY
				/* Abort caused by invisible reads */
				tx->visible_reads++;
#endif /* CM == CM_PRIORITY */
#ifdef INTERNAL_STATS
				tx->aborts_validate_read++;
#endif /* INTERNAL_STATS */
				mtm_pwb_restart_transaction (RESTART_VALIDATE_READ);
			}
			/* Verify that version has not been overwritten (read value has not
			 * yet been added to read set and may have not been checked during
			 * extend) */
			l = ATOMIC_LOAD_ACQ(lock);
			if (l != l2) {
				l = l2;
				goto restart_no_load;
			}
			/* Worked: we now have a good version (version <= tx->end) */
		}
	}
	/* We have a good version: add to read set (update transactions) and return value */

	/* Add address and version to read set */
	if (modedata->r_set.nb_entries == modedata->r_set.size) {
		mtm_allocate_rs_entries(tx, modedata, 1);
	}
	r = &modedata->r_set.entries[modedata->r_set.nb_entries++];
	r->version = version;
	r->lock = lock;

	MTM_DEBUG_PRINT("==> mtm_pwb_load[NOT LOCKED](t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu,v=%lu)\n",
	                tx, (unsigned long)modedata->start, 
	                (unsigned long)modedata->end, 
	                addr,
	                lock,
	                (unsigned long)l,
	                (void *)value,
	                (unsigned long)value,
	                (unsigned long)version);

	return value;
}


/*
 * Called by the CURRENT thread to store a word-sized value.
 */
void mtm_pwb_store(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value)
{
	pwb_write(tx, addr, value, ~(mtm_word_t)0);
}


/*
 * Called by the CURRENT thread to store part of a word-sized value.
 */
void mtm_pwb_store2(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value, mtm_word_t mask)
{
	pwb_write(tx, addr, value, mask);
}



DEFINE_LOAD_BYTES(pwb)
DEFINE_STORE_BYTES(pwb)

FOR_ALL_TYPES(DEFINE_READ_BARRIERS, pwb)
FOR_ALL_TYPES(DEFINE_WRITE_BARRIERS, pwb)
