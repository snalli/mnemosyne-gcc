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

/**
 * \file gcc-abi.c
 *
 * \brief Implements the GCC ABI.
 */

#include "mtm_i.h"
#include <stdio.h>
#include "init.h"
#include "useraction.h"


void _ITM_CALL_CONVENTION _ITM_abortTransaction(_ITM_abortReason __reason,
                              const _ITM_srcLocation *__src)
{
	mtm_tx_t *tx = mtm_get_tx();
	mtm_pwbetl_abortTransaction(tx, __reason, __src);
}

void _ITM_CALL_CONVENTION _ITM_rollbackTransaction(const _ITM_srcLocation *__src)
{
	mtm_tx_t *tx = mtm_get_tx();	
	mtm_pwbetl_rollbackTransaction(tx, __src);
}

void _ITM_CALL_CONVENTION _ITM_commitTransaction()
{
  	mtm_tx_t *tx = mtm_get_tx();	
	void *__src = NULL;
	mtm_pwbetl_commitTransaction(tx, __src);
}

bool _ITM_CALL_CONVENTION _ITM_tryCommitTransaction(const _ITM_srcLocation *__src)
{
	mtm_tx_t *tx = mtm_get_tx();
	return mtm_pwbetl_tryCommitTransaction(tx, __src);
}

void _ITM_CALL_CONVENTION _ITM_commitTransactionToId(const _ITM_transactionId tid,
                              const _ITM_srcLocation *__src)
{
	mtm_tx_t *tx = mtm_get_tx();
	mtm_pwbetl_commitTransactionToId(tx, tid, __src);
}

/*
TODO : Requires assembly-level hacking
uint32_t
mtm_pwbetl_beginTransaction_internal (mtm_tx_t *tx,
                                   uint32_t prop,
                                   _ITM_srcLocation *srcloc)
{
        PM_START_TX();
        return beginTransaction_internal (tx, prop, srcloc, 1);
}
*/


int _ITM_CALL_CONVENTION
_ITM_versionCompatible (int version)
{
	return version == _ITM_VERSION_NO;
}


const char * _ITM_CALL_CONVENTION
_ITM_libraryVersion (void)
{
	return "Mnemosyne MTM" _ITM_VERSION;
}


_ITM_howExecuting _ITM_CALL_CONVENTION
_ITM_inTransaction ()
{
	mtm_tx_t *tx = mtm_get_tx();
	if (tx && tx->status != TX_IDLE) {
		if (tx->status & TX_IRREVOCABLE) {
			return inIrrevocableTransaction;
		} else {
			return inRetryableTransaction;
		}
	}
	return outsideTransaction;
}


_ITM_transactionId _ITM_CALL_CONVENTION
_ITM_getTransactionId ()
{
	mtm_tx_t *tx = mtm_get_tx();
	return tx ? tx->id : _ITM_noTransactionId;
}


mtm_tx_t * _ITM_CALL_CONVENTION
_ITM_getTransaction(void)
{
	mtm_tx_t *tx = mtm_get_tx();
	
	if (tx) {
		return tx;
	}
	
	tx = mtm_init_thread();
	return tx;
}


int _ITM_CALL_CONVENTION
_ITM_getThreadnum (void)
{
	static int   global_num;
	mtm_tx_t     *tx = mtm_get_tx();
	int          num = (int) tx->thread_num;

	if (num == 0) {
		num = __sync_add_and_fetch (&global_num, 1);
		tx->thread_num = num;
	}

	return num;
}


void _ITM_CALL_CONVENTION ITM_NORETURN
_ITM_error (const _ITM_srcLocation * loc UNUSED, int errorCode UNUSED)
{
	abort ();
}


void _ITM_CALL_CONVENTION 
_ITM_userError (const char *errorStr, int errorCode)
{
	fprintf (stderr, "A TM fatal error: %s (code = %i).\n", errorStr, errorCode);
	exit (errorCode);
}


void _ITM_CALL_CONVENTION 
_ITM_dropReferences (const void *start, size_t size)
{
	//TODO
}


void _ITM_CALL_CONVENTION 
_ITM_registerThrownObject(mtm_tx_t *td, 
                          const void *exception_object, size_t s)
{


}


int _ITM_CALL_CONVENTION
_ITM_initializeProcess (void)
{
	int          ret;
	mtm_tx_t *thr;

	if ((ret = mtm_init_global()) == 0) {
		thr = mtm_init_thread();
		if (thr) {
			return 0;
		} else {
			return -1;
		}
	}
	return ret;
}


void _ITM_CALL_CONVENTION
_ITM_finalizeProcess (void)
{
	mtm_fini_global();
	return;
}


int _ITM_CALL_CONVENTION
_ITM_initializeThread (void)
{
	if (mtm_init_thread()) {
		return 0;
	} else {
		return -1;
	}
}


void _ITM_CALL_CONVENTION
_ITM_finalizeThread (void)
{
	mtm_fini_thread();
}

void _ITM_CALL_CONVENTION
_ITM_registerThreadFinalization (void (_ITM_CALL_CONVENTION * thread_fini_func) (void *), void *arg)
{
	/* Not yet implemented. */
	//abort();
}

void _ITM_CALL_CONVENTION
_ITM_addUserCommitAction(_ITM_userCommitFunction fn,
                         _ITM_transactionId tid, 
                         void *arg)
{
	mtm_tx_t *tx = mtm_get_tx();
	mtm_useraction_addUserCommitAction(tx, fn, tid, arg);
}


void _ITM_CALL_CONVENTION
_ITM_addUserUndoAction(const _ITM_userUndoFunction fn, 
                       void *arg)
{
	mtm_tx_t *tx = mtm_get_tx();
	mtm_useraction_addUserUndoAction(tx, fn, arg);
}


void _ITM_CALL_CONVENTION
_ITM_changeTransactionMode(_ITM_transactionState __mode,
                           const _ITM_srcLocation * __loc)
{
	/* This mode is not implemented yet */
	//TODO: Support compiler instructed switching of transaction execution mode
	//assert (state == modeSerialIrrevocable);
	//MTM_serialmode (false, true);
}
