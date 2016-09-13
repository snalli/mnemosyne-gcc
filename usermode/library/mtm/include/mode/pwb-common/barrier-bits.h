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

/**
 * \file barrier-bits_i.h
 *
 * \brief Private header file for write-back barrier bits.
 *
 *
 */

#include <cm.h>
#include <rwset.h>
#include <mask.h>


#ifndef _PWB_COMMON_BARRIER_BITS_JKI671_H
#define _PWB_COMMON_BARRIER_BITS_JKI671_H


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
 * \param transaction is the transaction under which the insertion is made. This is
 *  necessary for bookkeeping on the total size of the list.
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
                                      volatile mtm_word_t* lock,
                                      int is_nonvolatile)
{	
	/* Add address to write set */
	entry->addr = address;
	entry->lock = lock;
	entry->mask = 0x0;
	mask_new_value(entry, address, value, mask);
	entry->version = version;
	entry->next = NULL;
	entry->next_cache_neighbor = NULL;
	entry->is_nonvolatile = is_nonvolatile;
	
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
 * \param cache_neigbor is a write entry whose address places it in the same cache line
 *  as new_entry. The new cacheline will be appended after 
 */
static
void insert_write_set_entry_after(w_entry_t* new_entry, 
                                  w_entry_t* tail, 
                                  mtm_tx_t* transaction, 
                                  w_entry_t* cache_neighbor)
{
	/* Append the entry to the list. */
	if (tail != NULL) {
		new_entry->next = tail->next;
		tail->next = new_entry;
	} else {
		new_entry->next = NULL;
	}
	
	/* Attach the new entry to others in the same cache block/line. */
	if (cache_neighbor != NULL) {
		new_entry->next_cache_neighbor = cache_neighbor->next_cache_neighbor;
		cache_neighbor->next_cache_neighbor = new_entry;
	} else {
		new_entry->next_cache_neighbor = NULL;
	}
	
	/* Update the total number of entries. */
	mode_data_t* modedata = (mode_data_t *) transaction->modedata[transaction->mode];
	modedata->w_set.nb_entries++;

	/* Write the new entry to the persistent TM log as well? */
	if (new_entry->is_nonvolatile) {
		M_TMLOG_WRITE(transaction->pcm_storeset, modedata->ptmlog, (uintptr_t) new_entry->addr, new_entry->value, new_entry->mask);
	}
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
 * \param last_cache_neighbor is written with the last entry discovered whose address
 *  would be stored in the same cache line. This may be useful if entries are flushed
 *  to persistent storage by cache line (instead of by word). This parameter may be NULL.
 *
 * \return NULL, if the address is not referenced in the write set. Otherwise, returns
 *  a pointer to the write-set entry that contained that address.
 */
static inline
w_entry_t *
matching_write_set_entry(w_entry_t* const list_head,
                         volatile mtm_word_t* address,
                         w_entry_t** list_tail,
                         w_entry_t** last_cache_neighbor)
{
	w_entry_t* this_entry = list_head;  // The entry examined in "this" iteration of the loop.
	while (true) {
		if (last_cache_neighbor != NULL  &&  BLOCK_ADDR(address) == BLOCK_ADDR(this_entry->addr))
			*last_cache_neighbor = this_entry;
		
		if (address == this_entry->addr) {
			// Found a matching entry!
			return this_entry;
		}
		else if (this_entry->next == NULL) {
			// EOL: Could not find a matching write-set entry.
			if (list_tail != NULL) {
				*list_tail = this_entry;
			}
			return NULL;
		}
		else {
			this_entry = this_entry->next;
		}
	}
}


/**
 * \brief Store a masked value of size less than or equal to a word, creating or
 * updating a write-set entry as necessary.
 * 
 * If isolation is disabled (!enable_isolation) then instead of keeping new 
 * values in the global lock table, we keep them in a private hash table. 
 * The structure of the private table is the same as the global one's to 
 * keep code changes minimal; its size may be different.
 *
 * Private pseudo-locks are not actually locks but a hack to keep the code 
 * that deals with version-management common between the isolation and 
 * no-isolation path. Since no two threads may compete for these pseudo-locks, 
 * we don't need to use CAS to atomically acquire them but instead we can just 
 * set them using a normal STORE. The set-operation may be done together with the
 * assignment of the hash-table entry to point to the head of the linked list.
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
pwb_write_internal(mtm_tx_t *tx, 
                   volatile mtm_word_t *addr, 
                   mtm_word_t value,
                   mtm_word_t mask,
		           int enable_isolation)
{
	assert(tx->mode == MTM_MODE_pwbnl || tx->mode == MTM_MODE_pwbetl);
	mode_data_t         *modedata = (mode_data_t *) tx->modedata[tx->mode];
	volatile mtm_word_t *lock;
	mtm_word_t          l;
	mtm_word_t          version;
	w_entry_t           *w;
	w_entry_t           *write_set_tail = NULL;
	int                 ret;
	int                 access_is_nonvolatile;

	MTM_DEBUG_PRINT("==> pwb_write(t=%p[%lu-%lu],a=%p,d=%llX-%llu,m=0x%llX)\n", tx,
	                (unsigned long)modedata->start,
	                (unsigned long)modedata->end, 
	                addr, 
	                (unsigned long long)value, 
	                (unsigned long long)value,
	                (unsigned long long)mask);
	/* Check status */
	//assert(tx->status == TX_ACTIVE);
	if (tx->status != TX_ACTIVE) {
		assert(0);
	}

#if 0
/* ENABLES NULL BARRIERS */
	{
			mtm_word_t prev_value;
			prev_value = ATOMIC_LOAD(addr);
			if (mask == 0) {
				return NULL;
			}
			if (mask != ~(mtm_word_t)0) {
				value = (prev_value & ~mask) | (value & mask);
			}	

			/* 
			 * Do eager version management for stack data because ICC does not
			 * like lazy version management for stack.
			 *
			 * But if there could not be user initiated or concurrency control 
			 * aborts then don't need to perform version management.
			 */
			ATOMIC_STORE(addr, value);
			return NULL;
	}
#endif

	
	/* Check whether access is to volatile or non-volatile memory */
	if (((uintptr_t) addr >= PSEGMENT_RESERVED_REGION_START &&
	     (uintptr_t) addr < (PSEGMENT_RESERVED_REGION_START + PSEGMENT_RESERVED_REGION_SIZE)))
	{
		access_is_nonvolatile = 1;
	} else {
		access_is_nonvolatile = 0;

		/* Is it a stack access? */
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

			/* 
			 * Do eager version management for stack data because ICC does not
			 * like lazy version management for stack.
			 *
			 * But if there could not be user initiated or concurrency control 
			 * aborts then don't need to perform version management.
			 */
			if (enable_isolation) {
				mtm_local_LB(tx, (void *) addr, sizeof(mtm_word_t));
			} else {
#ifdef ALLOW_ABORTS
				mtm_local_LB(tx, (void *) addr, sizeof(mtm_word_t));
#endif
			}
			ATOMIC_STORE(addr, value);
			return NULL;
		}

		/* 
		 * Normal volatile access
		 * 
		 * If there could not be user initiated or concurrency control aborts. 
 		 * then don't need to perform version management. However this path
		 * cannot be merged into the stack non-version management path because
		 * we must write only the bytes covered by the mask. We could use the
		 * the same function for writing the stack but writing using a mask
		 * is a slower operation that's why we optimized the stack path.
		 */
		if (!enable_isolation) {
#if !defined(ALLOW_ABORTS)
			PCM_WB_STORE_MASKED(tx->pcm_storeset, addr, value, mask);
			return NULL;
#endif
		}
	}

	
#ifdef _M_STATS_BUILD
	m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, writes, 1);
	if (access_is_nonvolatile) {
		m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, nvwrites, 1);
	} else {
		m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, vwrites, 1);
	}
#endif	

	/* Get reference to lock */
	if (enable_isolation) {
		lock = GET_LOCK(addr);
	} else {
		/* Since isolation is off, go through the private pseudo-lock hash table 
		 * instead of the global one. Since it's private, there is no notion of locking
		 * so it might be confusing why we assign the hash table entry to a 
		 * variable lock. The reason is that besides locking, the logic behind
		 * the private table is the same as the global table's logic. So this
		 * helps us keep most of the path the same and avoid code changes.
		 */
		lock = PRIVATE_GET_LOCK(tx, addr);
	}

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
			/* The written address already hashes into our write set. */
			/* Did we previously write the exact same address? */
			w_entry_t* write_set_tail = NULL;
			w_entry_t* last_entry_in_same_cache_block = NULL;
			w_entry_t* matching_entry = matching_write_set_entry(write_set_head, addr, &write_set_tail, &last_entry_in_same_cache_block);
			if (matching_entry != NULL) {
				if (matching_entry->mask != 0) {
					mask_new_value(matching_entry, addr, value, mask);
					/* Write out the entry to the persistent TM log? */
					if (access_is_nonvolatile) {
						M_TMLOG_WRITE(tx->pcm_storeset, modedata->ptmlog, (uintptr_t) matching_entry->addr, matching_entry->value, matching_entry->mask);
					}	
				}
				return matching_entry;
			} else {
				if (modedata->w_set.nb_entries == modedata->w_set.size) {
					/* Extend write set (invalidate pointers to write set entries => abort and reallocate) */
					modedata->w_set.size *= 2;
					modedata->w_set.reallocate = 1;
#ifdef _M_STATS_BUILD
					m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
					#ifdef INTERNAL_STATS
						tx->aborts_reallocate++;
					#endif
					
					mtm_pwb_restart_transaction (tx, RESTART_REALLOCATE);  // Does not return!
				} else {
#ifdef _M_STATS_BUILD
					m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, writes_distinct, 1);
					if (access_is_nonvolatile) {
						m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, nvwrites_distinct, 1);
					} else {
						m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, vwrites_distinct, 1);
					}
#endif					
					// Build a new write set entry
					w = &modedata->w_set.entries[modedata->w_set.nb_entries];
					version = write_set_tail->version;  // Get version from previous write set entry (all
					                                    // entries in linked list have same version)
					w_entry_t* initialized_entry = initialize_write_set_entry(w, addr, value, mask, version, lock, access_is_nonvolatile);

	
					// Add entry to the write set
					insert_write_set_entry_after(initialized_entry, write_set_tail, tx, last_entry_in_same_cache_block);					
					return initialized_entry;
				}
			}
		}
		/* If isolation is off and the pseudo-lock was set then we should have already 
		 * found a written-back value entry and never reach here. */
		assert(enable_isolation);

		/* Conflict: CM kicks in */
		ret = cm_conflict(tx, lock, &l);
		switch (ret) {
			case CM_RESTART:
				goto restart;
			case CM_RESTART_NO_LOAD:
				goto restart_no_load;
			case CM_RESTART_LOCKED:
				/* Abort */
#ifdef _M_STATS_BUILD
				m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
#ifdef INTERNAL_STATS
				tx->aborts_locked_write++;
#endif /* INTERNAL_STATS */
				mtm_pwb_restart_transaction(tx, RESTART_LOCKED_WRITE);
		}
		/* Should never reach here. */
		assert(0);
	} else {
		/* This region has not been locked by this thread. */
		if (enable_isolation) {
			/* Handle write after reads (before CAS) */
			version = LOCK_GET_TIMESTAMP(l);

			if (version > modedata->end) {
				/* We might have read an older version previously */
				if (!tx->can_extend || mtm_has_read(tx, modedata, lock) != NULL) {
					/* Read version must be older (otherwise, tx->end >= version) */
					/* Not much we can do: abort */
					/* Abort caused by invisible reads */
					cm_visible_read(tx);
#ifdef _M_STATS_BUILD
					m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
#ifdef INTERNAL_STATS
					tx->aborts_validate_write++;
#endif /* INTERNAL_STATS */
					mtm_pwb_restart_transaction(tx, RESTART_VALIDATE_WRITE);
				}
			}
		}
		
		/* Acquire lock (ETL) */
		if (modedata->w_set.nb_entries == modedata->w_set.size) {
			/* Extend write set (invalidate pointers to write set entries => abort and reallocate) */
			modedata->w_set.size *= 2;
			modedata->w_set.reallocate = 1;
#ifdef _M_STATS_BUILD
			m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
# ifdef INTERNAL_STATS
			tx->aborts_reallocate++;
# endif /* INTERNAL_STATS */
			mtm_pwb_restart_transaction (tx, RESTART_REALLOCATE);
		}
	    w = &modedata->w_set.entries[modedata->w_set.nb_entries];
		if (enable_isolation) {
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
		} else {
			/* Don't need a CAS; just use a regular STORE. */
			/* We also set the lock bit to ensure that the next write will 
			 * see the write entry as valid. */
			*lock = LOCK_SET_ADDR((mtm_word_t)w);
		}
		
		w_entry_t* initialized_entry = 	initialize_write_set_entry(w, addr, value, mask, version, lock, access_is_nonvolatile);
		insert_write_set_entry_after(initialized_entry, write_set_tail, tx, NULL);					
#ifdef _M_STATS_BUILD
		m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, writes_distinct, 1);
		if (access_is_nonvolatile) {
			m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, nvwrites_distinct, 1);
		} else {
			m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, vwrites_distinct, 1);
		}
#endif		
		return w;
	}
}


static inline
mtm_word_t 
pwb_load_internal(mtm_tx_t *tx, volatile mtm_word_t *addr, int enable_isolation)
{
	assert(tx->mode == MTM_MODE_pwbnl || tx->mode == MTM_MODE_pwbetl);
	mode_data_t         *modedata = (mode_data_t *) tx->modedata[tx->mode];
	volatile mtm_word_t *lock;
	mtm_word_t          l;
	mtm_word_t          l2;
	mtm_word_t          value;
	mtm_word_t          version;
	r_entry_t           *r;
	w_entry_t           *w;
	int                 ret;

	MTM_DEBUG_PRINT("==> mtm_pwb_load(t=%p[%lu-%lu],a=%p)\n", tx, 
	                (unsigned long)modedata->start,
	                (unsigned long)modedata->end, 
	                addr);

	/* Check status */
	if (tx->status != TX_ACTIVE) {
		printf("%p, tx->status = %d\n", tx, tx->status);
	}
	assert(tx->status == TX_ACTIVE);

#if 0
/* ENABLES NULL BARRIERS */

			value = ATOMIC_LOAD(addr);
			return value;
#endif

	/* Check whether access is to volatile or non-volatile memory */
	if (((uintptr_t) addr >= PSEGMENT_RESERVED_REGION_START &&
	     (uintptr_t) addr < (PSEGMENT_RESERVED_REGION_START + PSEGMENT_RESERVED_REGION_SIZE)))
	{
		/* Access is non-volatile */
		/* Fall through */
	} else {
		/* Is it a stack access? */
		if ((uintptr_t) addr <= tx->stack_base && 
			(uintptr_t) addr > tx->stack_base - tx->stack_size)
		{
			value = ATOMIC_LOAD(addr);
			return value;
		}

		/* 
		 * Normal volatile access
		 *
		 * If there could not be user initiated or concurrency control aborts. 
 		 * then we don't perform version management for volatile data, so we 
		 * just read the value directly from memory. 
		 */
		if (!enable_isolation) {
#if !defined(ALLOW_ABORTS)
			value = ATOMIC_LOAD(addr);
			return value;
#endif
		}
	}


	if (enable_isolation) {
		/* Check with contention manager whether to upgrade to write lock. */
		if (cm_upgrade_lock(tx)) {
			w = pwb_write_internal(tx, addr, 0, 0, enable_isolation);
			/* Make sure we did not abort */
			if(tx->status != TX_ACTIVE) {
				return 0;
			}
			assert(w != NULL);
			/* We now own the lock */
			return w->mask == 0 ? ATOMIC_LOAD(addr) : w->value;
		}
	}

	/* Get reference to lock */
	if (enable_isolation) {
		lock = GET_LOCK(addr);
	} else {
		/* See comments under pwb_write. */
		lock = PRIVATE_GET_LOCK(tx, addr);
	}


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
		/* If isolation is off and the pseudo-lock was set then we should have already 
		 * found a written-back value entry and never reach here. */
		assert(enable_isolation);

		/* Conflict: CM kicks in */
		/* TODO: we could check for duplicate reads and get value from read set (should be rare) */
		ret = cm_conflict(tx, lock, &l);
		switch (ret) {
			case CM_RESTART:
				goto restart;
			case CM_RESTART_NO_LOAD:
				goto restart_no_load;
			case CM_RESTART_LOCKED:
				/* Abort */
#ifdef _M_STATS_BUILD
				m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
#ifdef INTERNAL_STATS
				tx->aborts_locked_write++;
#endif /* INTERNAL_STATS */
				mtm_pwb_restart_transaction(tx, RESTART_LOCKED_READ);
		}
		/* Should never reach here. */
		assert(0);
	} else {
		/* Not locked */
		value = ATOMIC_LOAD_ACQ(addr);
		if (enable_isolation) {
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
					/* Abort caused by invisible reads */
					cm_visible_read(tx);
#ifdef _M_STATS_BUILD
					m_stats_statset_increment(mtm_statsmgr, tx->statset, XACT, aborts, 1);
#endif					
#ifdef INTERNAL_STATS
					tx->aborts_validate_read++;
#endif /* INTERNAL_STATS */
					mtm_pwb_restart_transaction (tx, RESTART_VALIDATE_READ);
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
	}

	/* We have a good version: add to read set (update transactions) and return value */
	if (enable_isolation) {
		/* Add address and version to read set */
		if (modedata->r_set.nb_entries == modedata->r_set.size) {
			mtm_allocate_rs_entries(tx, modedata, 1);
		}
		r = &modedata->r_set.entries[modedata->r_set.nb_entries++];
		r->version = version;
		r->lock = lock;
	}

	MTM_DEBUG_PRINT("==> mtm_pwb_load(t=%p[%lu-%lu],a=%p,l=%p,*l=%lu,d=%p-%lu,v=%lu)\n",
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


#endif /* _PWB_COMMON_BARRIER_BITS_JKI671_H */
