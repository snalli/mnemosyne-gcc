#include <mtm_i.h>


typedef struct mtm_local_undo
{
  void *addr;
  size_t len;
  char saved[];
} mtm_local_undo;


void
mtm_commit_local (TXPARAM)
{
	TX_GET;
	mtm_local_undo **local_undo = tx->local_undo;
	size_t         i;
	size_t         n = tx->n_local_undo;

	if (n > 0) {
		for (i = 0; i < n; ++i) {
			free (local_undo[i]);
		}	
		tx->n_local_undo = 0;
	}
	if (local_undo)	{
		free (local_undo);
		tx->local_undo = NULL;
		tx->size_local_undo = 0;
	}
}


void
mtm_rollback_local (TXPARAM)
{
	TX_GET;
	mtm_local_undo **local_undo = tx->local_undo;
	size_t         i;
	size_t         n = tx->n_local_undo;

	if (n > 0) {
		for (i = n; i-- > 0; ) {
			mtm_local_undo *u = local_undo[i];
			memcpy (u->addr, u->saved, u->len);
			free (u);
		}
		tx->n_local_undo = 0;
	}
}


static
void _ITM_CALL_CONVENTION
log_arbitrarily (TXPARAMS const void *ptr, size_t len)
{
	TX_GET;
	mtm_local_undo *undo;

	undo = malloc (sizeof (struct mtm_local_undo) + len);
	undo->addr = (void *) ptr;
	undo->len = len;

	if (tx->local_undo == NULL) {
		tx->size_local_undo = 32;
		tx->local_undo = malloc (sizeof (undo) * tx->size_local_undo);
    }
	else if (tx->n_local_undo == tx->size_local_undo) {
		tx->size_local_undo *= 2;
		tx->local_undo = realloc (tx->local_undo,
		                          sizeof (undo) * tx->size_local_undo);
	}
	tx->local_undo[tx->n_local_undo++] = undo;

	memcpy (undo->saved, ptr, len);
}


# define DEFINE_LOG_BARRIER(name, type, encoding)                                \
void _ITM_CALL_CONVENTION mtm_##name##L##encoding (mtm_tx_t *tx, const type *ptr)\
{ log_arbitrarily (TXARGS ptr, sizeof (type)); }


FOR_ALL_TYPES(DEFINE_LOG_BARRIER, local_)
void  _ITM_CALL_CONVENTION mtm_local_LB (mtm_tx_t *tx, const void *ptr, size_t len)
{ log_arbitrarily (TXARGS ptr, len); }
