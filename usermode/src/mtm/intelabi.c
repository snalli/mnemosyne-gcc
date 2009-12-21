/**
 * \file intelabi.c
 *
 * \brief Implements the rest of the Intel ABI that is not exported via
 * the VTABLE.
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
	return "Mnemosyne libitm" _ITM_VERSION;
}


_ITM_howExecuting _ITM_CALL_CONVENTION
_ITM_inTransaction (mtm_thread_t *td)
{
	//FIXME
	mtm_transaction_t *tx = mtm_tx();
	if (tx) {
		if (tx->state & STATE_IRREVOCABLE) {
			return inIrrevocableTransaction;
		} else {
			return inRetryableTransaction;
		}
	}
	return outsideTransaction;
}


_ITM_transactionId _ITM_CALL_CONVENTION
_ITM_getTransactionId (mtm_thread_t *td)
{
	mtm_transaction_t *tx = mtm_tx();
	return tx ? tx->id : _ITM_noTransactionId;
}


mtm_thread_t * _ITM_CALL_CONVENTION
_ITM_getTransaction(void)
{
	mtm_thread_t *thr = mtm_thr();
	
	if (thr) {
		return thr;
	}
	
	thr = mtm_init_thread();
	return thr;
}


int _ITM_CALL_CONVENTION
_ITM_getThreadnum (void)
{
	static int   global_num;
	mtm_thread_t *thr = mtm_thr();
	int          num = thr->thread_num;

	if (num == 0) {
		num = __sync_add_and_fetch (&global_num, 1);
		thr->thread_num = num;
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
_ITM_dropReferences (mtm_thread_t * td, const void *start, size_t size)
{
	//TODO
}


void _ITM_CALL_CONVENTION 
_ITM_registerThrownObject(mtm_thread_t *td, 
                          const void *exception_object, size_t s)
{


}


int _ITM_CALL_CONVENTION
_ITM_initializeProcess (void)
{
	int          ret;
	mtm_thread_t *thr;

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
	abort();
}

void _ITM_CALL_CONVENTION
_ITM_addUserCommitAction(mtm_thread_t * __td,
                         _ITM_userCommitFunction fn,
                         _ITM_transactionId tid, 
                         void *arg)
{
	MTM_useraction_addUserCommitAction(__td, fn, tid, arg);
}


void _ITM_CALL_CONVENTION
_ITM_addUserUndoAction(mtm_thread_t * __td,
                       const _ITM_userUndoFunction fn, 
                       void *arg)
{
	MTM_useraction_adduserUndoAction(__td, fn, arg);
}
