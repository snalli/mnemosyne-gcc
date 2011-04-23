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
 * NON-VOLATILE PHYSICAL LOG FORMAT:
 *
 * +----------------------------------------------------------+ 
 * |                       64-bit WORD                        |
 * +-------------------------------+--------+-----------------+
 * |             Byte 7            |   ...  |      Byte 0     |
 * +---+---+---+---+---+---+---+---+--------+---+-----+---+---+
 * |b63|b62|b61|b60|b59|b58|b57|b56|        |b07| ... |b01|b00|
 * +---+---+---+---+---+---+---+---+--------+---+-----+---+---+
 * | T |                      Payload                         | 
 * +---+------------------------------------------------------+
 *
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

/*
 * FIXME: Tornbit currently appears to be working correctly and passing our 
 *        tests. Developing it was quite painful mainly due to some
 *        optimizations (to avoid branches), which is not clear whether 
 *        they paid off. Thus, the author fills a simpler, more robust 
 *        implementation with good performance should be possible. 
 *        Reconsider a simpler implementation...
 *        
 *
 */


#ifndef _PHYSICAL_LOG_TORNBIT_H
#define _PHYSICAL_LOG_TORNBIT_H

/* System header files */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
/* Mnemosyne common header files */
#include <result.h>
#include <list.h>
#include "../hal/pcm_i.h"
#include "log_i.h"

uint64_t load_nt_word(void *addr);

#undef _DEBUG_THIS 
//#define _DEBUG_THIS 1

#ifdef __cplusplus
extern "C" {
#endif


#define CHUNK_SIZE              64


#define TORN_MASK               0x8000000000000000LLU
#define TORN_MASKC              0x7FFFFFFFFFFFFFFFLLU /* complement of TORN_MASK */
#define TORNBIT_ZERO            0x0000000000000000LLU
#define TORNBIT_ONE             0x8000000000000000LLU

#define LF_TORNBIT              TORN_MASK
#define LF_HEAD_MASK            0x00000000FFFFFFFFLLU



typedef struct m_phlog_tornbit_s      m_phlog_tornbit_t;
typedef struct m_phlog_tornbit_nvmd_s m_phlog_tornbit_nvmd_t;

struct m_phlog_tornbit_nvmd_s {
	pcm_word_t generic_flags;
	pcm_word_t flags;                         /**< the MSB part is reserved for flags while the LSB part stores the head index */
	pcm_word_t reserved3;                     /**< reserved for future use */
	pcm_word_t reserved4;                     /**< reserved for future use */
};




/** 
 * The volatile representation of the physical log 
 *
 * To allow a consumer read metadata and values without locking we need to 
 * ensure that writes are atomic. x86 guarantees atomicity of word writes 
 * if the words are aligned. All fields are word-sized so that when the 
 * whole structure is word aligned, each field is word-aligned.
 */
struct m_phlog_tornbit_s {
	uint64_t                buffer[CHUNK_SIZE/sizeof(uint64_t)];    /**< software buffer to collect log writes till we form a complete chunk */
	uint64_t                buffer_count;                           /**< number of valid words in the buffer */
	uint64_t                write_remainder;                        /**< the remainder bits after the encoding operation */
	uint64_t                write_remainder_nbits;                  /**< number of the valid least-significant bits of the write_remainder buffer */
	uint64_t                read_remainder;                         /**< the remainder bits after the decoding operation */
	uint64_t                read_remainder_nbits;                   /**< number of the valid least-significant bits of the read_remainder buffer */
	uint64_t                *nvphlog;                               /**< points to the non-volatile physical log */
	m_phlog_tornbit_nvmd_t  *nvmd;                                  /**< points to the non-volatile metadata */
	uint64_t                head;
	uint64_t                tail;
	uint64_t                stable_tail;                            /**< data between head and stable_tail have been made persistent */
	uint64_t                read_index;
	uint64_t                tornbit;
	//uint64_t              tornbit[CHUNK_SIZE/sizeof(uint64_t)];
	
	/* statistics */
	uint64_t                pad1[8];                                /**< some padding to avoid having statistics in the same cacheline with metadata */
	uint64_t                stat_wait_for_trunc;                    /**< number of times waited for asynchronous truncation */
	uint64_t                stat_wait_time_for_trunc;               /**< total time waited for asynchronous truncation */
};


static 
void 
print_binary64(uint64_t val)
{
	int i;
	uint64_t mask = 0x8000000000000000LLU; 
	for (i=0; i<64; i++){
		if ((mask & val) > 0)
			printf("1");
		else
			printf("0");
		mask >>= 1;
	}
}


/**
 * \brief Writes the contents of the log buffer to the actual log.
 *
 * Does not guarantee that the contents actually made it to persistent memory. 
 * A separate action to flush the log to memory is needed. 
 */
static inline
void
tornbit_write_buffer2log(pcm_storeset_t *set, m_phlog_tornbit_t *log)
{
	/* 
	 * Modulo arithmetic is implemented using the most efficient equivalent:
	 * (log->tail + k) % PHYSICAL_LOG_NUM_ENTRIES == (log->tail + k) & (PHYSICAL_LOG_NUM_ENTRIES-1)
	 */
#ifdef _DEBUG_THIS		
	printf("tornbit_write_buffer2log: log->tail = %llu\n", log->tail);	 
#endif	

	PCM_SEQSTREAM_STORE_64B_FIRST_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+0)], 
	                                   log->tornbit | (pcm_word_t) log->buffer[0]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+1)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[1]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+2)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[2]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+3)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[3]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+4)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[4]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+5)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[5]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+6)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[6]);
	PCM_SEQSTREAM_STORE_64B_NEXT_WORD(set, (volatile pcm_word_t *) &log->nvphlog[(log->tail+7)], 
	                                  log->tornbit | (pcm_word_t) log->buffer[7]);

	log->buffer_count=0;
	log->tail = (log->tail+8) & (PHYSICAL_LOG_NUM_ENTRIES-1);

	/* Flip tornbit if wrap around */
	if (log->tail == 0x0) {
		log->tornbit = ~(log->tornbit) & TORN_MASK;
	}
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
m_phlog_tornbit_write(pcm_storeset_t *set, m_phlog_tornbit_t *log, pcm_word_t value)
{
	/* 
	 * Check there is space in the buffer and in the log before writing the
	 * new value. This duplicates some code but doesn't require unrolling state
	 * in case of any error. We could also avoid duplication by putting some
	 * extra branches but we prefer as few branches as possible in the critical
	 * path.
	 */

#ifdef _DEBUG_THIS
	printf("buffer_count = %lu, write_remainder_nbits = %lu, log->head=%lu, log->tail=%lu\n", log->buffer_count, log->write_remainder_nbits, log->head, log->tail);
	printf("value = 0x%llX\n", value);
	printf("[T: %x] &log->tail= %p\n", pthread_self(), &log->tail);
#endif
	/* Will new write flush buffer out to log? */
	if (log->buffer_count+1 > CHUNK_SIZE/sizeof(pcm_word_t)-1) {
		/* UNCOMMON PATH */
		/* Will log overflow? */
		if (((log->tail + CHUNK_SIZE/sizeof(pcm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1))
		    == log->head)
		{
#ifdef _DEBUG_THIS
			printf("LOG OVERFLOW!!!\n");
			printf("tail: %lu\n", log->tail);
			printf("head: %lu\n", log->head);
#endif			
			return M_R_FAILURE;
		} else {
#ifdef _DEBUG_THIS		
			printf("FLUSH BUFFER OUT\n");
#endif			
			log->buffer[log->buffer_count] = ~TORN_MASK & 
											 (log->write_remainder | 
											  (value << log->write_remainder_nbits));
			/* the new remainder is the part of the value not written to the buffer */								  
			log->write_remainder = value >> (64 - (log->write_remainder_nbits+1));
			log->write_remainder_nbits = (log->write_remainder_nbits + 1) & (64 - 1); /* efficient form of (log->write_remainder_nbits + 1) % 64 */
			//log->buffer_count++; /* do we need this inc? write_buffer2log sets this to zero */

			tornbit_write_buffer2log(set, log); /* write_buffer2log resets buffer_count to zero */
			/* 
			 * If write_remainder_nbits wrapped around then we are actually left 
			 * with an overflow remainder of 64 bits, not zero. Write a complete 
			 * buffer entry using 63 of the 64 bits. 
			 */
			if (log->write_remainder_nbits == 0) {
				log->buffer[0] = ~TORN_MASK & 
				                 (log->write_remainder);
				log->buffer_count++;
				//log->write_remainder = value >> 63;
				/* the new remainder is the 64th bit that didn't make it */
				log->write_remainder = log->write_remainder >> 63;
				log->write_remainder_nbits = 1;
			}
		}
	} else {
		/* COMMON PATH */
		/* 
		 * Store the remainder bits in the lowest part of the buffer word
		 * and fill the highest part of the buffer word with a fragment of
		 * the value.
		 *
		 * +-+--------------------------------+------------------------+
		 * |T| value << write_remainder_nbits | write_remainder        |
		 * +-+--------------------------------+------------------------+
		 *
		 */
		log->buffer[log->buffer_count] = ~TORN_MASK & 
										 (log->write_remainder | 
										  (value << log->write_remainder_nbits));
		/* 
		 * Invariant: Remainder bits cannot be more than 63. So one buffer 
		 * slot should suffice. 
		 */
		log->write_remainder_nbits = (log->write_remainder_nbits + 1) & (64 - 1); 
		log->write_remainder = value >> (64 - log->write_remainder_nbits);
		log->buffer_count++;
		/* 
		 * Note: It would be easier to check whether there are 63 remaining
		 * bits and write them to the buffer. However this would add an 
		 * extra branch in the common path. We avoid this by doing the check
		 * in the uncommon path above instead. This is correct because we go 
		 * through the uncommon path every 8th write, so when we are left with 
		 * 64 bits to write out (i.e 63 bits we had to write + 1 new bit), we 
		 * will be in the uncommon path already.
		 */
	}
	return M_R_SUCCESS;
}


/**
 * \brief Flushes the log to SCM memory.
 *
 * Any outstanding buffered writes are written to the log. Then the 
 * log is forced to SCM memory.
 *
 */
static inline
m_result_t
m_phlog_tornbit_flush(pcm_storeset_t *set, m_phlog_tornbit_t *log)
{
#ifdef _DEBUG_THIS		
	printf("m_phlog_flush\n");
  	printf("nvmd       : %p\n", log->nvmd);
  	printf("nvphlog    : %p\n", log->nvphlog);
#endif	
	if (log->write_remainder_nbits > 0) {
		/* Will log overflow? */
		if (((log->tail + CHUNK_SIZE/sizeof(pcm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1))
		    == log->head) 
		{
#ifdef _DEBUG_THIS		
			printf("FLUSH: LOG OVERFLOW!!!\n");
			printf("tail: %lu\n", log->tail);
			printf("head: %lu\n", log->head);
			printf("stable_tail: %lu\n", log->stable_tail);
#endif			
			return M_R_FAILURE;
		} else {
			/* 
			 * Invariant: After each log_write we check whether buffer is full. 
			 * Therefore when we get here we should have at least one slot free.
			 *
			 * Invariant: Remainder bits cannot be more than 63. So one buffer
			 * slot should suffice.
			 */
			log->buffer[log->buffer_count] = ~TORN_MASK & 
											 (log->write_remainder);
			log->buffer_count++;
			log->write_remainder_nbits = 0;
			log->write_remainder = 0;
			/* 
			 * Write the buffer out to log even if it is not completely full. 
			 * Simplifies and makes bound checking faster: no extra branches, 
			 * no memory-fences.
			 */
			tornbit_write_buffer2log(set, log);
		}	
	}
	log->stable_tail = log->tail;
	PCM_SEQSTREAM_FLUSH(set);
#ifdef _DEBUG_THIS		
	printf("phlog_tornbit_flush: log->tail = %llu, log->stable_tail = %llu\n", log->tail, log->stable_tail);
#endif	
	return M_R_SUCCESS;
}


/**
 * \brief Reads a single word from the stable part of the log. 
 *
 * The stable part is the one that has made it to SCM memory.
 *
 */
#if  0
static inline
m_result_t
m_phlog_tornbit_read(m_phlog_tornbit_t *log, uint64_t *valuep)
{
	uint64_t value;

#ifdef _DEBUG_THIS		
	printf("log_read: %lu %lu %lu\n", log->read_index, (uint64_t) log->read_remainder_nbits, log->stable_tail);
#endif
	/* Are there any stable data to read? */
	if (log->read_index != log->stable_tail && 
	    log->read_index + 1 != log->stable_tail) 
	{
#ifdef _DEBUG_THIS		
		printf("[%04lu]: 0x%016llX\n", log->read_index, ~TORN_MASK & log->nvphlog[log->read_index]);
#endif		
		value = (~TORN_MASK & log->nvphlog[log->read_index]) >> log->read_remainder_nbits;
		log->read_index = (log->read_index + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
#ifdef _DEBUG_THIS		
		printf("[%04lu]: 0x%016llX\n", log->read_index, ~TORN_MASK & log->nvphlog[log->read_index]);
#endif		
		value |= log->nvphlog[log->read_index] << (63 - log->read_remainder_nbits);
		log->read_remainder_nbits = (log->read_remainder_nbits + 1) & (64 - 1);
		if (log->read_remainder_nbits == 63) {
			log->read_index = (log->read_index + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
			log->read_remainder_nbits = 0;
		}
		*valuep = value;
#ifdef _DEBUG_THIS		
		printf("\t read value: 0x%016lX\n", value);
#endif		
		return M_R_SUCCESS;
	}
#ifdef _DEBUG_THIS		
	printf("NO DATA\n");
#endif		
	return M_R_FAILURE;
}
#else

static inline
m_result_t
m_phlog_tornbit_read(m_phlog_tornbit_t *log, uint64_t *valuep)
{
	uint64_t value;
	uint64_t tmp;

#ifdef _DEBUG_THIS		
	printf("log_read: %lu %lu %lu\n", log->read_index, (uint64_t) log->read_remainder_nbits, log->stable_tail);
#endif
	/* Are there any stable data to read? */
	if (log->read_index != log->stable_tail && 
	    log->read_index + 1 != log->stable_tail) 
	{
#ifdef _DEBUG_THIS		
		printf("[%04lu]: 0x%016llX\n", log->read_index, ~TORN_MASK & log->nvphlog[log->read_index]);
#endif		
		tmp = load_nt_word(&log->nvphlog[log->read_index]);
		value = (~TORN_MASK & tmp) >> log->read_remainder_nbits;
		log->read_index = (log->read_index + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
#ifdef _DEBUG_THIS		
		printf("[%04lu]: 0x%016llX\n", log->read_index, ~TORN_MASK & log->nvphlog[log->read_index]);
#endif		
		tmp = load_nt_word(&log->nvphlog[log->read_index]);
		value |= tmp << (63 - log->read_remainder_nbits);
		log->read_remainder_nbits = (log->read_remainder_nbits + 1) & (64 - 1);
		if (log->read_remainder_nbits == 63) {
			log->read_index = (log->read_index + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
			log->read_remainder_nbits = 0;
		}
		*valuep = value;
#ifdef _DEBUG_THIS		
		printf("\t read value: 0x%016lX\n", value);
#endif		
		return M_R_SUCCESS;
	}
#ifdef _DEBUG_THIS		
	printf("NO DATA\n");
#endif		
	return M_R_FAILURE;
}
#endif

/**
 * \brief Checks whether there is a stable part of the log to read. 
 *
 * The stable part is the one that has made it to SCM memory.
 *
 */
static inline
bool
m_phlog_tornbit_stable_exists(m_phlog_tornbit_t *log)
{
	if (log->read_index != log->stable_tail) {
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
m_phlog_tornbit_next_chunk(m_phlog_tornbit_t *log)
{
	uint64_t read_index; 

	if (log->read_remainder_nbits > 0) {
		log->read_remainder_nbits = 0;
		read_index = log->read_index & ~(CHUNK_SIZE/sizeof(pcm_word_t) - 1);
		log->read_index = (read_index + CHUNK_SIZE/sizeof(pcm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1); 
	} else {
		log->read_remainder_nbits = 0;
		read_index = log->read_index & ~(CHUNK_SIZE/sizeof(pcm_word_t) - 1);
		/* 
		 * If current log->read_index points to the beginning of a chunk
		 * then we are already in the next chunk so we don't need to advance.
		 */
		if (read_index != log->read_index) {
			log->read_index = (read_index + CHUNK_SIZE/sizeof(pcm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1); 
		}
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
m_phlog_tornbit_checkpoint_readindex(m_phlog_tornbit_t *log, uint64_t *readindex)
{
	if (log->read_remainder_nbits > 0) {
		return M_R_FAILURE;
	}
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
m_phlog_tornbit_restore_readindex(m_phlog_tornbit_t *log, uint64_t readindex)
{
	log->read_index = readindex;
	log->read_remainder_nbits = 0;
}


static inline
m_result_t
m_phlog_tornbit_truncate_sync(pcm_storeset_t *set, m_phlog_tornbit_t *phlog) 
{
	phlog->head = phlog->tail;

	//FIXME: do we need a flush? PCM_NT_FLUSH(set);
	PCM_NT_STORE(set, (volatile pcm_word_t *) &phlog->nvmd->flags, (pcm_word_t) (phlog->head | phlog->tornbit));
	PCM_NT_FLUSH(set);
	
	return M_R_SUCCESS;
}

m_result_t m_phlog_tornbit_format (pcm_storeset_t *set, m_phlog_tornbit_nvmd_t *nvmd, pcm_word_t *nvphlog, int type);
m_result_t m_phlog_tornbit_alloc (m_phlog_tornbit_t **phlog_tornbitp);
m_result_t m_phlog_tornbit_init (m_phlog_tornbit_t *phlog, m_phlog_tornbit_nvmd_t *nvmd, pcm_word_t *nvphlog);
m_result_t m_phlog_tornbit_check_consistency(m_phlog_tornbit_nvmd_t *nvmd, pcm_word_t *nvphlog, uint64_t *stable_tail);
m_result_t m_phlog_tornbit_prepare_truncate(m_log_dsc_t *log_dsc);
m_result_t m_phlog_tornbit_truncate_async(pcm_storeset_t *set, m_phlog_tornbit_t *phlog);


#ifdef __cplusplus
}
#endif

#endif /* _PHYSICAL_LOG_TORNBIT_H */
