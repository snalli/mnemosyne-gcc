/**
 * \file
 * 
 * \brief Physical log implemented using a torn-bit per word to detect
 * torn words. 
 *
 */

/* System header files */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
/* Mnemosyne common header files */
#include <result.h>
#include "phlog_base.h"
#include "hal/pcm_i.h"


/**
 * \brief Check the consistency of the non-volatile log and find the consistent
 * stable region starting from the head.
 */
m_result_t
m_phlog_base_check_consistency(m_phlog_base_nvmd_t *nvmd, 
                                  pcm_word_t *nvphlog,
                                  uint64_t *stable_tail)
{
	*stable_tail = nvmd->tail;

	return M_R_SUCCESS;
}


/**
 * \brief Formats the non-volatile physical log for reuse.
 */
m_result_t
m_phlog_base_format (pcm_storeset_t *set, 
                     m_phlog_base_nvmd_t *nvmd, 
                     pcm_word_t *nvphlog, 
                     int type)
{
	PCM_NT_STORE(set, (volatile pcm_word_t *) &nvmd->head, 0);
	PCM_NT_STORE(set, (volatile pcm_word_t *) &nvmd->tail, 0);
	PCM_NT_FLUSH(set);
	
	return M_R_SUCCESS;
}


/**
 * \brief Allocates a volatile log structure.
 */
m_result_t
m_phlog_base_alloc (m_phlog_base_t **phlog_basep)
{
	m_phlog_base_t      *phlog_base;
	
	if (posix_memalign((void **) &phlog_base, sizeof(uint64_t), sizeof(m_phlog_base_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	*phlog_basep = phlog_base;

	return M_R_SUCCESS;
}


/**
 * \brief Initializes the volatile log descriptor using the non-volatile 
 * metadata referenced by log_dsc.
 */
m_result_t
m_phlog_base_init(m_phlog_base_t *phlog, 
                  m_phlog_base_nvmd_t *nvmd,
                  pcm_word_t *nvphlog)					  
{
	phlog->nvmd = nvmd;
	phlog->nvphlog = nvphlog;
	phlog->buffer_count = 0;
	phlog->head = phlog->nvmd->head;
	phlog->tail = phlog->nvmd->tail;
	phlog->read_index = phlog->nvmd->head;

	return M_R_SUCCESS;
}


/**
 * \brief Truncates the log up to the read_index point.
 */
m_result_t
m_phlog_base_truncate_async(pcm_storeset_t *set, m_phlog_base_t *phlog) 
{
	phlog->head = phlog->read_index;

	PCM_NT_FLUSH(set);
	PCM_NT_STORE(set, (volatile pcm_word_t *) &phlog->nvmd->head, (pcm_word_t) phlog->head);
	PCM_NT_FLUSH(set);
	
	return M_R_SUCCESS;
}
