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
#include "phlog_tornbit.h"


/**
 * \brief Check the consistency of the non-volatile log and find the consistent
 * stable region starting from the head.
 */
m_result_t
m_phlog_tornbit_check_consistency(m_phlog_tornbit_nvmd_t *nvmd, 
                                  scm_word_t *nvphlog,
                                  uint64_t *stable_tail)
{
	uint64_t          head_index;
	uint64_t          i;
	uint64_t          tornbit;
	uint64_t          valid_tornbit;
	int               flip_tornbit = 0;

	valid_tornbit = LF_TORNBIT & nvmd->flags;
	i = head_index = nvmd->flags & LF_HEAD_MASK;
	while(1) {
		tornbit = (TORN_MASK & nvphlog[i]);
		printf("index = %d, valid_tornbit = %lx, tornbit = %lx\n", i, valid_tornbit, tornbit);

		if (tornbit != valid_tornbit) {
			*stable_tail = i;
			break;
		}
		i = (i + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
		if (i==0) {
			flip_tornbit = 1;
		}
		if (i==head_index) {
			break;
		}
	}

	return M_R_SUCCESS;
}


static
m_result_t
tornbit_format_nvlog (uint32_t head_index, 
                      uint64_t tornbit, 
                      m_phlog_tornbit_nvmd_t *nvmd, 
                      scm_word_t *nvphlog)
{
	int i;

	PCM_STORE(&nvmd->flags, head_index | tornbit);
	for (i=0; i<PHYSICAL_LOG_NUM_ENTRIES; i++) {
		if (i<head_index) {
			if ((nvphlog[i] & TORN_MASK) != tornbit) {
				PCM_STORE(&nvphlog[i], tornbit);
			}	
		} else {
			if ((nvphlog[i] & TORN_MASK) == tornbit) {
				PCM_STORE(&nvphlog[i], ~tornbit & TORN_MASK);
			}	
		}
	}
	PCM_BARRIER
	
	return M_R_SUCCESS;
}


/**
 * \brief Formats the non-volatile physical log for reuse.
 */
m_result_t
m_phlog_tornbit_format (m_phlog_tornbit_nvmd_t *nvmd, scm_word_t *nvphlog, int type)
{
	m_result_t             rv = M_R_FAILURE;
	
	if ((nvmd->generic_flags & LF_TYPE_MASK) == type) {
		//FIXME: should check consistency and perform a quick format instead
		//tornbit_check_nvlog(log_dsc);
		rv = tornbit_format_nvlog (0, TORNBIT_ONE, nvmd, nvphlog);
		if (rv != M_R_SUCCESS) {
			goto out;
		}
	} else {
		rv = tornbit_format_nvlog (0, TORNBIT_ONE, nvmd, nvphlog);
		if (rv != M_R_SUCCESS) {
			goto out;
		}
	}
	rv = M_R_SUCCESS;
out:
	return rv;
}


/**
 * \brief Allocates a volatile log structure.
 */
m_result_t
m_phlog_tornbit_alloc (m_phlog_tornbit_t **phlog_tornbitp)
{
	m_phlog_tornbit_t      *phlog_tornbit;
	
	if (posix_memalign(&phlog_tornbit, sizeof(uint64_t),sizeof(m_phlog_tornbit_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	*phlog_tornbitp = phlog_tornbit;

	return M_R_SUCCESS;
}


/**
 * \brief Initializes the volatile log descriptor using the non-volatile 
 * metadata referenced by log_dsc.
 */
m_result_t
m_phlog_tornbit_init (m_phlog_tornbit_t *phlog, 
                      m_phlog_tornbit_nvmd_t *nvmd,
                      scm_word_t *nvphlog)					  
{
	scm_word_t             tornbit;

	phlog->nvmd = nvmd;
	phlog->nvphlog = nvphlog;
	tornbit = LF_TORNBIT & phlog->nvmd->flags;
	printf("phlog_tornbit_init: %lx\n", tornbit);
	phlog->tornbit = tornbit;
	phlog->buffer_count = 0;
	phlog->write_remainder = 0x0;
	phlog->write_remainder_nbits = 0;
	phlog->read_remainder = 0x0;
	phlog->read_remainder_nbits = 0;
	phlog->head = phlog->tail = phlog->stable_tail = phlog->read_index = phlog->nvmd->flags & LF_HEAD_MASK;

	return M_R_SUCCESS;
}


m_result_t
m_phlog_tornbit_truncate(m_phlog_tornbit_t *phlog) 
{
	scm_word_t        tornbit;

	tornbit = LF_TORNBIT & phlog->nvmd->flags;
	/*
	 * If head wraps around, the torn bit stored in the non-volatile 
	 * metadata is flipped.
	 */
	if (phlog->head > phlog->read_index) {
		tornbit = ~tornbit & TORN_MASK;
	}

	phlog->head = phlog->read_index;

	PCM_STORE(&phlog->nvmd->flags, phlog->head | tornbit);
	
	return M_R_SUCCESS;
}



void m_phlog_print_buffer(m_phlog_tornbit_t *log)
{
	int i;
	for (i=0; i<log->buffer_count; i++) {
		printf("buffer[%d] = %lX (", i, log->buffer[i]);
		print_binary64(log->buffer[i]);
		printf(")\n");
	}
}
