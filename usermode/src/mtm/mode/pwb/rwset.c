/*!
 * \file
 * 
 */
#include "nonvolatile_write_set.h"

/*!
 * Allocate non-volatile write-set entries to the active transaction.
 *
 * \param tx the transaction receiving the write-set space. Unused.
 * \param data the mode-specific descriptor of the active transaction.
 * \param extend an indicator whether the
 */
static inline 
void 
mtm_allocate_ws_entries_nv(mtm_tx_t *tx, mode_data_t *data, int extend)
{
	/* Because of the current state of the recoverable write-set block, reallocation is not
	   possible. This program cannot be run until the recoverable blocks are
	   somehow extendable. */
	assert(!extend);
	data->w_set_nv = nonvolatile_write_set_next_available();
	
	/* If there were no available write-set blocks, fail immediately. This is a todo
	   item for the seeking of nonvolatile write-set blocks. */
	assert(data->w_set_nv != NULL);
}


