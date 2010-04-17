#include <pcm.h>
#include <log.h>
#include <mtm_i.h>
#include <mode/vtable.h>
#include <mode/vtable_macros.h>
#include <local.h>
#include "mode/pwb/tmlog.h"
#include "mode/pwb/barrier.h"
#include "mode/pwb/beginend.h"
#include "mode/pwb/memcpy.h"
#include "mode/pwb/memset.h"
#include "mode/pwb/pwb_i.h"
#include "mode/pwb/rwset.c"
#include "mode/common/memcpy.h"
#include "mode/common/memset.h"
#include "mode/common/rwset.h"

#ifndef RW_SET_SIZE
#define RW_SET_SIZE 16384
#endif

#undef DEFINE_VTABLE_MEMBER
#define DEFINE_VTABLE_MEMBER(result, function, args, ARG)   \
    ARG##function,


mtm_vtable_t STR2 (mtm_pwb, _vtable) =
{
	FOREACH_OUTER_FUNCTION (DEFINE_VTABLE_MEMBER, mtm_pwb_)
	_GEN_READ_BARRIERS_LIST (mtm_pwb_R, mtm_pwb_R, mtm_pwb_R, mtm_pwb_RfW)
	_GEN_WRITE_BARRIERS_LIST (mtm_pwb_W, mtm_pwb_W, mtm_pwb_W)
	_ITM_FOREACH_MEMCPY (DEFINE_VTABLE_MEMBER, STR2 (mtm_pwb, _))
	_ITM_FOREACH_LOG_TRANSFER (DEFINE_VTABLE_MEMBER, mtm_local_)
	_ITM_FOREACH_MEMSET (DEFINE_VTABLE_MEMBER, STR2 (mtm_pwb, _))
	_ITM_FOREACH_MEMMOVE (DEFINE_VTABLE_MEMBER, STR2 (mtm_pwb, _))
};

/* Called by mtm_init_thread */
m_result_t
mtm_pwb_create(mtm_tx_t *tx, mtm_mode_data_t **datap)
{
	mtm_pwb_mode_data_t *data;

#if CM == CM_PRIORITY
	COMPILE_TIME_ASSERT((sizeof(w_entry_t) & ALIGNMENT_MASK) == 0); /* Multiple of ALIGNMENT */
#endif /* CM == CM_PRIORITY */

	if ((data = (mtm_pwb_mode_data_t *) malloc(sizeof(mtm_pwb_mode_data_t)))
	    == NULL)
	{
		return M_R_FAILURE;
	}

	/* Read set */
	data->r_set.nb_entries = 0;
	data->r_set.size = RW_SET_SIZE;
	mtm_allocate_rs_entries(tx, data, 0);

	/* Volatile write set */
	data->w_set.nb_entries = 0;
	data->w_set.size = RW_SET_SIZE;
	data->w_set.reallocate = 0;
	mtm_allocate_ws_entries(tx, data, 0);

	/* Non-volatile log */
	m_logmgr_alloc_log(tx->pcm_storeset, M_TMLOG_LF_TYPE, LF_ASYNC_TRUNCATION, &data->ptmlog_dsc);
	data->ptmlog = (M_TMLOG_T *) data->ptmlog_dsc->log;


	*datap = (mtm_mode_data_t *) data;

	return M_R_SUCCESS;
}


m_result_t
mtm_pwb_destroy(mtm_mode_data_t *_data)
{
	mode_data_t *data = (mode_data_t *) _data;
#ifdef EPOCH_GC
	mtm_word_t t;
#endif /* EPOCH_GC */
	
	m_logmgr_free_log(data->ptmlog_dsc);

#ifdef EPOCH_GC
	t = GET_CLOCK;
	gc_free(data->r_set.entries, t);
	gc_free(data->w_set.entries, t);
#else /* ! EPOCH_GC */
	free(data->r_set.entries);
	free(data->w_set.entries);
#endif /* ! EPOCH_GC */
}
