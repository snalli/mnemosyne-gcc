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

#ifndef _PHYSICAL_LOG_TORNBIT_H
#define _PHYSICAL_LOG_TORNBIT_H

/* System header files */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
/* Mnemosyne common header files */
#include <result.h>
#include <list.h>
#include "log.h"


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
	scm_word_t generic_flags;
	scm_word_t flags;                         /**< the MSB part is reserved for flags while the LSB part stores the head index */
	scm_word_t reserved3;                     /**< reserved for future use */
	scm_word_t reserved4;                     /**< reserved for future use */
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

#if 0
/**
 * This version pays some extra overhead in arithmetic operations to avoid using a branch
 */
static inline
void
write_buffer2log(m_phlog_tornbit_t *log)
{
	/* 
	 * Modulo arithmetic is implemented using the most efficient equivalent:
	 * (log->tail + k) % PHYSICAL_LOG_NUM_ENTRIES == (log->tail + k) & (PHYSICAL_LOG_NUM_ENTRIES-1)
	 */
	printf("write_buffer2log: BEFORE_WRITE: log->tail = %d, tornbit = %lx\n", log->tail, log->tornbit[0]);
	PCM_STORE(&log->nvphlog[(log->tail+0) & (PHYSICAL_LOG_NUM_ENTRIES-1)], 
			  log->buffer[0] | log->tornbit[((log->tail+0) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+1) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[1] | log->tornbit[((log->tail+1) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+2) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[2] | log->tornbit[((log->tail+2) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+3) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[3] | log->tornbit[((log->tail+3) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+4) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[4] | log->tornbit[((log->tail+4) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+5) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[5] | log->tornbit[((log->tail+5) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+6) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[6] | log->tornbit[((log->tail+6) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);
	PCM_STORE(&log->nvphlog[(log->tail+7) & (PHYSICAL_LOG_NUM_ENTRIES-1)],
			  log->buffer[7] | log->tornbit[((log->tail+7) & ~(PHYSICAL_LOG_NUM_ENTRIES-1)) >> PHYSICAL_LOG_NUM_ENTRIES_LOG2]);

	log->buffer_count=0;
	log->tail = (log->tail+8) & (PHYSICAL_LOG_NUM_ENTRIES-1);
	if (log->tail == 0x0) {
		/* flip tornbits */
		int i;
		for (i=0; i<CHUNK_SIZE/sizeof(scm_word_t); i++) {
			log->tornbit[i] = (log->tornbit[i] == TORNBIT_ONE) ? TORNBIT_ZERO : TORNBIT_ONE;
		}
	}
	printf("write_buffer2log: AFTER_WRITE : log->tail = %d\n", log->tail);
}
#endif


/**
 * \brief Writes the contents of the log buffer to the actual log.
 *
 * Does not guarantee that the contents actually made it to persistent memory. 
 * A separate action to flush the log to memory is needed. 
 */
static inline
void
write_buffer2log(m_phlog_tornbit_t *log)
{
	/* 
	 * Modulo arithmetic is implemented using the most efficient equivalent:
	 * (log->tail + k) % PHYSICAL_LOG_NUM_ENTRIES == (log->tail + k) & (PHYSICAL_LOG_NUM_ENTRIES-1)
	 */
	printf("write_buffer2log: BEFORE_WRITE: log->tail = %lu, tornbit = %lx\n", log->tail, log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+0)], log->buffer[0] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+1)], log->buffer[1] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+2)], log->buffer[2] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+3)], log->buffer[3] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+4)], log->buffer[4] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+5)], log->buffer[5] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+6)], log->buffer[6] | log->tornbit);
	PCM_STORE(&log->nvphlog[(log->tail+7)], log->buffer[7] | log->tornbit);

	log->buffer_count=0;
	log->tail = (log->tail+8) & (PHYSICAL_LOG_NUM_ENTRIES-1);

	/* Flip tornbit if wrap around */
	if (log->tail == 0x0) {
		log->tornbit = ~(log->tornbit) & TORN_MASK;
	}
	printf("write_buffer2log: AFTER_WRITE : log->tail = %lu\n", log->tail);
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
m_phlog_tornbit_write(m_phlog_tornbit_t *log, scm_word_t value)
{
	printf("log_write:  0x%016lX (", value);
	//print_binary64(value);
	printf(")\n");

	/* 
	 * Check there is space in the buffer and in the log before writing the
	 * new value. This duplicates some code but doesn't require unrolling state
	 * in case of any error. We could also avoid duplication by putting some
	 * extra branches but we prefer as few branches as possible in the critical
	 * path.
	 */

	/* Will new write flush buffer out to log? */
	if (log->buffer_count+1 > CHUNK_SIZE/sizeof(scm_word_t)-1) {
		/* Will log overflow? */
		if (((log->tail + CHUNK_SIZE/sizeof(scm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1))
		    == log->head)
		{
			printf("LOG OVERFLOW!!!\n");
			printf("tail: %lu\n", log->tail);
			printf("head: %lu\n", log->head);
			return M_R_FAILURE;
		} else {
			log->buffer[log->buffer_count] = ~TORN_MASK & 
											 (log->write_remainder | 
											  (value << log->write_remainder_nbits));
			/* 
			 * Invariant: Remainder bits cannot be more than 63. So one buffer 
			 * slot should suffice. 
			 */
			log->write_remainder_nbits = (log->write_remainder_nbits + 1) & (64 - 1); /* efficient form of (log->write_remainder_nbits + 1) % 64 */
			log->write_remainder = value >> (64 - log->write_remainder_nbits);
			log->buffer_count++;

			write_buffer2log(log);
		}
	} else {
		log->buffer[log->buffer_count] = ~TORN_MASK & 
										 (log->write_remainder | 
										  (value << log->write_remainder_nbits));
		/* 
		 * Invariant: Remainder bits cannot be more than 63. So one buffer 
		 * slot should suffice. 
		 */
		log->write_remainder_nbits = (log->write_remainder_nbits + 1) & (64 - 1); /* efficient form of (log->write_remainder_nbits + 1) % 64 */
		log->write_remainder = value >> (64 - log->write_remainder_nbits);
		log->buffer_count++;
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
m_phlog_tornbit_flush(m_phlog_tornbit_t *log)
{
	printf("m_phlog_flush\n");
	if (log->write_remainder_nbits > 0) {
		/* Will log overflow? */
		if (((log->tail + CHUNK_SIZE/sizeof(scm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1))
		    == log->head) 
		{
			printf("FLUSH: LOG OVERFLOW!!!\n");
			printf("tail: %lu\n", log->tail);
			printf("head: %lu\n", log->head);
			printf("stable_tail: %lu\n", log->stable_tail);
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

			write_buffer2log(log);
		}	
	}
	log->stable_tail = log->tail;
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
m_phlog_tornbit_read(m_phlog_tornbit_t *log, uint64_t *valuep)
{
	uint64_t value;

	printf("log_read: %lu %lu\n", log->read_index, log->stable_tail);
	/* Are there any stable data to read? */
	if (log->read_index != log->stable_tail) {
		value = (~TORN_MASK & log->nvphlog[log->read_index]) >> log->read_remainder_nbits;
		log->read_index = (log->read_index + 1) & (PHYSICAL_LOG_NUM_ENTRIES - 1);
		value |= log->nvphlog[log->read_index] << (63 - log->read_remainder_nbits);		 
		log->read_remainder_nbits++;						  
		*valuep = value;
		return M_R_SUCCESS;
	}
	printf("NO DATA\n");
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
	log->read_remainder_nbits = 0;						  
	printf("m_phlog_next_chunk:       %lu\n", log->read_index);
	log->read_index = log->read_index & ~(CHUNK_SIZE/sizeof(scm_word_t) -1);
	printf("m_phlog_next_chunk: next: %lu\n", log->read_index);
	log->read_index = (log->read_index + CHUNK_SIZE/sizeof(scm_word_t)) & (PHYSICAL_LOG_NUM_ENTRIES-1); ;
	printf("m_phlog_next_chunk: next: %lu\n", log->read_index);
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


m_result_t m_phlog_tornbit_format (m_phlog_tornbit_nvmd_t *nvmd, scm_word_t *nvphlog, int type);
m_result_t m_phlog_tornbit_alloc (m_phlog_tornbit_t **phlog_tornbitp);
m_result_t m_phlog_tornbit_init (m_phlog_tornbit_t *phlog, m_phlog_tornbit_nvmd_t *nvmd, scm_word_t *nvphlog);
m_result_t m_phlog_tornbit_check_consistency(m_phlog_tornbit_nvmd_t *nvmd, scm_word_t *nvphlog, uint64_t *stable_tail);
m_result_t m_phlog_tornbit_prepare_truncate(m_log_dsc_t *log_dsc);
m_result_t m_phlog_tornbit_truncate(m_phlog_tornbit_t *phlog);


#ifdef __cplusplus
}
#endif

#endif /* _PHYSICAL_LOG_TORNBIT_H */
