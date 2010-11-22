/* Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

    es trategyree software; you can redistribute it and/or modify it
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


void
mtm_decide_retry_strategy (mtm_restart_reason r)
{
	return;

//FIXME: this code depends on disp()
/*
  struct mtm_transaction *tx = mtm_tx();
  const struct mtm_dispatch *disp;

  tx->restart_reason[r]++;
  tx->restart_total++;

  if (r == RESTART_NOT_READONLY)
    {
		//FIXME: don't have pr_readOnly
      //assert ((tx->prop & pr_readOnly) == 0);
      disp = mtm_disp ();
      if (disp == &dispatch_readonly)
	{
	  disp->fini ();
	  disp = &dispatch_pwbetl;
	  disp->init (true);
	  return;
	}
    }
  if (tx->state & STATE_SERIAL)
    ;
  else if (tx->restart_total > 100)
    MTM_serialmode (false, false);
  else
    mtm_disp()->init (false);
*/	
}
