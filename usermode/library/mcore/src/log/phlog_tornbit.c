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

/**
 * \file
 * 
 * \brief Physical log implemented using a torn-bit per word to detect
 * torn words. 
 *
 */

#define __SSE4_1__
/* System header files */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
/* Mnemosyne common header files */
#include <result.h>
#include "phlog_tornbit.h"
#include "hal/pcm_i.h"
#include <smmintrin.h>


uint64_t load_nt_word(void *addr)
{
	union {
		__m128i x;
		char b[16];
	} u;

	uintptr_t aladdr;
	uintptr_t offset;

	aladdr = ((uintptr_t) addr) & ~0xF;
	offset = ((uintptr_t) addr) & 0xF;

	u.x = _mm_stream_load_si128((__m128i *) aladdr);

	return *((uint64_t *) &u.b[offset]);
}	



/**
 * \brief Check the consistency of the non-volatile log and find the consistent
 * stable region starting from the head.
 */
m_result_t
m_phlog_tornbit_check_consistency(m_phlog_tornbit_nvmd_t *nvmd, 
                                  pcm_word_t *nvphlog,
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
		if (tornbit != valid_tornbit) {
			*stable_tail = i;
			break;
		}
		i = (i + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
		if (i==0) {
			flip_tornbit = 1;
			valid_tornbit = TORN_MASK & ~valid_tornbit;
		}
		if (i==head_index) {
			break;
		}
	}

	return M_R_SUCCESS;
}


static
m_result_t
tornbit_format_nvlog (pcm_storeset_t *set,
                      uint32_t head_index, 
                      uint64_t tornbit, 
                      m_phlog_tornbit_nvmd_t *nvmd, 
                      pcm_word_t *nvphlog)
{
	int i;

	PCM_NT_STORE(set, (volatile pcm_word_t *) &nvmd->flags, head_index | tornbit);
	for (i=0; i<PHYSICAL_LOG_NUM_ENTRIES; i++) {
		if (i<head_index) {
			if ((nvphlog[i] & TORN_MASK) != tornbit) {
				PCM_NT_STORE(set, (volatile pcm_word_t *) &nvphlog[i], tornbit);
			}	
		} else {
			if ((nvphlog[i] & TORN_MASK) == tornbit) {
				PCM_NT_STORE(set, (volatile pcm_word_t *) &nvphlog[i], ~tornbit & TORN_MASK);
			}	
		}
	}
	PCM_NT_FLUSH(set);
	
	return M_R_SUCCESS;
}


/**
 * \brief Formats the non-volatile physical log for reuse.
 */
m_result_t
m_phlog_tornbit_format (pcm_storeset_t *set, 
                        m_phlog_tornbit_nvmd_t *nvmd, 
                        pcm_word_t *nvphlog, 
                        int type)
{
	m_result_t             rv = M_R_FAILURE;
	
	if ((nvmd->generic_flags & LF_TYPE_MASK) == type) {
		/* 
		 * TODO: Optimization: check consistency and perform a quick format 
		 * instead.
		 */
		rv = tornbit_format_nvlog (set, 0, TORNBIT_ONE, nvmd, nvphlog);
		if (rv != M_R_SUCCESS) {
			goto out;
		}
	} else {
		rv = tornbit_format_nvlog (set, 0, TORNBIT_ONE, nvmd, nvphlog);
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

	if (posix_memalign((void **) &phlog_tornbit, sizeof(uint64_t),sizeof(m_phlog_tornbit_t)) != 0) 
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
                      pcm_word_t *nvphlog)					  
{
	pcm_word_t             tornbit;

	phlog->nvmd = nvmd;
	phlog->nvphlog = nvphlog;
	tornbit = LF_TORNBIT & phlog->nvmd->flags;
	phlog->tornbit = tornbit;
	phlog->buffer_count = 0;
	phlog->write_remainder = 0x0;
	phlog->write_remainder_nbits = 0;
	phlog->read_remainder = 0x0;
	phlog->read_remainder_nbits = 0;
	phlog->head = phlog->tail = phlog->stable_tail = phlog->read_index = phlog->nvmd->flags & LF_HEAD_MASK;

	/* initialize statistics */
	phlog->stat_wait_for_trunc = 0;
	phlog->stat_wait_time_for_trunc = 0;
	return M_R_SUCCESS;
}


/**
 * \brief Truncates the log up to the read_index point.
 */
m_result_t
m_phlog_tornbit_truncate_async(pcm_storeset_t *set, m_phlog_tornbit_t *phlog) 
{
	pcm_word_t        tornbit;

	tornbit = LF_TORNBIT & phlog->nvmd->flags;
	/*
	 * If head is larger than the current read_index point, then the assignment
	 * of read_index to the head will cause head to wrap around, thus the 
	 * torn bit stored in the non-volatile metadata is flipped.
	 */
	if (phlog->head > phlog->read_index) {
		tornbit = ~tornbit & TORN_MASK;
	}

	phlog->head = phlog->read_index;

	//FIXME: do we need a flush? PCM_NT_FLUSH(set);
	PCM_NT_STORE(set, (volatile pcm_word_t *) &phlog->nvmd->flags, (pcm_word_t) (phlog->head | tornbit));
	PCM_NT_FLUSH(set);
	
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
