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

/*!
 * \file
 *
 * \brief Implements the base log for persistent writeback transactions.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */

#include <stdio.h>
#include <assert.h>
#include <mnemosyne.h>
#include <pcm.h>
#include <cuckoo_hash/PointerHashInline.h>
#include <debug.h>
#include "tmlog_base.h"

m_log_ops_t tmlog_base_ops = {
	m_tmlog_base_alloc,
	m_tmlog_base_init,
	m_tmlog_base_truncation_init,
	m_tmlog_base_truncation_prepare_next,
	m_tmlog_base_truncation_do,
	m_tmlog_base_recovery_init,
	m_tmlog_base_recovery_prepare_next,
	m_tmlog_base_recovery_do,
	m_tmlog_base_report_stats,
};

/* Print debug messages */
#undef _DEBUG_THIS
//#define _DEBUG_THIS

#define _DEBUG_PRINT_TMLOG(tmlog)                               \
  printf("stable_tail: %lu\n", tmlog->phlog_base.nvmd->tail);   \
  printf("stable_head: %lu\n", tmlog->phlog_base.nvmd->head);   \
  printf("tail       : %lu\n", tmlog->phlog_base.tail);         \
  printf("head       : %lu\n", tmlog->phlog_base.head);         \
  printf("read_index : %lu\n", tmlog->phlog_base.read_index);

m_result_t 
m_tmlog_base_alloc(m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t *tmlog_base;

	if (posix_memalign((void **) &tmlog_base, sizeof(uint64_t), sizeof(m_tmlog_base_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	/* 
	 * The underlying physical log volatile structure requires to be
	 * word aligned.
	 */
	assert((( (uintptr_t) &tmlog_base->phlog_base) & (sizeof(uint64_t)-1)) == 0);
	tmlog_base->flush_set = (set_t *) PointerHash_new();
	log_dsc->log = (m_log_t *) tmlog_base;

	return M_R_SUCCESS;
}


m_result_t 
m_tmlog_base_init(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t *tmlog_base = (m_tmlog_base_t *) log;
	m_phlog_base_t *phlog_base = &(tmlog_base->phlog_base);

	m_phlog_base_format(set, 
	                    (m_phlog_base_nvmd_t *) log_dsc->nvmd, 
	                    log_dsc->nvphlog, 
	                    LF_TYPE_TM_BASE);
	m_phlog_base_init(phlog_base, 
	                  (m_phlog_base_nvmd_t *) log_dsc->nvmd, 
	                  log_dsc->nvphlog);

	return M_R_SUCCESS;
}


static inline
m_result_t 
truncation_prepare(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t *tmlog = (m_tmlog_base_t *) log_dsc->log;
	pcm_word_t        value;
	uint64_t          sqn = INV_LOG_ORDER;
	uintptr_t         addr;
	pcm_word_t        mask;
	uintptr_t         block_addr;
	int               val;
	int               i;

#ifdef _DEBUG_THIS
	printf("truncation_prepare: log_dsc = %p\n", log_dsc);
	_DEBUG_PRINT_TMLOG(tmlog);
#endif


	/*
	 * Invariant: If there is a stable region to read from then there is at 
	 * least one atomic log fragment which corresponds to one logical 
	 * transaction. 
	 */
retry:	 
	if (m_phlog_base_stable_exists(&(tmlog->phlog_base))) {
		while(1) {
			if (m_phlog_base_read(&(tmlog->phlog_base), &addr) == M_R_SUCCESS) {
				if (addr == XACT_COMMIT_MARKER) {
					assert(m_phlog_base_read(&(tmlog->phlog_base), &sqn) == M_R_SUCCESS);
					m_phlog_base_next_chunk(&tmlog->phlog_base);
					break;
				} else if (addr == XACT_ABORT_MARKER) {
					/* 
					 * Log fragment corresponds to an aborted transaction.
					 * Ignore it, truncate the log up to here, and retry.
					 */
					assert(m_phlog_base_read(&(tmlog->phlog_base), &sqn) == M_R_SUCCESS);
					m_phlog_base_next_chunk(&tmlog->phlog_base);
#ifdef FLUSH_CACHELINE_ONCE
					for(i = 0; i < ((PointerHash *) tmlog->flush_set)->size; i++) {
						PointerHashRecord *r = PointerHashRecords_recordAt_(((PointerHash *) tmlog->flush_set)->records, i);
						if (block_addr = (uintptr_t) r->k) {
							PointerHash_removeKey_((PointerHash *) tmlog->flush_set, (void *) block_addr);
						}
					}
#endif					
					m_phlog_base_truncate_async(set, &tmlog->phlog_base);
					sqn = INV_LOG_ORDER;
					goto retry;
				} else {
					assert(m_phlog_base_read(&(tmlog->phlog_base), &value) == M_R_SUCCESS);
					assert(m_phlog_base_read(&(tmlog->phlog_base), &mask) == M_R_SUCCESS);
					block_addr = (uintptr_t) BLOCK_ADDR(addr);

#ifdef FLUSH_CACHELINE_ONCE
					if (!PointerHash_at_((PointerHash *) tmlog->flush_set, (void *) block_addr)) {
						PointerHash_at_put_((PointerHash *) tmlog->flush_set, 
						                    (void *) block_addr, 
						                    (void *) 1);
					}
#else 					
					PCM_WB_FLUSH(set, (volatile pcm_word_t *) block_addr);
#endif					
				}
			} else {
				M_INTERNALERROR("Invariant violation: there must be at least one atomic log fragment.");
			}
		}	
		log_dsc->logorder = sqn;
	} else {
		log_dsc->logorder = sqn;
	}
	
	return M_R_SUCCESS;
}


m_result_t 
m_tmlog_base_truncation_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return truncation_prepare(set, log_dsc);
}


m_result_t 
m_tmlog_base_truncation_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return truncation_prepare(set, log_dsc);
}


m_result_t 
m_tmlog_base_truncation_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	int            i;
	m_tmlog_base_t *tmlog = (m_tmlog_base_t *) log_dsc->log;
	uintptr_t      block_addr;

#ifdef _DEBUG_THIS
	printf("m_tmlog_base_truncation_do: START: log_dsc = %p\n", log_dsc);
	_DEBUG_PRINT_TMLOG(tmlog);
#endif


#ifdef FLUSH_CACHELINE_ONCE
	for(i = 0; i < ((PointerHash *) tmlog->flush_set)->size; i++) {
		PointerHashRecord *r = PointerHashRecords_recordAt_(((PointerHash *) tmlog->flush_set)->records, i);
		if (block_addr = (uintptr_t) r->k) {
			PointerHash_removeKey_((PointerHash *) tmlog->flush_set, (void *) block_addr);
			PCM_WB_FLUSH(set, (volatile pcm_word_t *) block_addr);
		}
	}
#endif	
	m_phlog_base_truncate_async(set, &tmlog->phlog_base);

#ifdef _DEBUG_THIS
	printf("m_tmlog_base_truncation_do: DONE: log_dsc = %p\n", log_dsc);
	_DEBUG_PRINT_TMLOG(tmlog);
#endif

	return M_R_SUCCESS;
}


static inline
m_result_t 
recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t    *tmlog = (m_tmlog_base_t *) log_dsc->log;
	pcm_word_t        value;
	uint64_t          sqn = INV_LOG_ORDER;
	uintptr_t         addr;
	pcm_word_t        mask;
	uintptr_t         block_addr;
	int               val;
	uint64_t          readindex_checkpoint;

#ifdef _DEBUG_THIS
	printf("recovery_prepare_next: log_dsc = %p\n", log_dsc);
	printf("stable_tail: %lu\n", tmlog->phlog_base.nvmd->tail);
	printf("stable_head: %lu\n", tmlog->phlog_base.nvmd->head);
	printf("tail       : %lu\n", tmlog->phlog_base.tail);
	printf("head       : %lu\n", tmlog->phlog_base.head);
	printf("read_index : %lu\n", tmlog->phlog_base.read_index);
#endif

	/*
	 * Invariant: If there is a stable region to read from then there is at 
	 * least one atomic log fragment which corresponds to one logical 
	 * transaction. 
	 */
retry:	 
	if (m_phlog_base_stable_exists(&(tmlog->phlog_base))) {
		/* 
		 * Checkpoint the readindex so that we can restore it after we find 
		 * the transaction sequence number and be able to recover the 
		 * transaction.
		 */
		assert(m_phlog_base_checkpoint_readindex(&(tmlog->phlog_base), &readindex_checkpoint) == M_R_SUCCESS);
		while(1) {
			if (m_phlog_base_read(&(tmlog->phlog_base), &addr) == M_R_SUCCESS) {
				if (addr == XACT_COMMIT_MARKER) {
					assert(m_phlog_base_read(&(tmlog->phlog_base), &sqn) == M_R_SUCCESS);
					m_phlog_base_restore_readindex(&(tmlog->phlog_base), readindex_checkpoint);
					break;
				} else if (addr == XACT_ABORT_MARKER) {
					assert(m_phlog_base_read(&(tmlog->phlog_base), &sqn) == M_R_SUCCESS);
					m_phlog_base_next_chunk(&tmlog->phlog_base);
					/* Ignore an aborted transaction's log fragment */
					m_phlog_base_truncate_async(set, &tmlog->phlog_base);
					sqn = INV_LOG_ORDER;
					goto retry;
				} else {
					assert(m_phlog_base_read(&(tmlog->phlog_base), &value) == M_R_SUCCESS);
					assert(m_phlog_base_read(&(tmlog->phlog_base), &mask) == M_R_SUCCESS);
				}	
			} else {
				M_INTERNALERROR("Invariant violation: there must be at least one atomic log fragment.");
			}
		}	
		log_dsc->logorder = sqn;
	} else {
		log_dsc->logorder = sqn;
	}
	
	return M_R_SUCCESS;
}


m_result_t 
m_tmlog_base_recovery_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t *tmlog = (m_tmlog_base_t *) log_dsc->log;

	m_phlog_base_init(&tmlog->phlog_base, 
	                  (m_phlog_base_nvmd_t *) log_dsc->nvmd, 
	                  log_dsc->nvphlog);
	
	recovery_prepare_next(set, log_dsc);

	return M_R_SUCCESS;
}


m_result_t 
m_tmlog_base_recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return recovery_prepare_next(set, log_dsc);
}


m_result_t 
m_tmlog_base_recovery_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t    *tmlog = (m_tmlog_base_t *) log_dsc->log;
	pcm_word_t        value;
	uint64_t          sqn = INV_LOG_ORDER;
	uintptr_t         addr;
	pcm_word_t        mask;
	uintptr_t         block_addr;
	int               val;
	uint64_t          readindex_checkpoint;

#ifdef _DEBUG_THIS
	printf("m_tmlog_base_recovery_do: %lu\n", log_dsc->logorder);
	_DEBUG_PRINT_TMLOG(tmlog)
#endif

	/*
	 * Invariant: If there is a stable region to read from then there is at 
	 * least one atomic log fragment which corresponds to one logical 
	 * transaction. 
	 */
	assert (m_phlog_base_stable_exists(&(tmlog->phlog_base))); 
	while(1) {
		if (m_phlog_base_read(&(tmlog->phlog_base), &addr) == M_R_SUCCESS) {
			if (addr == XACT_COMMIT_MARKER) {
				assert(m_phlog_base_read(&(tmlog->phlog_base), &sqn) == M_R_SUCCESS);
				m_phlog_base_next_chunk(&tmlog->phlog_base);
				/* Drop the recovered log fragment */
				m_phlog_base_truncate_async(set, &tmlog->phlog_base);
				break;
			} else if (addr == XACT_ABORT_MARKER) {
				/* 
				 * Recovery shouldn't be passed a log fragment corresponding to
				 * an aborted transaction.
				 */
				M_INTERNALERROR("Trying to recover an aborted transaction!\n");
			} else {
				assert(m_phlog_base_read(&(tmlog->phlog_base), &value) == M_R_SUCCESS);
				assert(m_phlog_base_read(&(tmlog->phlog_base), &mask) == M_R_SUCCESS);
				if (mask!=0) {
					PCM_WB_STORE_ALIGNED_MASKED(set, (volatile pcm_word_t *) addr, value, mask);
					PCM_WB_FLUSH(set, (volatile pcm_word_t *) addr);
				}	
			}	
		} else {
			M_INTERNALERROR("Invariant violation: there must be at least one atomic log fragment.");
		}
	}	

	return M_R_SUCCESS;
}

m_result_t 
m_tmlog_base_report_stats(m_log_dsc_t *log_dsc)
{
	m_tmlog_base_t *tmlog = (m_tmlog_base_t *) log_dsc->log;
	m_phlog_base_t *phlog = &(tmlog->phlog_base);

	printf("PRINT BASE STATS\n");
	printf("wait_for_trunc               : %llu\n", phlog->stat_wait_for_trunc);
	if (phlog->stat_wait_for_trunc > 0) {
		printf("AVG(stat_wait_time_for_trunc): %llu\n", phlog->stat_wait_time_for_trunc / phlog->stat_wait_for_trunc);
	}	
}
