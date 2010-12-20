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
#include "log_tornbit.h"

m_log_ops_t log_tornbit_ops = {
	m_log_tornbit_alloc,
	m_log_tornbit_init,
	m_log_tornbit_truncation_init,
	m_log_tornbit_truncation_prepare_next,
	m_log_tornbit_truncation_do,
	m_log_tornbit_recovery_init,
	m_log_tornbit_recovery_prepare_next,
	m_log_tornbit_recovery_do,
	m_log_tornbit_report_stats,
};

#define FLUSH_CACHELINE_ONCE

/* Print debug messages */
#undef _DEBUG_THIS
//#define _DEBUG_THIS


#define _DEBUG_PRINT_TMLOG(log)                                 \
  printf("nvmd       : %p\n", log->phlog_tornbit.nvmd);         \
  printf("nvphlog    : %p\n", log->phlog_tornbit.nvphlog);      \
  printf("stable_tail: %lu\n", log->phlog_tornbit.stable_tail); \
  printf("tail       : %lu\n", log->phlog_tornbit.tail);        \
  printf("head       : %lu\n", log->phlog_tornbit.head);        \
  printf("read_index : %lu\n", log->phlog_tornbit.read_index);

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
m_log_tornbit_alloc(m_log_dsc_t *log_dsc)
{
	m_log_tornbit_t *log_tornbit;

	if (posix_memalign((void **) &log_tornbit, sizeof(uint64_t), sizeof(m_log_tornbit_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	/* 
	 * The underlying physical log volatile structure requires to be
	 * word aligned.
	 */
	assert((( (uintptr_t) &log_tornbit->phlog_tornbit) & (sizeof(uint64_t)-1)) == 0);
	log_tornbit->flush_set = (tornbit_flush_set_t *) PointerHash_new();
	log_dsc->log = (m_log_t *) log_tornbit;

	return M_R_SUCCESS;
}


m_result_t 
m_log_tornbit_init(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc)
{
	m_log_tornbit_t *log_tornbit = (m_log_tornbit_t *) log;
	m_phlog_tornbit_t *phlog_tornbit = &(log_tornbit->phlog_tornbit);

	m_phlog_tornbit_format(set, 
	                       (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                       log_dsc->nvphlog, 
	                       LF_TYPE_PAXOS_TORNBIT);
	m_phlog_tornbit_init(phlog_tornbit, 
	                     (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                     log_dsc->nvphlog);

	return M_R_SUCCESS;
}


static inline
m_result_t 
truncation_prepare(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{

	return M_R_SUCCESS;
}


m_result_t 
m_log_tornbit_truncation_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return truncation_prepare(set, log_dsc);
}


m_result_t 
m_log_tornbit_truncation_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return truncation_prepare(set, log_dsc);
}


m_result_t 
m_log_tornbit_truncation_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return M_R_SUCCESS;
}

static inline
m_result_t 
recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	
	return M_R_SUCCESS;
}


m_result_t 
m_log_tornbit_recovery_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return M_R_SUCCESS;
}


m_result_t 
m_log_tornbit_recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return recovery_prepare_next(set, log_dsc);
}


m_result_t 
m_log_tornbit_recovery_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc)
{
	return M_R_SUCCESS;
}


m_result_t 
m_log_tornbit_report_stats(m_log_dsc_t *log_dsc)
{
	return M_R_SUCCESS;
}
