#include <stdio.h>
#include <assert.h>
#include <mnemosyne.h>
#include <pcm.h>
#include <cuckoo_hash/PointerHashInline.h>
#include <debug.h>
#include "rawlog_tornbit.helper.h"

m_log_ops_t rawlog_tornbit_ops = {
	m_rawlog_tornbit_alloc,
	m_rawlog_tornbit_init,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};


m_result_t 
m_rawlog_tornbit_alloc(m_log_dsc_t *log_dsc)
{
	m_rawlog_tornbit_t *rawlog_tornbit;

	if (posix_memalign((void **) &rawlog_tornbit, sizeof(uint64_t), sizeof(m_rawlog_tornbit_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	/* 
	 * The underlying physical log volatile structure requires to be
	 * word aligned.
	 */
	assert((( (uintptr_t) &rawlog_tornbit->phlog_tornbit) & (sizeof(uint64_t)-1)) == 0);
	log_dsc->log = (m_log_t *) rawlog_tornbit;

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_tornbit_init(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc)
{
	m_rawlog_tornbit_t *rawlog_tornbit = (m_rawlog_tornbit_t *) log;
	m_phlog_tornbit_t *phlog_tornbit = &(rawlog_tornbit->phlog_tornbit);

	m_phlog_tornbit_format(set, 
	                       (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                       log_dsc->nvphlog, 
	                       LF_TYPE_TM_TORNBIT);
	m_phlog_tornbit_init(phlog_tornbit, 
	                     (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                     log_dsc->nvphlog);

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_tornbit_register(pcm_storeset_t *pcm_storeset)
{
	m_logmgr_register_logtype(pcm_storeset, LF_TYPE_TM_TORNBIT, &rawlog_tornbit_ops);

	return M_R_SUCCESS;
}
