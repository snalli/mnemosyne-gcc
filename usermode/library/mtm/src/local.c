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

#include <mtm_i.h>

#define LOCAL_LOG_SIZE (256*1024)

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
    uintptr_t              *sp;
	memcpy(&sp, &(tx->jb), sizeof(uintptr_t)); /* Stack pointer is in the first 8 bytes */
    uintptr_t              *current_sp = get_stack_pointer();
 
	local_undo_entry = local_undo->last_entry;
	while(local_undo_entry) {
		/* 
		 * Make sure I don't corrupt the stack I am operating on. 
		 * See Wang et al [CGO'07] for more information. 
		 */
		addr = local_undo_entry->addr;
		if (sp+1 < (uintptr_t*) addr || ((uintptr_t*) addr) <= current_sp) {
			PM_MEMCPY(addr, local_undo_entry->saved, local_undo_entry->len);
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

	PM_MEMCPY(local_undo_entry->saved, (const void*) ptr, len);

	local_undo->last_entry = local_undo_entry;
}


# define DEFINE_LOG_BARRIER(name, type, encoding)                       \
void _ITM_CALL_CONVENTION _ITM_##name##L##encoding (const type *ptr)	\
{ 									\
	mtm_tx_t *tx = mtm_get_tx();					\
	log_arbitrarily (tx, ptr, sizeof (type)); 			\
}


FOR_ALL_TYPES(DEFINE_LOG_BARRIER, local_)

void _ITM_CALL_CONVENTION 
mtm_local_LB (mtm_tx_t *tx, const void *ptr, size_t len)
{ 
	log_arbitrarily (TXARGS ptr, len); 
}
