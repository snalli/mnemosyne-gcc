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

#include "mtm_i.h"
#include "beginend-bits.h"

void ITM_NORETURN
mtm_pwb_restart_transaction (mtm_tx_t *tx, mtm_restart_reason r)
{
	uint32_t actions;
	//fprintf(stderr,"%d %s-%d restart_reason=%d\n",syscall(SYS_gettid),__func__,__LINE__, r);
#if (!defined(ALLOW_ABORTS))
	if (tx->mode == MTM_MODE_pwbnl) {
		assert(0 && "Aborts disabled for current mode pwbnl but want to abort.\n");
	}	
#endif

	if (r == RESTART_REALLOCATE) {
		assert(0 && "Currently we don't support extending the read/write set size");
	}

	rollback_transaction(tx);
	cm_delay(tx);
	/* TODO: decide whether to transition to a different execution mode 
	 * after a restart. For example, transition to serial mode
	 */

	/* Reset field to restart transaction */
	pwb_prepare_transaction(tx);

	actions = a_runInstrumentedCode | a_restoreLiveVariables;
	//if ((tx->prop & pr_uninstrumentedCode) && (tx->status & TX_IRREVOCABLE)) {
	//	actions = a_runUninstrumentedCode | a_restoreLiveVariables;
	//}	
	
	_ITM_siglongjmp (tx->jb, actions);
}


