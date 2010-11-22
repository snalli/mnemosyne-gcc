#include <mtm_i.h>
#include <mode/vtable.h>
#include <mode/vtable_macros.h>
#include <local.h>
#include "mode/wbetl/barrier.h"
#include "mode/wbetl/beginend.h"
#include "mode/wbetl/memcpy.h"
#include "mode/wbetl/memset.h"
#include "mode/wbetl/wbetl_i.h"
#include "mode/common/memcpy.h"
#include "mode/common/memset.h"
#include "mode/common/rwset.h"


#undef DEFINE_VTABLE_MEMBER
#define DEFINE_VTABLE_MEMBER(result, function, args, ARG)   \
    ARG##function,


mtm_vtable_t STR2 (mtm_wbetl, _vtable) =
{
	FOREACH_OUTER_FUNCTION (DEFINE_VTABLE_MEMBER, mtm_wbetl_)
	_GEN_READ_BARRIERS_LIST (mtm_wbetl_R, mtm_wbetl_R, mtm_wbetl_R, mtm_wbetl_RfW)
	_GEN_WRITE_BARRIERS_LIST (mtm_wbetl_W, mtm_wbetl_W, mtm_wbetl_W)
	_ITM_FOREACH_MEMCPY (DEFINE_VTABLE_MEMBER, STR2 (mtm_wbetl, _))
	_ITM_FOREACH_LOG_TRANSFER (DEFINE_VTABLE_MEMBER, mtm_local_)
	_ITM_FOREACH_MEMSET (DEFINE_VTABLE_MEMBER, STR2 (mtm_wbetl, _))
	_ITM_FOREACH_MEMMOVE (DEFINE_VTABLE_MEMBER, STR2 (mtm_wbetl, _))
};






m_result_t
mtm_wbetl_create(mtm_tx_t *tx, mtm_mode_data_t **datap)
{
	mtm_wbetl_mode_data_t *data;

#if CM == CM_PRIORITY
	COMPILE_TIME_ASSERT((sizeof(w_entry_t) & ALIGNMENT_MASK) == 0); /* Multiple of ALIGNMENT */
#endif /* CM == CM_PRIORITY */

	if ((data = (mtm_wbetl_mode_data_t *) malloc(sizeof(mtm_wbetl_mode_data_t)))
	    == NULL)
	{
		return M_R_FAILURE;
	}

	/* Read set */
	data->r_set.nb_entries = 0;
	data->r_set.size = RW_SET_SIZE;
	mtm_allocate_rs_entries(tx, data, 0);

	/* Write set */
	data->w_set.nb_entries = 0;
	data->w_set.size = RW_SET_SIZE;
	data->w_set.reallocate = 0;
	mtm_allocate_ws_entries(tx, data, 0);

	*datap = (mtm_mode_data_t *) data;

	return M_R_SUCCESS;
}


m_result_t
mtm_wbetl_destroy(mtm_mode_data_t *_data)
{
	mode_data_t *data = (mode_data_t *) _data;
#ifdef EPOCH_GC
	mtm_word_t t;
#endif /* EPOCH_GC */
	
#ifdef EPOCH_GC
	t = GET_CLOCK;
	gc_free(data->r_set.entries, t);
	gc_free(data->w_set.entries, t);
#else /* ! EPOCH_GC */
	free(data->r_set.entries);
	free(data->w_set.entries);
#endif /* ! EPOCH_GC */
}
