#include <mtm_i.h>

#define LOCAL_LOG_SIZE 16384

struct mtm_local_undo_entry_s {
  void   *addr;
  size_t len;
  char   *saved;
};

/*
 * Layout of the local undo log
 *
 *
 *  +------------+
 *  |            | <--+  <--   local_undo->buf
 *  |            |    |
 *  |            |    |
 *  |            |    |
 *  |            |    |
 *  +------------+    |   _
 *  |   addr     |    |    |  
 *  |   len      |    |     >  mtm_local_undo_entry_t
 *  |   saved    | ---+   _|
 *  +------------+        
 *  |            | <--+
 *  |            |    |
 *  |            |    |
 *  +------------+    |
 *  |   addr     |    |  <--   local_undo->last_entry
 *  |   len      |    |
 *  |   saved    | ---+
 *  +------------+
 *
 *
 * Rollback proceeds backwards first starting at local_undo->last_entry then 
 * finding the previous mtm_local_undo_entry_t by doing arithmetic on saved 
 * and len.
 */

static void
local_allocate (mtm_tx_t *tx, int extend)
{
	mtm_local_undo_t *local_undo = &tx->local_undo;

	if (extend) {
		/* Extend read set */
		local_undo->size *= 2;
		PRINT_DEBUG2("==> reallocate read set (%p[%lu-%lu],%d)\n", tx, 
		             (unsigned long)data->start, 
		             (unsigned long)data->end, 
		             data->r_set.size);
		if ((local_undo->buf = 
		     (char *)realloc((void*) local_undo->buf, 
		                     local_undo->size)) == NULL) 
		{
			perror("realloc");
			exit(1);
		}
	} else {
		if ((local_undo->buf = 
		     (char *)malloc(local_undo->size)) == NULL) 
		{
			perror("malloc");
			exit(1);
		}
	}
}


void
mtm_local_init(mtm_tx_t *tx)
{
	mtm_local_undo_t *local_undo = &tx->local_undo;

	local_undo->size = LOCAL_LOG_SIZE;
	local_undo->last_entry = NULL;
	local_undo->n = 0;
	local_allocate(tx, 0);
}


void
mtm_local_commit (mtm_tx_t *tx)
{
	mtm_local_undo_t *local_undo = &tx->local_undo;

	local_undo->last_entry = NULL;
	local_undo->n = 0;
}


void
mtm_local_rollback (mtm_tx_t *tx)
{
	mtm_local_undo_t       *local_undo = &tx->local_undo;
	mtm_local_undo_entry_t *local_undo_entry;
	char                   *buf;
	uintptr_t              entryp;
	void                   *addr;
    uintptr_t              *sp = (uintptr_t *) tx->jb.spSave;
    uintptr_t              *current_sp = get_stack_pointer();
 
	local_undo_entry = local_undo->last_entry;
	while(local_undo_entry) {
		/* 
		 * Make sure I don't corrupt the stack I am operating on. 
		 * See Wang et al [CGO'07] for more information. 
		 */
		addr = local_undo_entry->addr;
		if (sp+1 < (uintptr_t*) addr || ((uintptr_t*) addr) <= current_sp) {
			memcpy (addr, local_undo_entry->saved, local_undo_entry->len);
		}
		/* Get next local_undo_entry */
		entryp = (uintptr_t) local_undo_entry - local_undo_entry->len - sizeof(mtm_local_undo_entry_t);
		if (entryp > (uintptr_t) local_undo->buf) {
			local_undo_entry = (mtm_local_undo_entry_t *) entryp;
		} else {
			local_undo_entry = NULL;
		}
	}
}


static
void _ITM_CALL_CONVENTION
log_arbitrarily (mtm_tx_t *tx, const volatile void *ptr, size_t len)
{
	mtm_local_undo_t       *local_undo = &tx->local_undo;
	mtm_local_undo_entry_t *local_undo_entry;
	char                   *buf;

	if ((local_undo->n + len + sizeof(mtm_local_undo_entry_t)) > local_undo->size) {
		local_allocate(tx, 1);	
	}
	
	buf = &local_undo->buf[local_undo->n];
	local_undo_entry = (mtm_local_undo_entry_t *) &local_undo->buf[local_undo->n + len];
	local_undo->n += (len + sizeof(mtm_local_undo_entry_t));
	local_undo_entry->addr = (void*) ptr;
	local_undo_entry->len = len;
	local_undo_entry->saved = buf;

	memcpy (local_undo_entry->saved, (const void*) ptr, len);

	local_undo->last_entry = local_undo_entry;
}


# define DEFINE_LOG_BARRIER(name, type, encoding)                                \
void _ITM_CALL_CONVENTION mtm_##name##L##encoding (mtm_tx_t *tx, const type *ptr)\
{ log_arbitrarily (TXARGS ptr, sizeof (type)); }


FOR_ALL_TYPES(DEFINE_LOG_BARRIER, local_)
void  _ITM_CALL_CONVENTION mtm_local_LB (mtm_tx_t *tx, volatile const void *ptr, size_t len)
{ log_arbitrarily (TXARGS ptr, len); }
