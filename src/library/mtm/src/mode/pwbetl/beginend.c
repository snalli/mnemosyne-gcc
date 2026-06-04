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

#include "pwb_i.h"
#include "mode/pwb-common/beginend-bits.h"


uint32_t
mtm_pwbetl_beginTransaction_internal (mtm_tx_t *tx, 
                                   uint32_t prop, 
                                   _ITM_srcLocation *srcloc, jmp_buf **__env)
{
	PM_START_TX();
	return beginTransaction_internal (tx, prop, srcloc, 1, __env);
}


void _ITM_CALL_CONVENTION
mtm_pwbetl_rollbackTransaction (mtm_tx_t *tx, const _ITM_srcLocation *loc)
{
	assert ((tx->prop & pr_hasNoAbort) == 0);
	//assert ((mtm_tx()->state & STATE_ABORTING) == 0);

	rollback_transaction (tx);
	//tx->status |= STATE_ABORTING;
}


void _ITM_CALL_CONVENTION
mtm_pwbetl_abortTransaction (mtm_tx_t *tx, 
                            _ITM_abortReason reason,
                            const _ITM_srcLocation *loc)
{
	assert (reason == userAbort || reason == userRetry);
	/* Compiler Bug: pr_hasNoRetry seems to be wrong */
	assert ((reason == userAbort && (tx->prop & pr_hasNoAbort) == 0) || 
	        (reason == userRetry && 1));
	//assert ((tx->state & STATE_ABORTING) == 0);

	if (tx->status & TX_IRREVOCABLE) {
		abort ();
	}	

	if (reason == userAbort) {
		rollback_transaction (tx);
		//pwb_fini (td);

		/* TODO: Implement serial mode to support irrevocable actions such as I/O
		 * 
		 * mtm_rwlock_unlock (&mtm_serial_lock);
		 * 
		 */

		/* TODO: Implement true nesting. Currently we only flatten  nested 
		 * transactions.
		 * 
		 * set_mtm_tx (tx->prev);     
		 * free_tx (td, tx);
		 */

		_ITM_siglongjmp (tx->jb, a_abortTransaction | a_restoreLiveVariables);
	} else if (reason == userRetry) {
		mtm_pwb_restart_transaction(tx, RESTART_USER_RETRY);
	}
}


bool _ITM_CALL_CONVENTION
mtm_pwbetl_tryCommitTransaction (mtm_tx_t *tx, const _ITM_srcLocation *loc)
{
	return trycommit_transaction(tx, 1);
}


void _ITM_CALL_CONVENTION
mtm_pwbetl_commitTransaction(mtm_tx_t *tx, const _ITM_srcLocation *loc)
{
	MTM_DEBUG_PRINT("==> mtm_pwb_commitTransaction(%p)\n", tx);
	if (!trycommit_transaction(tx, 1)) {
		mtm_pwb_restart_transaction(tx, RESTART_VALIDATE_COMMIT);
	}
	if (tx->status == TX_COMMITTED) {
		tx->status = TX_IDLE; //FIXME: Can we set IDLE here ? Don't others depend
		                      // on seeing our state as TX_COMMITTED. For example, 
		                      // do we need to quiesce first? TinySTM 1.0.0 has 
		                      // quiescence support.
	}							  
	PM_END_TX();
	MTM_DEBUG_PRINT("==> mtm_pwb_commitTransaction(%p): DONE\n", tx);
}


void _ITM_CALL_CONVENTION
mtm_pwbetl_commitTransactionToId(mtm_tx_t *tx, 
                                const _ITM_transactionId tid,
                                const _ITM_srcLocation *loc)
{
	//TODO
	assert(0);
}



