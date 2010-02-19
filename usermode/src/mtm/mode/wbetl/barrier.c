// These two must come before all other includes; they define things like w_entry_t.
#include "mtm_i.h"
#include "mode/wbetl/wbetl_i.h"

#include "mode/common/mask.h"
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


/*!
 * Write the value to an uninitialized write-set entry. This should be done, for
 * instance, if this transaction has not previously written to the address given,
 * by passing an unused write-set entry to this function. This entry is filled with
 * the existing value in memory, with the new value masked on top of it.
 *
 * \param entry the write-set entry that will express this write operation. This
 *  entry must be uninitialized; this routine assumes the mask is zero.
 * \param address the address in memory at which the write occurs.
 * \param value the value being written. This value must be in position within
 *  the word; it will not be shifted to agree with the mask.
 * \param mask identifies which bits of value are to be written in the entry. A
 *  1 in the mask indicates bits in value which will overwrite existing data.
 * \param version is the version identifier associated with this entry. It should
 *  match other entries in the same write set.
 * \param lock is the lock-set entry that indicated the write-set containing
 *  entry.
 *
 * \return the pointer given to entry, where the object is now initialized with
 *  the written address and masked value.
 */
static
w_entry_t* initialize_write_set_entry(w_entry_t* entry,
                                      volatile mtm_word_t *address,
                                      mtm_word_t value,
                                      mtm_word_t mask,
                                      volatile mtm_word_t version,
                                      volatile mtm_word_t* lock)
{	
	/* Add address to write set */
	entry->addr = address;
	entry->lock = lock;
	mask_new_value(entry, address, value, mask);
	#ifndef READ_LOCKED_DATA
		entry->version = version;
	#endif
	entry->next = NULL;

	return entry;
}


/*!
 * Correctly adds a given write-set entry after another in the singly-linked list
 * which composes the write set. Normal singly-linked-list semantics apply,
 * specifically that any sequels to tail will be correctly appended after new_entry.
 *
 * \param tail is the write-set entry after which new_entry will be chained. If this
 *  is NULL, it is assumed that new_entry is the only entry in the list and that it
 *  is reachable by some other list-head pointer.
 * \param new_entry is a correctly-initialized entry. This must not be NULL.
 * \param transaction is the transaction under which the insertion is made. This is
 *  necessary for bookkeeping on the total size of the list.
 */
static
void insert_write_set_entry_after(w_entry_t* new_entry, w_entry_t* tail, mtm_tx_t* transaction)
{
	if (tail != NULL) {
		new_entry->next = tail->next;
		tail->next = new_entry;
	} else {
		new_entry->next = NULL;
	}
		
	mode_data_t* modedata = (mode_data_t *) transaction->modedata[transaction->mode];
	modedata->w_set.nb_entries++;
}


/*!
 * Searches a given linked list of write-set entries for a reference to the given address.
 *
 * \param list_head is the beginning of a singly-linked list of write-set entries
 *  which may or may not contain a reference to addr. This must not be NULL.
 * \param address is an address whose membership in the write-set is in question.
 * \param list_tail if the address in question is not found in the given write-set,
 *  this output parameter is set to the tail of the write-set to aid in the appending
 *  of a new entry. If the given parameter is NULL or a matching entry is located,
 *  this value is undefined.
 *
 * \return NULL, if the address is not referenced in the write set. Otherwise, returns
 *  a pointer to the write-set entry that contained that address.
 */
static
w_entry_t* matching_write_set_entry(w_entry_t* const list_head,
                                    volatile mtm_word_t* address,
                                    w_entry_t** list_tail)
{
	w_entry_t* this_entry = list_head;  // The entry examined in "this" iteration of the loop.
	while (true) {
		if (address == this_entry->addr) {
			// Found a matching entry!
			return this_entry;
		}
		else if (this_entry->next == NULL) {
			// EOL: Could not find a matching write-set entry.
			if (list_tail != NULL)
				*list_tail = this_entry;
			
			return NULL;
		}
		else {
			this_entry = this_entry->next;
		}
	}
}


/*
 * Store a masked value of size less than or equal to a word, creating or
 * updating a write-set entry as necessary.
 *
 * \param tx is the local transaction in the current thread.
 * \param addr is the address to which value is being written.
 * \param value includes the bits which are being written.
 * \param mask determines the relevant bits of value. Only these bits are written
 *  to the write set entry and/or memory.
 *
 * \return If addr is a stack address, this routine returns NULL (stack addresses
 *  are not logged). Otherwise, returns a pointer to the updated write-set entry
 *  reflecting the write.
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
	w_entry_t           *prev = NULL;

	MTM_DEBUG_PRINT("==> mtm_write(t=%p[%lu-%lu],a=%p,d=%p-%lu,m=0x%lx)\n", tx,
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
		w_entry_t *write_set_head;
		write_set_head = (w_entry_t *)LOCK_GET_ADDR(l);
		
		/* Simply check if address falls inside our write set (avoids non-faulting load) */
		if (modedata->w_set.entries <= write_set_head &&
		    write_set_head < modedata->w_set.entries + modedata->w_set.nb_entries) 
		{
			/* The written address is already in our write set. */
			/* Did we previously write the exact same address? */
			w_entry_t* write_set_tail = NULL;
			w_entry_t* matching_entry = matching_write_set_entry(write_set_head, addr, &write_set_tail);
			if (matching_entry != NULL) {
				if (matching_entry->mask != 0)
					mask_new_value(matching_entry, addr, value, mask);
				
				return matching_entry;
			} else {
				if (modedata->w_set.nb_entries == modedata->w_set.size) {
					/* Extend write set (invalidate pointers to write set entries => abort and reallocate) */
					modedata->w_set.size *= 2;
					modedata->w_set.reallocate = 1;
					#ifdef INTERNAL_STATS
						tx->aborts_reallocate++;
					#endif
					
					mtm_wbetl_restart_transaction (tx, RESTART_REALLOCATE);  // Does not return!
				} else {
					// Build a new write set entry
					w = &modedata->w_set.entries[modedata->w_set.nb_entries];
					version = write_set_tail->version;  // Get version from previous write set entry (all
					                                    // entries in linked list have same version)
					w_entry_t* initialized_entry = initialize_write_set_entry(w, addr, value, mask, version, lock);
					
					// Add entry to the write set
					insert_write_set_entry_after(write_set_tail, initialized_entry, tx);					
					return initialized_entry;
				}
			}
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
		mtm_wbetl_restart_transaction (tx, RESTART_LOCKED_WRITE);
	} else {
		/* This region has not been locked by this thread. */
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
				mtm_wbetl_restart_transaction (tx, RESTART_VALIDATE_WRITE);
			}
		}
		
		/* Acquire lock (ETL) */
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
		
		initialize_write_set_entry(w, addr, value, mask, version, lock);
		insert_write_set_entry_after(prev, w, tx);
		return w;
	}
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
		#ifdef ENABLE_ISOLATION
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
		#endif /* ENABLE_ISOLATION */
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