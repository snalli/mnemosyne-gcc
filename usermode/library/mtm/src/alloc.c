/* Copyright (C) 2009 Free Software Foundation, Inc.
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


/* This is extra data attached to each node of an AA tree.  */
typedef struct mtm_alloc_action
{
	void (*free_fn)(void *);
	size_t size;
	bool allocated;
} mtm_alloc_action;


void
mtm_alloc_record_allocation (void *ptr, size_t size, void (*free_fn)(void *))
{
	mtm_tx_t *tx = mtm_get_tx ();
	mtm_alloc_action  *a;
	uintptr_t         iptr = (uintptr_t) ptr;

	a = aa_find (tx->alloc_actions, iptr);
	if (a == NULL) {
		a = aa_insert (&tx->alloc_actions, iptr, sizeof (*a));
	}
	a->free_fn = free_fn;
	a->size = size;
	a->allocated = true;
}

void
mtm_alloc_forget_allocation (void *ptr, void (*free_fn)(void *))
{
	mtm_tx_t *tx = mtm_get_tx ();
	mtm_alloc_action  *a;
	uintptr_t         iptr = (uintptr_t) ptr;

	a = aa_find (tx->alloc_actions, iptr);
	if (a == NULL) {
		a = aa_insert (&tx->alloc_actions, iptr, sizeof (*a));
	}

	a->free_fn = free_fn;
	a->size = 0;
	a->allocated = false;
}

size_t
mtm_alloc_get_allocation_size (void *ptr)
{
	mtm_tx_t *tx = mtm_get_tx ();
	mtm_alloc_action  *a;
	uintptr_t         iptr = (uintptr_t) ptr;

	a = aa_find (tx->alloc_actions, iptr);
	return a ? a->size : 0;
}

static void
commit_allocations_1 (aa_key key, void *node_data, void *cb_data)
{
	void             *ptr = (void *)key;
	mtm_alloc_action *a = node_data;
	uintptr_t        revert_p = (uintptr_t) cb_data;

	if (a->allocated == revert_p) {
		a->free_fn (ptr);
	}	
}

void
mtm_alloc_commit_allocations (bool revert_p)
{
	aa_traverse (mtm_get_tx()->alloc_actions, commit_allocations_1,
	             (void *)(uintptr_t)revert_p);
}
