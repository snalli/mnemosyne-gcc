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
#include <setjmp.h>

extern void* mtm_pmalloc(size_t);
extern void* mtm_pmalloc_undo(size_t);
extern void* mtm_pcalloc (size_t, size_t);
extern void mtm_pfree (void*);
extern void mtm_pfree_prepare (void*);
extern void mtm_pfree_commit (void*);
extern void* mtm_prealloc (void *, size_t);
extern size_t mtm_get_obj_size(void*);


struct clone_entry
{
  void *orig, *clone;
};

/* This is a list, not a table per se */
struct clone_table
{
  struct clone_entry *table;
  size_t size;
  struct clone_table *next;
};

static struct clone_table *all_tables;

static void *
find_clone (void *ptr)
{
  struct clone_table *table;

  for (table = all_tables; table ; table = table->next)
    {
      struct clone_entry *t = table->table;
      size_t lo = 0, hi = table->size, i;

      // Quick test for whether PTR is present in this table.  
      if (ptr < t[0].orig || ptr > t[hi - 1].orig)
	continue;

      // Otherwise binary search. 
      while (lo < hi)
	{
	  i = (lo + hi) / 2;
	  if (ptr < t[i].orig)
	    hi = i;
	  else if (ptr > t[i].orig)
	    lo = i + 1;
	  else
	    {
	      return t[i].clone;
	    }
	}

      //  Given the quick test above, if we don't find the entry in
      //  this table then it doesn't exist.  
      break;
    }
  return NULL;
}


void * _ITM_CALL_CONVENTION
_ITM_getTMCloneOrIrrevocable (void *ptr)
{
  // if the function (ptr) have a TM version, give the pointer to the TM function 
  // otherwise, set transaction to irrevocable mode
	void *ret = find_clone (ptr);
	if (ret)
		return ret;

  //  TODO Check we are in an active transaction 
  //  if (stm_current_tx() != NULL && stm_is_active(tx))
  //  GCC always use implicit transaction descriptor 
	mtm_tx_t *tx = mtm_get_tx();
	tx->status = TX_IRREVOCABLE;
	return ptr;
}

void * _ITM_CALL_CONVENTION
_ITM_getTMCloneSafe (void *ptr)
{
  void *ret = find_clone(ptr);
  if (ret == NULL) {
    fprintf(stderr, "libitm: cannot find clone for %p\n", ptr);
    abort();
  }
  return ret;
}

static int
clone_entry_compare (const void *a, const void *b)
{
  const struct clone_entry *aa = (const struct clone_entry *)a;
  const struct clone_entry *bb = (const struct clone_entry *)b;

  if (aa->orig < bb->orig)
    return -1;
  else if (aa->orig > bb->orig)
    return 1;
  else
    return 0;
}

void
_ITM_registerTMCloneTable (void *xent, size_t size)
{
  struct clone_entry *ent = (struct clone_entry *)(xent);
  struct clone_table *old, *table;

  table = (struct clone_table *) malloc (sizeof (struct clone_table));
  table->table = ent;
  table->size = size;

  qsort (ent, size, sizeof (struct clone_entry), clone_entry_compare);

  old = all_tables;
  do
    {
      table->next = old;
      // TODO Change to use AtomicOps wrapper 
      old = __sync_val_compare_and_swap (&all_tables, old, table);
    }
  while (old != table);
}

void
_ITM_deregisterTMCloneTable (void *xent)
{
  struct clone_entry *ent = (struct clone_entry *)(xent);
  struct clone_table **pprev = &all_tables;
  struct clone_table *tab;

  // FIXME: we must make sure that no transaction is active at this point. 

  for (pprev = &all_tables;
       tab = *pprev, tab->table != ent;
       pprev = &tab->next)
    continue;
  *pprev = tab->next;

  free (tab);
}

void _ITM_CALL_CONVENTION _ITM_commitTransactionEH(void *exc_ptr)
{
  	mtm_tx_t *tx = mtm_get_tx();	
	void *__src = NULL;
	mtm_pwbetl_commitTransaction(tx, __src);
}


uint32_t _ITM_CALL_CONVENTION MTM_begin_transaction(uint32_t attr, jmp_buf * buf)
{
	uint32_t ret;
	jmp_buf *env = NULL;
	mtm_tx_t *tx = mtm_get_tx();
	if (unlikely(tx == NULL)) {
		tx = mtm_init_thread();
	}
	assert(tx != NULL);
	ret = mtm_pwbetl_beginTransaction_internal(tx, attr, NULL, &env);

  /* Save thread context only when outermost transaction */
  	if (likely(env != NULL))
		memcpy(env, buf, sizeof(jmp_buf)); /* TODO limit size to real size */
  // freud : This is where you intialized the jump buffer. 
  // And then use a code like _ITM_siglongjmp to parse the buffer 
  // and jump to the appropriate routine.

	return ret;
}

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


int _ITM_CALL_CONVENTION
_ITM_versionCompatible (int version)
{
	return version == _ITM_VERSION_NO;
}


const char * _ITM_CALL_CONVENTION
_ITM_libraryVersion (void)
{
	return "Mnemosyne MTM" _ITM_VERSION_NO_STR;
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


_ITM_transaction* _ITM_CALL_CONVENTION
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
	assert(0);
}


void _ITM_CALL_CONVENTION 
_ITM_registerThrownObject(const void *exception_object, size_t s)
{
	assert(0);
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
	assert(0);
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
	assert(0);
	/* This mode is not implemented yet */
	//TODO: Support compiler instructed switching of transaction execution mode
	//assert (state == modeSerialIrrevocable);
	//MTM_serialmode (false, true);
}

void * _ITM_malloc(size_t size)
{
  void *ptr = NULL;
  ptr = malloc(size);
  if(!ptr)
	goto out;

  mtm_tx_t *tx = mtm_get_tx();
  if(tx)
	_ITM_addUserUndoAction(free, ptr);
out:
  return ptr;
}

void * _ITM_calloc(size_t nm, size_t size)
{
  void *ptr = NULL;
  ptr = calloc(nm, size);
  if(!ptr)
	goto out;

  mtm_tx_t *tx = mtm_get_tx();
  if(tx)
	_ITM_addUserUndoAction(free, ptr);
out:
  return ptr;
}   

void _ITM_free(void *ptr)
{   
  mtm_tx_t *tx = mtm_get_tx();
  if (tx) {
    _ITM_addUserCommitAction(free, tx->id, ptr);
    return;
  }
  free(ptr);
}

_ITM_TRANSACTION_PURE
void * _ITM_pmalloc(size_t size)
{
  void *ptr = NULL;
  ptr = mtm_pmalloc(size);
  if(!ptr)
	goto out;

  mtm_tx_t *tx = mtm_get_tx();
  if(tx)
	_ITM_addUserUndoAction(mtm_pmalloc_undo, ptr);
out:
  return ptr;
}

_ITM_TRANSACTION_PURE
void * _ITM_pcalloc(size_t nm, size_t size)
{
  void *ptr = NULL;
  ptr = mtm_pcalloc(nm, size);
  if(!ptr)
	goto out;

  mtm_tx_t *tx = mtm_get_tx();
  if(tx)
	_ITM_addUserUndoAction(mtm_pfree, ptr);
out:
  return ptr;
}   

_ITM_TRANSACTION_PURE
void _ITM_pfree(void *ptr)
{   
  mtm_tx_t *tx = mtm_get_tx();
  if (tx) {
    mtm_pfree_prepare(ptr);
    _ITM_addUserCommitAction(mtm_pfree_commit, tx->id, ptr);
    return;
  }
  mtm_pfree(ptr);
}

/*
_ITM_TRANSACTION_PURE
void * _ITM_prealloc (void * ptr, size_t sz)
{ return mtm_prealloc(ptr, sz); }
*/

_ITM_TRANSACTION_PURE
void * _ITM_prealloc (void * ptr, size_t sz)
{
	if(!ptr)
		return _ITM_pmalloc(sz);
	if(!sz) {
		_ITM_pfree(ptr);
		return NULL;
	}

	size_t obj_size = mtm_get_obj_size(ptr);
	if(obj_size >= sz || obj_size == -1) 
		return ptr;

	assert(obj_size < sz);
	void *buf = _ITM_pmalloc(sz);
	_ITM_memcpyRtWt (buf, ptr, obj_size);
	_ITM_pfree(ptr);

	return buf;
}

