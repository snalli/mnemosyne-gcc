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
 * \brief Implements the tornbit log for persistent writeback transactions.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */

#include <stdio.h>
#include <assert.h>
#include <mnemosyne.h>
#include <pcm.h>
#include <cuckoo_hash/PointerHashInline.h>
#include <debug.h>
#include "tmlog_tornbit.h"

m_log_ops_t tmlog_tornbit_ops = {
	m_tmlog_tornbit_alloc,
	m_tmlog_tornbit_init,
	m_tmlog_tornbit_truncation_init,
	m_tmlog_tornbit_truncation_prepare_next,
	m_tmlog_tornbit_truncation_do,
	m_tmlog_tornbit_recovery_init,
	m_tmlog_tornbit_recovery_prepare_next,
	m_tmlog_tornbit_recovery_do,
	m_tmlog_tornbit_report_stats,
};

#define FLUSH_CACHELINE_ONCE

/* Print debug messages */
#undef _DEBUG_THIS
//#define _DEBUG_THIS


#define _DEBUG_PRINT_TMLOG(tmlog)                                 \
  printf("nvmd       : %p\n", tmlog->phlog_tornbit.nvmd);         \
  printf("nvphlog    : %p\n", tmlog->phlog_tornbit.nvphlog);      \
  printf("stable_tail: %lu\n", tmlog->phlog_tornbit.stable_tail); \
  printf("tail       : %lu\n", tmlog->phlog_tornbit.tail);        \
  printf("head       : %lu\n", tmlog->phlog_tornbit.head);        \
  printf("read_index : %lu\n", tmlog->phlog_tornbit.read_index);

void PointerHash_removeKey_noshrink(PointerHash *self, void *k)
{
	PointerHashRecord *r;
	
	r = PointerHash_record1_(self, k);	
	if(r->k == k)
	{
		r->k = 0x0;
		r->v = 0x0;
		self->keyCount --;
		return;
	}
	
	r = PointerHash_record2_(self, k);
	if(r->k == k)
	{
		r->k = 0x0;
		r->v = 0x0;
		self->keyCount --;
		return;
	}
}


m_result_t 
m_tmlog_tornbit_alloc(m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog_tornbit;

	if (posix_memalign((void **) &tmlog_tornbit, sizeof(uint64_t), sizeof(m_tmlog_tornbit_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	/* 
	 * The underlying physical log volatile structure requires to be
	 * word aligned.
	 */
	assert((( (uintptr_t) &tmlog_tornbit->phlog_tornbit) & (sizeof(uint64_t)-1)) == 0);
	tmlog_tornbit->flush_set = (tornbit_flush_set_t *) PointerHash_new();
	log_dsc->log = (m_log_t *) tmlog_tornbit;

	return M_R_SUCCESS;
}


m_result_t 
m_tmlog_tornbit_init(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog_tornbit = (m_tmlog_tornbit_t *) log;
	m_phlog_tornbit_t *phlog_tornbit = &(tmlog_tornbit->phlog_tornbit);

	m_phlog_tornbit_format(set, 
	                       (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                       log_dsc->nvphlog, 
	                       LF_TYPE_TM_TORNBIT);
	m_phlog_tornbit_init(phlog_tornbit, 
	                     (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                     log_dsc->nvphlog);

	return M_R_SUCCESS;
}


static inline
m_result_t 
truncation_prepare(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog = (m_tmlog_tornbit_t *) log_dsc->log;
	pcm_word_t        value;
	uint64_t          sqn = INV_LOG_ORDER;
	uintptr_t         addr;
	pcm_word_t        mask;
	uintptr_t         block_addr;
	int               val;

#ifdef _DEBUG_THIS
	printf("prepare_truncate: log_dsc = %p\n", log_dsc);
	_DEBUG_PRINT_TMLOG(tmlog)
#endif

	/*
	 * Invariant: If there is a stable region to read from then there is at 
	 * least one atomic log fragment which corresponds to one logical 
	 * transaction. 
	 */
retry:	 
	if (m_phlog_tornbit_stable_exists(&(tmlog->phlog_tornbit))) {
		while(1) {
			if (m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &addr) == M_R_SUCCESS) {
				if (addr == XACT_COMMIT_MARKER) {
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &sqn) == M_R_SUCCESS);
					m_phlog_tornbit_next_chunk(&tmlog->phlog_tornbit);
					break;
				} else if (addr == XACT_ABORT_MARKER) {
					/* 
					 * Log fragment corresponds to an aborted transaction.
					 * Ignore it, truncate the log up to here, and retry.
					 */
					//FIXME: truncate the log up to here
					goto retry;
				} else {
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &value) == M_R_SUCCESS);
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &mask) == M_R_SUCCESS);
#ifdef _DEBUG_THIS
					printf("addr  = 0x%lX\n", addr);
					printf("value = 0x%lX\n", value);
					printf("mask  = 0x%lX\n", mask);
#endif
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
m_tmlog_tornbit_truncation_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return truncation_prepare(set, log_dsc);
}


m_result_t 
m_tmlog_tornbit_truncation_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return truncation_prepare(set, log_dsc);
}


m_result_t 
m_tmlog_tornbit_truncation_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	int               i;
	m_tmlog_tornbit_t *tmlog = (m_tmlog_tornbit_t *) log_dsc->log;
	uintptr_t         block_addr;

#ifdef _DEBUG_THIS
	printf("m_tmlog_tornbit_truncation_do: START\n");
	_DEBUG_PRINT_TMLOG(tmlog)
#endif

#ifdef FLUSH_CACHELINE_ONCE
	for(i = 0; i < ((PointerHash *) tmlog->flush_set)->size; i++) {
		PointerHashRecord *r = PointerHashRecords_recordAt_(((PointerHash *) tmlog->flush_set)->records, i);
		if (block_addr = (uintptr_t) r->k) {
			//PointerHash_removeKey_noshrink((PointerHash *) tmlog->flush_set, (void *) block_addr);
			PointerHash_removeKey_((PointerHash *) tmlog->flush_set, (void *) block_addr);
			PCM_WB_FLUSH(set, (volatile pcm_word_t *) block_addr);
		}
	}
#endif	
	m_phlog_tornbit_truncate_async(set, &tmlog->phlog_tornbit);

#ifdef _DEBUG_THIS
	printf("m_tmlog_tornbit_truncation_do: DONE\n");
	_DEBUG_PRINT_TMLOG(tmlog)
#endif

	return M_R_SUCCESS;
}

static inline
m_result_t 
recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog = (m_tmlog_tornbit_t *) log_dsc->log;
	pcm_word_t        value;
	uint64_t          sqn = INV_LOG_ORDER;
	uintptr_t         addr;
	pcm_word_t        mask;
	uintptr_t         block_addr;
	int               val;
	uint64_t          readindex_checkpoint;

#ifdef _DEBUG_THIS
	printf("recovery_prepare_next: log_dsc = %p\n", log_dsc);
	_DEBUG_PRINT_TMLOG(tmlog)
#endif

	/*
	 * Invariant: If there is a stable region to read from then there is at 
	 * least one atomic log fragment which corresponds to one logical 
	 * transaction. 
	 */
retry:	 
	if (m_phlog_tornbit_stable_exists(&(tmlog->phlog_tornbit))) {
		/* 
		 * Checkpoint the readindex so that we can restore it after we find 
		 * the transaction sequence number and be able to recover the 
		 * transaction.
		 */
		assert(m_phlog_tornbit_checkpoint_readindex(&(tmlog->phlog_tornbit), &readindex_checkpoint) == M_R_SUCCESS);
		while(1) {
			if (m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &addr) == M_R_SUCCESS) {
				if (addr == XACT_COMMIT_MARKER) {
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &sqn) == M_R_SUCCESS);
					m_phlog_tornbit_restore_readindex(&(tmlog->phlog_tornbit), readindex_checkpoint);
					break;
				} else if (addr == XACT_ABORT_MARKER) {
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &sqn) == M_R_SUCCESS);
					m_phlog_tornbit_next_chunk(&tmlog->phlog_tornbit);
					/* Ignore an aborted transaction's log fragment */
					m_phlog_tornbit_truncate_async(set, &tmlog->phlog_tornbit);
					sqn = INV_LOG_ORDER;
					goto retry;
				} else {
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &value) == M_R_SUCCESS);
					assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &mask) == M_R_SUCCESS);
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
m_tmlog_tornbit_recovery_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog = (m_tmlog_tornbit_t *) log_dsc->log;

	m_phlog_tornbit_init(&tmlog->phlog_tornbit, 
	                     (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                     log_dsc->nvphlog);
	
	m_phlog_tornbit_check_consistency((m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                                  log_dsc->nvphlog, 
	                                  &(tmlog->phlog_tornbit.stable_tail));
	recovery_prepare_next(set, log_dsc);

	return M_R_SUCCESS;
}


m_result_t 
m_tmlog_tornbit_recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return recovery_prepare_next(set, log_dsc);
}


m_result_t 
m_tmlog_tornbit_recovery_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog = (m_tmlog_tornbit_t *) log_dsc->log;
	pcm_word_t        value;
	uint64_t          sqn = INV_LOG_ORDER;
	uintptr_t         addr;
	pcm_word_t        mask;
	uintptr_t         block_addr;
	int               val;
	uint64_t          readindex_checkpoint;

#ifdef _DEBUG_THIS
	printf("m_tmlog_tornbit_recovery_do: %lu\n", log_dsc->logorder);
	_DEBUG_PRINT_TMLOG(tmlog)
#endif

	/*
	 * Invariant: If there is a stable region to read from then there is at 
	 * least one atomic log fragment which corresponds to one logical 
	 * transaction. 
	 */
	assert (m_phlog_tornbit_stable_exists(&(tmlog->phlog_tornbit))); 

#ifdef _DEBUG_THIS
	printf("tmlog->phlog_tornbit.head = %llu\n", tmlog->phlog_tornbit.head);
	printf("tmlog->phlog_tornbit.stable_tail = %llu\n", tmlog->phlog_tornbit.stable_tail);
	printf("tmlog->phlog_tornbit.read_index = %llu\n", tmlog->phlog_tornbit.read_index);
#endif	
	while(1) {
		if (m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &addr) == M_R_SUCCESS) {
			if (addr == XACT_COMMIT_MARKER) {
				assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &sqn) == M_R_SUCCESS);
				m_phlog_tornbit_next_chunk(&tmlog->phlog_tornbit);
				/* Drop the recovered log fragment */
				m_phlog_tornbit_truncate_async(set, &tmlog->phlog_tornbit);
				break;
			} else if (addr == XACT_ABORT_MARKER) {
				/* 
				 * Recovery shouldn't be passed a log fragment corresponding to
				 * an aborted transaction.
				 */
				M_INTERNALERROR("Trying to recover an aborted transaction!\n");
			} else {
				assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &value) == M_R_SUCCESS);
				assert(m_phlog_tornbit_read(&(tmlog->phlog_tornbit), &mask) == M_R_SUCCESS);
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
m_tmlog_tornbit_report_stats(m_log_dsc_t *log_dsc)
{
	m_tmlog_tornbit_t *tmlog = (m_tmlog_tornbit_t *) log_dsc->log;
	m_phlog_tornbit_t *phlog = &(tmlog->phlog_tornbit);

	printf("PRINT TORNBIT STATS\n");
	printf("wait_for_trunc               : %llu\n", phlog->stat_wait_for_trunc);
	if (phlog->stat_wait_for_trunc > 0) {
		printf("AVG(stat_wait_time_for_trunc): %llu\n", phlog->stat_wait_time_for_trunc / phlog->stat_wait_for_trunc);
	}
}
