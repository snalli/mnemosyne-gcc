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


/* Wrap: malloc (size_t sz)  */
void * _ITM_CALL_CONVENTION
_ITM_malloc (size_t sz)
{
	void *r = malloc (sz);
	if (r) {
		mtm_alloc_record_allocation (r, sz, free);
	}	
	return r;
}

/* Wrap: calloc (size_t nm, size_t sz)  */
void *
_ITM_calloc (size_t nm, size_t sz)
{
	void *r = calloc (nm, sz);
	if (r) {
		mtm_alloc_record_allocation (r, nm*sz, free);
	}	
	return r;
}

/* Wrap: realloc (void *ptr, size_t sz)  */
void *
_ITM_realloc (void *ptr, size_t sz)
{
	mtm_tx_t *thr = mtm_thr();
	void         *r;


	if (sz == 0) {
		/* If sz == 0, then realloc == free.  */
		if (ptr) {
			mtm_alloc_forget_allocation (ptr, free);
		}
		r = NULL;
	} else if (ptr == NULL) {
		/* If ptr == NULL, then realloc == malloc.  */
		r = malloc (sz);
		if (r) {
			mtm_alloc_record_allocation (r, sz, free);
		}	
    } else if (ptr) {
		/* We may have recorded the size of the allocation earlier in
		 * this transaction.  If so, fine, we can reallocate.  Otherwise
		 * we're stuck and we'll have to go irrevokable.  */
		size_t osz = mtm_alloc_get_allocation_size (ptr);
		if (osz == 0) {
			mtm_serialmode (false, true);
			return realloc (ptr, sz);
		}

		r = malloc (sz);
		if (r) {
			/* ??? We don't really need to get locks, since we know this
			 * memory is local to the transaction.  However, if the STM
			 * method is write-back, we have to be careful to use memory
			 * from the cache if locks were taken.  */
			_ITM_memcpyRnWt (thr, r, ptr, (sz < osz ? sz : osz));
			mtm_alloc_record_allocation (r, sz, free);
			mtm_alloc_forget_allocation (ptr, free);
		}
	}
	return r;
}


/* Wrap:  free (void *ptr)  */
void _ITM_CALL_CONVENTION
_ITM_free (void *ptr)
{
	if (ptr) {
		mtm_alloc_forget_allocation (ptr, free);
	}	
}
