/**
 * \file intelabi.c
 *
 * \brief Implements the rest of the Intel ABI that is not exported via
 * the dispatch table.
 */

#include "mtm_i.h"
#include <stdio.h>
#include "init.h"
#include "useraction.h"


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
_ITM_inTransaction (mtm_tx_t *tx)
{
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
_ITM_getTransactionId (mtm_tx_t *td)
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
_ITM_dropReferences (mtm_tx_t * td, const void *start, size_t size)
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
_ITM_addUserCommitAction(mtm_tx_t * __td,
                         _ITM_userCommitFunction fn,
                         _ITM_transactionId tid, 
                         void *arg)
{
	mtm_useraction_addUserCommitAction(__td, fn, tid, arg);
}


void _ITM_CALL_CONVENTION
_ITM_addUserUndoAction(mtm_tx_t * __td,
                       const _ITM_userUndoFunction fn, 
                       void *arg)
{
	mtm_useraction_addUserUndoAction(__td, fn, arg);
}


void _ITM_CALL_CONVENTION
_ITM_changeTransactionMode(mtm_tx_t *td,
                           _ITM_transactionState __mode,
                           const _ITM_srcLocation * __loc)
{
	//TODO: Support compiler instructed switching of transaction execution mode
	//assert (state == modeSerialIrrevocable);
	//MTM_serialmode (false, true);
}
