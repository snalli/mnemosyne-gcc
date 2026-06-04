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
 * \brief Simple base physical log.
 *
 * INVARIANTS:
 *
 * 1. Head and tail always advance by the number of buffer entries.
 *    
 *    For example if cache-line size is 64 bytes and word is 64-bits,
 *    then the buffer has 8 entries to ensuere that there would always
 *    be a complete cacheline to write to the hardware WC buffer. 
 *    Since buffer has 8 entries, we advance head and tail by 8 entries
 *    as well. In the case when we flush the log, the buffer might not
 *    be full. We still flush 8 entries even if the buffer is full and
 *    we advance tail by 8. This simplifies and makes bounds checking
 *    faster by requiring less conditions (less branches, no-memory fences).
 *
 * 2. todo: state the rest of the invariants
 *
 * TERMINOLOGY:
 *
 * Atomic log fragment: a region of the log guaranteed to be made persistent.
 *
 */

#ifndef _PHYSICAL_LOG_BASE_H
#define _PHYSICAL_LOG_BASE_H

/* System header files */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
/* Mnemosyne common header files */
#include <result.h>
#include <list.h>
#include "../hal/pcm_i.h"
#include "log_i.h"


#ifdef __cplusplus
extern "C" {
#endif


#define CHUNK_SIZE              64


typedef struct m_phlog_base_s      m_phlog_base_t;
typedef struct m_phlog_base_nvmd_s m_phlog_base_nvmd_t;

struct m_phlog_base_nvmd_s {
	pcm_word_t generic_flags;
	pcm_word_t head;                         
	pcm_word_t tail;     
	pcm_word_t reserved4;                /**< reserved for future use */
	// char padding[32];
};/* 32 bytes, possibly. Two of these can reside in one cache line */




/** 
 * The volatile representation of the physical log 
 *
 * To allow a consumer read metadata and values without locking we need to 
 * ensure that writes are atomic. x86 guarantees atomicity of word writes 
 * if the words are aligned. All fields are word-sized so that when the 
 * whole structure is word aligned, each field is word-aligned.
 */
struct m_phlog_base_s {
	uint64_t                buffer[CHUNK_SIZE/sizeof(uint64_t)];    /**< software buffer to collect log writes till we form a complete chunk */
	uint64_t                buffer_count;                           /**< number of valid words in the buffer */
	uint64_t                *nvphlog;                               /**< points to the non-volatile physical log */
	m_phlog_base_nvmd_t     *nvmd;                                  /**< points to the non-volatile metadata */
	uint64_t                head;
	uint64_t                tail;
	uint64_t                read_index;

	/* statistics */
	uint64_t                pad1[8];                                /**< some padding to avoid having statistics in the same cacheline with metadata */
	uint64_t                stat_wait_for_trunc;                    /**< number of times waited for asynchronous truncation */
	uint64_t                stat_wait_time_for_trunc;               /**< total time waited for asynchronous truncation */
};



/**
 * \brief Writes the contents of the log buffer to the actual log.
 *
 * Does not guarantee that the contents actually made it to persistent memory. 
 * A separate action to flush the log to memory is needed. 
 */
static inline
void
base_write_buffer2log(pcm_storeset_t *set, m_phlog_base_t *log)
{
	/* 
	 * Modulo arithmetic is implemented using the most efficient equivalent:
	 * (log->tail + k) % PHYSICAL_LOG_NUM_ENTRIES == (log->tail + k) & (PHYSICAL_LOG_NUM_ENTRIES-1)
	 */
	PCM_SEQSTREAM_STORE_64B_FIRST_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+0)], 
	                                   (pcm_word_t) log->buffer[0]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+1)], 
	                                  (pcm_word_t) log->buffer[1]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+2)], 
	                                  (pcm_word_t) log->buffer[2]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+3)], 
	                                  (pcm_word_t) log->buffer[3]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+4)], 
	                                  (pcm_word_t) log->buffer[4]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+5)], 
	                                  (pcm_word_t) log->buffer[5]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+6)], 
	                                  (pcm_word_t) log->buffer[6]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+7)], 
	                                  (pcm_word_t) log->buffer[7]);
	log->buffer_count=0;
	log->tail = (log->tail+8) & (PHYSICAL_LOG_NUM_ENTRIES-1);
}


/** 
 * \brief Writes a given value to the physical log. 
 *
 * Value is first buffered and only written to the log when a complete
 * chunk is formed or when the log is flushed explicitly via 
 * m_phlog_flush.
 */
static inline
m_result_t
m_phlog_base_write(pcm_storeset_t *set, m_phlog_base_t *log, pcm_word_t value)
{
	/* 
	 * Check there is space in the buffer and in the log before writing the
	 * new value. This duplicates some code but doesn't require unrolling state
	 * in case of any error. We could also avoid duplication by putting some
	 * extra branches but we prefer as few branches as possible in the critical
	 * path.
	 */

	/* Will new write fill buffer and require flushing out to log? */
	if (log->buffer_count+1 > CHUNK_SIZE/sizeof(pcm_word_t)-1) {
		/* Will log overflow? */
		if (((log->tail + CHUNK_SIZE/sizeof(pcm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1))
		    == log->head)
		{
			return M_R_FAILURE;
		} else {
			log->buffer[log->buffer_count] = value; 
			log->buffer_count++;
			base_write_buffer2log(set, log);
		}
	} else {
		log->buffer[log->buffer_count] = value;
		log->buffer_count++;
	}

	return M_R_SUCCESS;
}


/**
 * \brief Flushes the log to SCM memory.
 *
 * Any pending buffered writes are written to the log. Then the 
 * log is forced to SCM memory.
 *
 */
static inline
m_result_t
m_phlog_base_flush(pcm_storeset_t *set, m_phlog_base_t *log)
{
	if (log->buffer_count > 0) { /* freud : There are still some stores left in buffer */
		base_write_buffer2log(set, log);
	}	
	PCM_SEQSTREAM_FLUSH(set); /* freud : necessary fence */
	PCM_NT_STORE(set, (volatile pcm_word_t *) &log->nvmd->tail, 
	             (pcm_word_t) log->tail);
	PCM_WB_FENCE(set); /* freud : necessary fence */
	// nvmd->tail and nvmd->head are in the same cacheline; hence self-dependency.
	return M_R_SUCCESS;
}


/**
 * \brief Reads a single word from the stable part of the log. 
 *
 * The stable part is the one that has made it to SCM memory.
 *
 */
static inline
m_result_t
m_phlog_base_read(m_phlog_base_t *log, uint64_t *valuep)
{
	uint64_t value;

	/* Are there any stable data to read? */
	if (log->read_index != log->nvmd->tail) {
		value = log->nvphlog[log->read_index];
		log->read_index = (log->read_index + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
		*valuep = value;
		return M_R_SUCCESS;
	}
	return M_R_FAILURE;
}


/**
 * \brief Checks whether there is a stable part of the log to read. 
 *
 * The stable part is the one that has made it to SCM memory.
 *
 */
static inline
bool
m_phlog_base_stable_exists(m_phlog_base_t *log)
{
	if (log->read_index != log->nvmd->tail) {
		return true;
	}
	return false;
}



/** 
 * \brief Moves read pointer to next available chunk and ignores 
 * the rest of the data of the current chunk.
 *
 */
static inline
void
m_phlog_base_next_chunk(m_phlog_base_t *log)
{
	uint64_t read_index; 

	read_index = log->read_index & ~(CHUNK_SIZE/sizeof(pcm_word_t) - 1);
	/* 
	 * If current log->read_index points to the beginning of a chunk
	 * then we are already in the next chunk so we don't need to advance.
	 */
	if (read_index != log->read_index) {
		log->read_index = (read_index + CHUNK_SIZE/sizeof(pcm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1); 
	}
}


/**
 * \brief Checkpoints the read pointer to allow re-reading data from the
 * checkpointed point. 
 * 
 * Cannot checkpoint in the middle of reading an atomic log fragment.
 */
static inline
m_result_t
m_phlog_base_checkpoint_readindex(m_phlog_base_t *log, uint64_t *readindex)
{
	*readindex = log->read_index;

	return M_R_SUCCESS;
}


/**
 * \brief Restored the read pointer.
 * 
 * Assumes that the checkpoint was not taken while in the middle of reading 
 * an atomic log fragment.
 */
static inline
void
m_phlog_base_restore_readindex(m_phlog_base_t *log, uint64_t readindex)
{
	log->read_index = readindex;
}


static inline
m_result_t
m_phlog_base_truncate_sync(pcm_storeset_t *set, m_phlog_base_t *phlog) 
{
	phlog->head = phlog->tail;
	/* freud : truncating the log by advancing the head, you could pull back the tail */
	PCM_NT_STORE(set, (volatile pcm_word_t *) &phlog->nvmd->head, 
	             (pcm_word_t) phlog->head);
	PCM_NT_FLUSH(set);
	
	return M_R_SUCCESS;
}


m_result_t m_phlog_base_format (pcm_storeset_t *set, m_phlog_base_nvmd_t *nvmd, pcm_word_t *nvphlog, int type);
m_result_t m_phlog_base_alloc (m_phlog_base_t **phlog_basep);
m_result_t m_phlog_base_init (m_phlog_base_t *phlog, m_phlog_base_nvmd_t *nvmd, pcm_word_t *nvphlog);
m_result_t m_phlog_base_check_consistency(m_phlog_base_nvmd_t *nvmd, pcm_word_t *nvphlog, uint64_t *stable_tail);
m_result_t m_phlog_base_truncate_async(pcm_storeset_t *set, m_phlog_base_t *phlog);


#ifdef __cplusplus
}
#endif

#endif /* _PHYSICAL_LOG_BASE_H */
