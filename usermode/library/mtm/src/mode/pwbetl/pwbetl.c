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

#include <pcm.h>
#include <log.h>

#include <pwb_i.h>
#include <rwset.h>


#ifndef RW_SET_SIZE
#define RW_SET_SIZE (256*1024)
#endif

#undef _DTABLE_MEMBER
#define _DTABLE_MEMBER(result, function, args, ARG)   ARG##function,


mtm_dtable_t mtm_pwbetl_dtable;
/*
{
	FOREACH_ABI_FUNCTION      (_DTABLE_MEMBER, _ITM_)
	FOREACH_READ_BARRIER      (_DTABLE_MEMBER, _ITM_)
	FOREACH_WRITE_BARRIER     (_DTABLE_MEMBER, _ITM_)
	_ITM_FOREACH_MEMCPY       (_DTABLE_MEMBER, _ITM_)
	_ITM_FOREACH_LOG_TRANSFER (_DTABLE_MEMBER, _ITM_)
	_ITM_FOREACH_MEMSET       (_DTABLE_MEMBER, _ITM_)
	_ITM_FOREACH_MEMMOVE      (_DTABLE_MEMBER, _ITM_)
};
*/

/* Called by mtm_init_thread */
m_result_t
mtm_pwbetl_create(mtm_tx_t *tx, mtm_mode_data_t **datap)
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
#ifdef SYNC_TRUNCATION	
	m_logmgr_alloc_log(tx->pcm_storeset, M_TMLOG_LF_TYPE, 0, &data->ptmlog_dsc);
#else
	m_logmgr_alloc_log(tx->pcm_storeset, M_TMLOG_LF_TYPE, LF_ASYNC_TRUNCATION, &data->ptmlog_dsc);
#endif	
	data->ptmlog = (M_TMLOG_T *) data->ptmlog_dsc->log;

	*datap = (mtm_mode_data_t *) data;

	return M_R_SUCCESS;
}


m_result_t
mtm_pwbetl_destroy(mtm_mode_data_t *_data)
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
