#ifndef _WBETL_BARRIER_H
#define _WBETL_BARRIER_H

typedef struct r_entry
{
	mtm_version version;
	volatile mtm_stmlock *lock;
} r_entry_t;

typedef struct r_set
{
	r_entry_t *entries;
	int nb_entries;
	int size;
} r_set_t;

typedef struct w_entry
{
	struct w_entry *next;
	uintptr_t addr;
	volatile mtm_stmlock *lock;
	mtm_cacheline *value;
	mtm_version version;
} w_entry_t;

typedef struct w_set
{
	w_entry_t *entries;
	int nb_entries;
	int size;
	int reallocate;
} w_set_t;

struct mtm_method
{
	mtm_version start;
	mtm_version end;

	r_set_t r_set;
	w_set_t w_set;

	mtm_cacheline_page *cache_page;
	int n_cache_page;
};

#define RW_SET_SIZE			4096

/* Check if W is one of our write locks.  */
static inline bool
wbetl_local_w_entry_p (mtm_thread_t *td, struct mtm_method *m, w_entry_t *w)
{
	return (m->w_set.entries <= w
	        && w < m->w_set.entries + m->w_set.nb_entries);
}

/* Check if stripe has been read previously.  */

static inline r_entry_t *
wbetl_has_read (mtm_thread_t *td, struct mtm_method *m, volatile mtm_word *lock)
{
	r_entry_t *r;
	int       i;

	r = m->r_set.entries;
	for (i = m->r_set.nb_entries; i > 0; i--, r++) {
		if (r->lock == lock) {
			return r;
		}	
	}

	return NULL;
}

/* Validate read set, i.e. check if all read addresses are still valid now.  */

static bool
wbetl_validate (mtm_thread_t *td, struct mtm_method *m)
{
	r_entry_t *r;
	int       i;
	mtm_word  l;

	__sync_synchronize ();

	r = m->r_set.entries;
	for (i = m->r_set.nb_entries; i > 0; i--, r++) {
		l = *r->lock;
		if (mtm_stmlock_owned_p (l)) {
			w_entry_t *w = mtm_stmlock_get_addr (l);

			if (!wbetl_local_w_entry_p (td, m, w)) {
				return false;
			}	
		} else {
			if (mtm_stmlock_get_version (l) != r->version) {
				return false;
			}
		}
    }

	return true;
}

/* Extend the snapshot range.  */

static bool
wbetl_extend (mtm_thread_t *td, struct mtm_method *m)
{
	mtm_word now = mtm_get_clock ();

	if (wbetl_validate (td, m)) {
		m->end = now;
		return true;
	}

	return false;
}

/* Acquire a write lock on ADDR.  */

static mtm_cacheline *
wbetl_write_lock (mtm_thread_t *td, uintptr_t addr)
{
	volatile mtm_stmlock *lock;
	mtm_stmlock          l;
	mtm_stmlock          l2;
	mtm_version          version;
	w_entry_t            *w;
	w_entry_t            *prev = NULL;
	struct mtm_method    *m;

	m = mtm_tx()->m;
	lock = mtm_get_stmlock (addr);
	l = *lock;

restart_no_load:
	if (mtm_stmlock_owned_p (l)) {
		w = mtm_stmlock_get_addr (l);

		/* Did we previously write the same address?  */
		if (wbetl_local_w_entry_p (td, m, w)) {
			prev = w;
			while (1) {
				if (addr == prev->addr) {
					return prev->value;
				}	
				if (prev->next == NULL) {
					break;
				}
				prev = prev->next;
			}

			/* Get version from previous entry write set.  */
			version = prev->version;

			/* If there's not enough entries, we must reallocate the array,
			   which invalidates all pointers to write set entries, which
			   means we have to restart the transaction.  */
			if (m->w_set.nb_entries == m->w_set.size) {
				m->w_set.size *= 2;
				m->w_set.reallocate = 1;
				mtm_wbetl_restart_transaction (RESTART_REALLOCATE);
			}

			w = &m->w_set.entries[m->w_set.nb_entries];
			goto do_write;
		}

		mtm_wbetl_restart_transaction (RESTART_LOCKED_WRITE);
	} else {
		version = mtm_stmlock_get_version (l);

		/* We might have read an older version previously.  */
		if (version > m->end) {
			if (wbetl_has_read (td, m, lock) != NULL) {
				mtm_wbetl_restart_transaction (RESTART_VALIDATE_WRITE);
			}
		}

		/* Extend write set, aborting to reallocate write set entries.  */
		if (m->w_set.nb_entries == m->w_set.size) {
			m->w_set.size *= 2;
			m->w_set.reallocate = 1;
			mtm_wbetl_restart_transaction (RESTART_REALLOCATE);
		}

		/* Acquire the lock.  */
		w = &m->w_set.entries[m->w_set.nb_entries];
		l2 = mtm_stmlock_set_owned (w);
		l = __sync_val_compare_and_swap (lock, l, l2);
		if (l != l2) {
			goto restart_no_load;
		}	
    }

do_write:
	m->w_set.nb_entries++;
	w->addr = addr;
	w->lock = lock;
	w->version = version;
	w->next = NULL;
	if (prev != NULL) {
		prev->next = w;
	}

	{
		mtm_cacheline_page *page = m->cache_page;
		unsigned           index = m->n_cache_page;
		mtm_cacheline      *line;

		if (page == NULL || index == CACHELINES_PER_PAGE)
		{
			mtm_cacheline_page *npage = mtm_page_alloc ();
			npage->prev = page;
			m->cache_page = page = npage;
			m->n_cache_page = 1;
			index = 0;
		} else {
			m->n_cache_page = index + 1;
		}

		w->value = line = &page->lines[index];
		page->masks[index] = 0;
		mtm_cacheline_copy (line, (const mtm_cacheline *) addr);

		return line;
	}
}

/* Acquire a read lock on ADDR.  */

static mtm_cacheline *
wbetl_read_lock (mtm_thread_t *td, uintptr_t addr, bool after_read)
{
	volatile mtm_stmlock *lock;
	mtm_stmlock          l;
	mtm_stmlock          l2;
	mtm_version          version;
	w_entry_t            *w;
	struct mtm_method    *m;

	m = mtm_tx()->m;
	lock = mtm_get_stmlock (addr);
	l = *lock;

restart_no_load:
	if (mtm_stmlock_owned_p (l)) {
		w = mtm_stmlock_get_addr (l);

		/* Did we previously write the same address?  */
		if (wbetl_local_w_entry_p (td, m, w)) {
			while (1) {
				if (addr == w->addr) {
					return w->value;
				}	
				if (w->next == NULL) {
					return (mtm_cacheline *) addr;
				}
				w = w->next;
			}
		}
		mtm_wbetl_restart_transaction (RESTART_LOCKED_READ);
    }

	version = mtm_stmlock_get_version (l);

	/* If version is no longer valid, re-validate the read set.  */
	if (version > m->end) {
		if (!wbetl_extend (td, m)) {
			mtm_wbetl_restart_transaction (RESTART_VALIDATE_READ);
		}
		/* Verify that the version has not yet been overwritten.  The read
		   value has not yet bee added to read set and may not have been
		   checked during the extend.  */
		__sync_synchronize ();
		l2 = *lock;
		if (l != l2) {
			l = l2;
			goto restart_no_load;
		}
    }

	if (!after_read) {
		r_entry_t *r;

		/* Add the address and version to the read set.  */
		if (m->r_set.nb_entries == m->r_set.size) {
			m->r_set.size *= 2;

			m->r_set.entries = (r_entry_t *)
			realloc (m->r_set.entries, m->r_set.size * sizeof(r_entry_t));
			if (m->r_set.entries == NULL) {
				abort ();
			}
		}
		r = &m->r_set.entries[m->r_set.nb_entries++];
		r->version = version;
		r->lock = lock;
    }

	return (mtm_cacheline *) addr;
}

static mtm_cacheline *
wbetl_after_write_lock (mtm_thread_t *td, uintptr_t addr)
{
	volatile mtm_stmlock *lock;
	mtm_stmlock l;
	w_entry_t *w;
	struct mtm_method *m;

	m = mtm_tx()->m;
	lock = mtm_get_stmlock (addr);

	l = *lock;
	assert (mtm_stmlock_owned_p (l));

	w = mtm_stmlock_get_addr (l);
	assert (wbetl_local_w_entry_p (td, m, w));

	while (1) {
		if (addr == w->addr) {
			return w->value;
		}	
		w = w->next;
	}
}

static mtm_cacheline *
wbetl_R (mtm_thread_t *td, uintptr_t addr)
{
	return wbetl_read_lock (td, addr, false);
}

static mtm_cacheline *
wbetl_RaR (mtm_thread_t *td, uintptr_t addr)
{
	return wbetl_read_lock (td, addr, true);
}

static mtm_cacheline *
wbetl_RaW (mtm_thread_t *td, uintptr_t addr)
{
	return wbetl_after_write_lock (td, addr);
}

static mtm_cacheline *
wbetl_RfW (mtm_thread_t *td, uintptr_t addr)
{
	return wbetl_write_lock (td, addr);
}

static mtm_cacheline_mask_pair
wbetl_W (mtm_thread_t *td, uintptr_t addr)
{
	mtm_cacheline_mask_pair pair;
	pair.line = wbetl_write_lock (td, addr);
	pair.mask = mtm_mask_for_line (pair.line);
	return pair;
}

static mtm_cacheline_mask_pair
wbetl_WaR(mtm_thread_t *td, uintptr_t addr)
{
	return wbetl_W(td, addr);
}

static mtm_cacheline_mask_pair
wbetl_WaW (mtm_thread_t *td, uintptr_t addr)
{
	mtm_cacheline_mask_pair pair;
	pair.line = wbetl_after_write_lock (td, addr);
	pair.mask = mtm_mask_for_line (pair.line);
	return pair;
}


#endif
