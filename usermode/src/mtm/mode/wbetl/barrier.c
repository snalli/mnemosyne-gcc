#include "mtm_i.h"
#include "mode/wbetl/wbetl_i.h"
#include "mode/wbetl/barrier.h"


/*
 * Extend snapshot range.
 */
static inline 
int 
wbetl_extend (mtm_tx_t *tx, mode_data_t *modedata)
{
	mtm_word_t now;

	PRINT_DEBUG("==> wbetl_extend(%p[%lu-%lu])\n", tx, 
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
wbetl_write(mtm_tx_t *tx, 
            volatile mtm_word_t *addr, 
            mtm_word_t value,
            mtm_word_t mask)
{
	assert(tx->mode == MTM_MODE_wbetl);
	mode_data_t         *modedata = (mode_data_t *) tx->modedata[tx->mode];
	volatile mtm_word_t *lock;
	mtm_word_t          l;
	mtm_word_t          version;
	w_entry_t           *w;
#if DESIGN == WRITE_BACK_ETL
	w_entry_t           *prev = NULL;
#elif DESIGN == WRITE_THROUGH
	int                 duplicate = 0;
#endif /* DESIGN == WRITE_THROUGH */

	PRINT_DEBUG2("==> mtm_write(t=%p[%lu-%lu],a=%p,d=%p-%lu,m=0x%lx)\n", tx,
	             (unsigned long)modedata->start,
	             (unsigned long)modedata->end, 
	             addr, 
	             (void *)value, 
	             (unsigned long)value,
	             (unsigned long)mask);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

	/* Get reference to lock */
	lock = GET_LOCK(addr);

	/* Try to acquire lock */
restart:
	l = ATOMIC_LOAD_ACQ(lock);
restart_no_load:
	if (LOCK_GET_OWNED(l)) {
		/* Locked */
		/* Do we own the lock? */
#if DESIGN == WRITE_THROUGH
		if (tx == (mtm_tx_t *)LOCK_GET_ADDR(l)) {
			/* Yes */
# ifdef NO_DUPLICATES_IN_RW_SETS
			int i;
			/* Check if address is in write set (a lock may cover multiple addresses) */
			w = modedata->w_set.entries;
			for (i = modedata->w_set.nb_entries; i > 0; i--, w++) {
				if (w->addr == addr) {
					if (mask == 0) {
						return w;
					}
				if (w->mask == 0) {
					/* Remember old value */
					w->value = ATOMIC_LOAD(addr);
					w->mask = mask;
				}
				/* Yes: only write to memory */
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
					value = (ATOMIC_LOAD(addr) & ~mask) | (value & mask);
				}	
				ATOMIC_STORE(addr, value);
				return w;
				}
			}
# endif /* NO_DUPLICATES_IN_RW_SETS */
			/* Mark entry so that we do not drop the lock upon undo */
			duplicate = 1;
			/* Must add to write set (may add entry multiple times) */
			goto do_write;
		}
#elif DESIGN == WRITE_BACK_ETL
		w = (w_entry_t *)LOCK_GET_ADDR(l);
		/* Simply check if address falls inside our write set (avoids non-faulting load) */
		if (modedata->w_set.entries <= w &&
		    w < modedata->w_set.entries + modedata->w_set.nb_entries) 
		{
			/* Yes */
			prev = w;
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
					return prev;
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
# ifdef INTERNAL_STATS
				tx->aborts_reallocate++;
# endif /* INTERNAL_STATS */
				mtm_wbetl_restart_transaction (tx, RESTART_REALLOCATE);
			}
			w = &modedata->w_set.entries[modedata->w_set.nb_entries];
# ifdef READ_LOCKED_DATA
			w->version = version;
# endif /* READ_LOCKED_DATA */
			goto do_write;
		}
#endif /* DESIGN == WRITE_BACK_ETL */
		/* Conflict: CM kicks in */
#if CM == CM_PRIORITY
		if (tx->retries >= cm_threshold) {
			if (LOCK_GET_PRIORITY(l) < tx->priority ||
			    (LOCK_GET_PRIORITY(l) == tx->priority &&
# if DESIGN == WRITE_BACK_ETL
			    l < (mtm_word_t)modedata->w_set.entries
# else /* DESIGN != WRITE_BACK_ETL */
			    l < (mtm_word_t)tx
# endif /* DESIGN != WRITE_BACK_ETL */
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
		mtm_wbetl_restart_transaction (tx, RESTART_LOCKED_WRITE);
	} else {
		/* Not locked */
		/* Handle write after reads (before CAS) */
		version = LOCK_GET_TIMESTAMP(l);
#if DESIGN == WRITE_THROUGH && defined(ROLLOVER_CLOCK)
		if (version == VERSION_MAX) {
			/* Cannot acquire lock on address with version VERSION_MAX: abort */
# ifdef INTERNAL_STATS
			tx->aborts_rollover++;
# endif /* INTERNAL_STATS */
			mtm_wbetl_restart_transaction (tx, RESTART_VALIDATE_WRITE);
		}
#endif /* DESIGN == WRITE_THROUGH && defined(ROLLOVER_CLOCK) */
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
				mtm_wbetl_restart_transaction (tx, RESTART_VALIDATE_WRITE);
			}
		}
		/* Acquire lock (ETL) */
#if DESIGN == WRITE_THROUGH
# if CM == CM_PRIORITY
		if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_ADDR((mtm_word_t)tx, tx->priority)) == 0) {
			goto restart;
		}	
# else /* CM != CM_PRIORITY */
		if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_ADDR((mtm_word_t)tx)) == 0) {
			goto restart;
		}	
# endif /* CM != CM_PRIORITY */
#elif DESIGN == WRITE_BACK_ETL
		if (modedata->w_set.nb_entries == modedata->w_set.size) {
			/* Extend write set (invalidate pointers to write set entries => abort and reallocate) */
			modedata->w_set.size *= 2;
			modedata->w_set.reallocate = 1;
# ifdef INTERNAL_STATS
			tx->aborts_reallocate++;
# endif /* INTERNAL_STATS */
			mtm_wbetl_restart_transaction (tx, RESTART_REALLOCATE);
		}
	    w = &modedata->w_set.entries[modedata->w_set.nb_entries];
# ifdef READ_LOCKED_DATA
		w->version = version;
# endif /* READ_LOCKED_DATA */
# if CM == CM_PRIORITY
		if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_ADDR((mtm_word_t)w, tx->priority)) == 0) {
			goto restart;
		}
# else /* CM != CM_PRIORITY */
		if (ATOMIC_CAS_FULL(lock, l, LOCK_SET_ADDR((mtm_word_t)w)) == 0) {
			goto restart;
		}
# endif /* CM != CM_PRIORITY */
#endif /* DESIGN == WRITE_BACK_ETL */
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
#if DESIGN != WRITE_BACK_ETL
	if (modedata->w_set.nb_entries == modedata->w_set.size) {
		mtm_allocate_ws_entries(tx, modedata, 1);
	}
	w = &modedata->w_set.entries[modedata->w_set.nb_entries++];
#endif /* DESIGN != WRITE_BACK_ETL */
	w->addr = addr;
	w->mask = mask;
	w->lock = lock;
	if (mask == 0) {
		/* Do not write anything */
#ifndef NDEBUG
		w->value = 0;
#endif /* ! NDEBUG */
	} else
#if DESIGN == WRITE_THROUGH
	{
		/* Remember old value */
		w->value = ATOMIC_LOAD(addr);
	}
	/* We store the old value of the lock (timestamp and incarnation) */
	w->version = l;
	w->no_drop = duplicate;
	if (mask == 0) {
		return w;
	}
	if (mask != ~(mtm_word_t)0) {
		value = (w->value & ~mask) | (value & mask);
	}	
	ATOMIC_STORE(addr, value);
#elif DESIGN == WRITE_BACK_ETL
	{
		/* Remember new value */
		if (mask != ~(mtm_word_t)0) {
			value = (ATOMIC_LOAD(addr) & ~mask) | (value & mask);
		}	
		w->value = value;
	}
# ifndef READ_LOCKED_DATA
	w->version = version;
# endif /* ! READ_LOCKED_DATA */
	w->next = NULL;
	if (prev != NULL) {
		/* Link new entry in list */
		prev->next = w;
	}
	modedata->w_set.nb_entries++;
#endif /* DESIGN == WRITE_BACK_ETL */

	return w;
}



/*
 * Called by the CURRENT thread to load a word-sized value.
 */
//static inline
mtm_word_t 
mtm_wbetl_load(mtm_tx_t *tx, volatile mtm_word_t *addr)
{
	assert(tx->mode == MTM_MODE_wbetl);
	mode_data_t         *modedata = (mode_data_t *) tx->modedata[tx->mode];
	volatile mtm_word_t *lock;
	mtm_word_t          l;
	mtm_word_t          l2;
	mtm_word_t          value;
	mtm_word_t          version;
	r_entry_t           *r;
#if CM == CM_PRIORITY || DESIGN == WRITE_BACK_ETL
	w_entry_t           *w;
#endif /* CM == CM_PRIORITY || DESIGN == WRITE_BACK_ETL */
#ifdef READ_LOCKED_DATA
	mtm_word_t id;
	mtm_tx_t *owner;
#endif /* READ_LOCKED_DATA */

	PRINT_DEBUG2("==> mtm_load(t=%p[%lu-%lu],a=%p)\n", tx, 
	             (unsigned long)modedata->start,
	             (unsigned long)modedata->end, addr);

	/* Check status */
	assert(tx->status == TX_ACTIVE);

#if CM == CM_PRIORITY
	if (tx->visible_reads >= vr_threshold && vr_threshold >= 0) {
		/* Visible reads: acquire lock first */
		w = mtm_wbetl_write(tx, addr, 0, 0);
		/* Make sure we did not abort */
		if(tx->status != TX_ACTIVE) {
			return 0;
		}
		assert(w != NULL);
		/* We now own the lock */
# if DESIGN == WRITE_THROUGH
		return ATOMIC_LOAD(addr);
# else /* DESIGN != WRITE_THROUGH */
		return w->mask == 0 ? ATOMIC_LOAD(addr) : w->value;
# endif /* DESIGN != WRITE_THROUGH */
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
#if DESIGN == WRITE_THROUGH
		if (tx == (mtm_tx_t *)LOCK_GET_ADDR(l)) {
			/* Yes: we have a version locked by us that was valid at write time */
			value = ATOMIC_LOAD(addr);
			/* No need to add to read set (will remain valid) */
			PRINT_DEBUG2("==> mtm_load(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu)\n",
			             tx, (unsigned long)modedata->start, 
			             (unsigned long)modedata->end, 
			             addr, 
			             lock,
			             (unsigned long)l,
			             (void *)value,
			             (unsigned long)value);
			return value;
		}
#elif DESIGN == WRITE_BACK_ETL
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
					break;
				}
				if (w->next == NULL) {
				/* No: get value from memory */
				value = ATOMIC_LOAD(addr);
				break;
				}
				w = w->next;
			}
			/* No need to add to read set (will remain valid) */
			PRINT_DEBUG2("==> mtm_load(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu)\n",
			             tx, (unsigned long)modedata->start, 
			             (unsigned long)modedata->end, 
			             addr, 
			             lock, 
			             (unsigned long)l,
			             (void *)value,
			             (unsigned long)value);
			return value;
		}
# ifdef READ_LOCKED_DATA
		if (l == LOCK_UNIT) {
			/* Data modified by a unit store: should not last long => retry */
			goto restart;
		}
		/* Try to read old version: we can safely access write set thanks to epoch-based GC */
		owner = w->tx;
		id = ATOMIC_LOAD_ACQ(&owner->id);
		/* Check that data is not being written */
		if ((id & 0x01) != 0) {
			/* Even identifier while address is locked: should not last long => retry */
			goto restart;
		}
		/* Check that the lock did not change */
		l2 = ATOMIC_LOAD_ACQ(lock);
		if (l != l2) {
			l = l2;
			goto restart_no_load;
		}
		/* Read old version */
		version =  ATOMIC_LOAD(&w->version);
		/* Read data */
		value = ATOMIC_LOAD(addr);
		/* Check that data has not been written */
		if (id != ATOMIC_LOAD_ACQ(&owner->id)) {
			/* Data concurrently modified: a new version might be available => retry */
			goto restart;
		}
		if (version >= modedata->start && 
		    (version <= modedata->end || (tx->can_extend && wbetl_extend(tx, modedata)))) {
			/* Success */
#  ifdef INTERNAL_STATS
			tx->locked_reads_ok++;
#  endif /* INTERNAL_STATS */
			goto add_to_read_set;
		}
		/* Invalid version: not much we can do => fail */
#  ifdef INTERNAL_STATS
		tx->locked_reads_failed++;
#  endif /* INTERNAL_STATS */
# endif /* READ_LOCKED_DATA */
#endif /* DESIGN == WRITE_BACK_ETL */
		/* Conflict: CM kicks in */
		/* TODO: we could check for duplicate reads and get value from read set (should be rare) */
#if CM == CM_PRIORITY
		if (tx->retries >= cm_threshold) {
			if (LOCK_GET_PRIORITY(l) < tx->priority ||
				(LOCK_GET_PRIORITY(l) == tx->priority &&
# if DESIGN == WRITE_BACK_ETL
			    l < (mtm_word_t)modedata->w_set.entries
# else /* DESIGN != WRITE_BACK_ETL */
			    l < (mtm_word_t)tx
# endif /* DESIGN != WRITE_BACK_ETL */
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
		tx->aborts_locked_read++;
#endif /* INTERNAL_STATS */
		mtm_wbetl_restart_transaction (RESTART_LOCKED_READ);
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
			if (!tx->can_extend || !wbetl_extend(tx, modedata)) {
				/* Not much we can do: abort */
#if CM == CM_PRIORITY
				/* Abort caused by invisible reads */
				tx->visible_reads++;
#endif /* CM == CM_PRIORITY */
#ifdef INTERNAL_STATS
				tx->aborts_validate_read++;
#endif /* INTERNAL_STATS */
				mtm_wbetl_restart_transaction (RESTART_VALIDATE_READ);
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

#ifdef READ_LOCKED_DATA
add_to_read_set:
#endif /* READ_LOCKED_DATA */

	/* Add address and version to read set */
	if (modedata->r_set.nb_entries == modedata->r_set.size) {
		mtm_allocate_rs_entries(tx, modedata, 1);
	}
	r = &modedata->r_set.entries[modedata->r_set.nb_entries++];
	r->version = version;
	r->lock = lock;

	PRINT_DEBUG2("==> mtm_load(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu,v=%lu)\n",
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
void mtm_wbetl_store(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value)
{
	wbetl_write(tx, addr, value, ~(mtm_word_t)0);
}


/*
 * Called by the CURRENT thread to store part of a word-sized value.
 */
void mtm_wbetl_store2(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value, mtm_word_t mask)
{
	wbetl_write(tx, addr, value, mask);
}



DEFINE_LOAD_BYTES(wbetl)
DEFINE_STORE_BYTES(wbetl)

FOR_ALL_TYPES(DEFINE_READ_BARRIERS, wbetl)
FOR_ALL_TYPES(DEFINE_WRITE_BARRIERS, wbetl)
