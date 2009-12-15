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


typedef struct mtm_local_undo
{
  void *addr;
  size_t len;
  char saved[];
} mtm_local_undo;


void
mtm_commit_local (void)
{
  mtm_transaction_t *tx = mtm_tx();
  mtm_local_undo **local_undo = tx->local_undo;
  size_t i, n = tx->n_local_undo;

  if (n > 0)
    {
      for (i = 0; i < n; ++i)
	free (local_undo[i]);
      tx->n_local_undo = 0;
    }
  if (local_undo)
    {
      free (local_undo);
      tx->local_undo = NULL;
      tx->size_local_undo = 0;
    }
}

void
mtm_rollback_local (void)
{
  mtm_transaction_t *tx = mtm_tx();
  mtm_local_undo **local_undo = tx->local_undo;
  size_t i, n = tx->n_local_undo;

  if (n > 0)
    {
      for (i = n; i-- > 0; )
	{
	  mtm_local_undo *u = local_undo[i];
	  memcpy (u->addr, u->saved, u->len);
	  free (u);
	}
      tx->n_local_undo = 0;
    }
}

static
void _ITM_CALL_CONVENTION
log_arbitrarily (mtm_thread_t *td, const void *ptr, size_t len)
{
  mtm_transaction_t *tx = mtm_tx();
  mtm_local_undo *undo;

  undo = malloc (sizeof (struct mtm_local_undo) + len);
  undo->addr = (void *) ptr;
  undo->len = len;

  if (tx->local_undo == NULL)
    {
      tx->size_local_undo = 32;
      tx->local_undo = malloc (sizeof (undo) * tx->size_local_undo);
    }
  else if (tx->n_local_undo == tx->size_local_undo)
    {
      tx->size_local_undo *= 2;
      tx->local_undo = realloc (tx->local_undo,
				sizeof (undo) * tx->size_local_undo);
    }
  tx->local_undo[tx->n_local_undo++] = undo;

  memcpy (undo->saved, ptr, len);
}


# define DEFINE_LOG_BARRIER(name, type, encoding)                                \
void _ITM_CALL_CONVENTION mtm_##name##L##encoding (mtm_thread_t *td, const type *ptr)      \
{ log_arbitrarily (td, ptr, sizeof (type)); }


FOR_ALL_TYPES(DEFINE_LOG_BARRIER, local_)
void  _ITM_CALL_CONVENTION mtm_local_LB (mtm_thread_t *td, const void *ptr, size_t len)
{ log_arbitrarily (td, ptr, len); }
