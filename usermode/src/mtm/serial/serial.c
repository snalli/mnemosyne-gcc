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


static mtm_cacheline_mask mtm_cacheline_mask_sink;

mtm_cacheline *
mtm_null_read_lock (mtm_thread_t *td, uintptr_t ptr)
{
  return (mtm_cacheline *) ptr;
}

mtm_cacheline_mask_pair
mtm_null_write_lock (mtm_thread_t *td, uintptr_t ptr)
{
  mtm_cacheline_mask_pair pair;
  pair.line = (mtm_cacheline *) ptr;
  pair.mask = &mtm_cacheline_mask_sink;
  return pair;
}

static bool
serial_trycommit (void)
{
  return true;
}

static void
serial_rollback (void)
{
  abort ();
}

static void
serial_init (bool first UNUSED)
{
}

static void
serial_fini (void)
{
}

//FIXME need to hardcode using MACROs
/*
const static struct mtm_dispatch serial_dispatch = 
{
  .R = MTM_null_read_lock,
  .RaR = MTM_null_read_lock,
  .RaW = MTM_null_read_lock,
  .RfW = MTM_null_read_lock,

  .W = MTM_null_write_lock,
  .WaR = MTM_null_write_lock,
  .WaW = MTM_null_write_lock,

  .trycommit = serial_trycommit,
  .rollback = serial_rollback,
  .init = serial_init,
  .fini = serial_fini,
};
*/

/* Put the transaction into serial mode.  */

void
MTM_serialmode (bool initial, bool irrevokable)
{
  mtm_transaction_t *tx = mtm_tx();
  //const struct mtm_dispatch *old_disp;

  if (tx->state & STATE_SERIAL)
    {
      if (irrevokable)
	tx->state |= STATE_IRREVOCABLE;
      return;
    }

  //old_disp = mtm_disp ();
  //set_mtm_disp (&serial_dispatch);

  if (initial)
    mtm_rwlock_write_lock (&mtm_serial_lock);
  else
    {
      mtm_rwlock_write_upgrade (&mtm_serial_lock);
	  /*
      if (old_disp->trycommit ()) {
			old_disp->fini ();
	}  else
	{
	  tx->state = STATE_SERIAL;
	  MTM_restart_transaction (RESTART_VALIDATE_COMMIT);
	}
	*/
    }

  tx->state = STATE_SERIAL | (irrevokable ? STATE_IRREVOCABLE : 0);
}


void _ITM_CALL_CONVENTION
_ITM_changeTransactionMode(mtm_thread_t *td,
                           mtm_thread_tState __mode,
                           const _ITM_srcLocation * __loc)
{
	//FIXME
  //assert (state == modeSerialIrrevocable);
  MTM_serialmode (false, true);
}
