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

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <mmintrin.h>
#include <list.h>
#include <spinlock.h>
#include "cuckoo_hash/PointerHashInline.h"
#include "pcm_i.h"


/* Private types */

typedef uint64_t cacheline_bitmask_t;

typedef struct pcm_storeset_list_s pcm_storeset_list_t; 

struct pcm_storeset_list_s {
	uint32_t          count;
	struct list_head  list;
	volatile uint8_t  outstanding_crash;
	pthread_mutex_t   lock;
	pthread_cond_t    cond_halt;
	pthread_cond_t    cond_wait_storesets_halt;
	volatile uint32_t waiters;
};

										  
/* 
 * The CDF that there will be a partial crash after we write the N first bytes
 * of a cacheline given there is a crash. SUM(cdf[0..8]) must be 1000.
 * cdf[I] means: crash after writing (I+1)*sizeof(word) bytes
 *
 * Example: For a cache line of size 64 bytes and word size of 8 bytes: 
 * cdf[0] --> crash after writing 0 bytes
 * cdf[1] --> crash after writing 8 bytes
 * ...
 * cdf[7] --> crash after writing 56 bytes
 * cdf[8] --> crash after writing 64 bytes (no partial crash)
 */
typedef struct cacheline_crash_cdf_s cacheline_crash_cdf_t;

struct cacheline_crash_cdf_s {
	int word[CACHELINE_SIZE/sizeof(pcm_word_t)+1];
};


typedef struct cacheline_s cacheline_t;

struct cacheline_s {
	cacheline_bitmask_t bitmask;
	uint8_t             bytes[CACHELINE_SIZE];
};


struct cacheline_tbl_s {
	cacheline_t   *cachelines;
	unsigned int  cachelines_count;
	unsigned int  cachelines_size;
};


/* Dynamic configuration */

pcm_storeset_list_t pcm_storeset_list = { 0, 
                                          LIST_HEAD_INIT(pcm_storeset_list.list), 
                                          0, 
                                          PTHREAD_MUTEX_INITIALIZER, 
                                          PTHREAD_COND_INITIALIZER, 
                                          PTHREAD_COND_INITIALIZER, 
                                          0 };


/* CDF for a cacheline partial crash. */

#define NO_PARTIAL_CRASH {{0,0,0,0,0,0,0,0,1000000}}

cacheline_crash_cdf_t cacheline_crash_cdf = NO_PARTIAL_CRASH;


/* Likelihood a store will block wait. */
unsigned int pcm_likelihood_store_blockwaits = 1000;  

/* Likelihood a cacheline has been evicted. */
unsigned int pcm_likelihood_evicted_cacheline = 10000;  

volatile arch_spinlock_t ticket_lock = {0};


__thread pcm_storeset_t* _thread_pcm_storeset;


static inline
cacheline_t *
cacheline_alloc()
{
	cacheline_t *l;

	l = (cacheline_t*) malloc(sizeof(cacheline_t));
	PM_MEMSET(l, 0x0, sizeof(cacheline_t));
	return l;
}

static inline
void
cacheline_free(cacheline_t *l)
{
	free(l);
}


int
pcm_storeset_create(pcm_storeset_t **setp)
{
	pcm_storeset_t *set;

	set = (pcm_storeset_t *) malloc(sizeof(pcm_storeset_t));
	pthread_mutex_lock(&pcm_storeset_list.lock);
	pcm_storeset_list.count++;
	list_add(&set->list, &pcm_storeset_list.list);
	pthread_mutex_unlock(&pcm_storeset_list.lock);
	set->hashtbl = PointerHash_new();
	PM_MEMSET(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
	set->wcbuf_hashtbl_count = 0;
	set->seqstream_len = 0;
	set->in_crash_emulation_code = 0;
	/* Initialize reentrant random generator */
	set->rand_seed = pthread_self();
	rand_int(&set->rand_seed);
	*setp = set;
	return 0;
}


void
pcm_storeset_destroy(pcm_storeset_t *set)
{
	pthread_mutex_lock(&pcm_storeset_list.lock);
	pcm_storeset_list.count--;
	list_del(&set->list);
	pthread_mutex_unlock(&pcm_storeset_list.lock);
	PointerHash_free(set->hashtbl);
	free(set);
}


pcm_storeset_t*
pcm_storeset_get(void)
{
	pcm_storeset_t *set = _thread_pcm_storeset;

	if (set) {
		return set;
	}

	pcm_storeset_create(&set);
	_thread_pcm_storeset = set;

	return set;
}


void
pcm_storeset_put(void)
{
	pcm_storeset_t *set = _thread_pcm_storeset;

	if (set) {
		pcm_storeset_destroy(set);
		_thread_pcm_storeset = NULL;
	}
}



void 
pcm_check_crash(pcm_storeset_t *set)
{
	int was_in_crash_emulation_code;

	if (pcm_storeset_list.outstanding_crash) {
		was_in_crash_emulation_code = set->in_crash_emulation_code;
		set->in_crash_emulation_code = 0;
		pthread_mutex_lock(&pcm_storeset_list.lock);
		if (pcm_storeset_list.outstanding_crash) {
			pcm_storeset_list.waiters++;
			pthread_cond_signal(&pcm_storeset_list.cond_wait_storesets_halt);
			while(pcm_storeset_list.outstanding_crash) {
				pthread_cond_wait(&pcm_storeset_list.cond_halt, &pcm_storeset_list.lock);
			}
		}	
		pthread_mutex_unlock(&pcm_storeset_list.lock);
		set->in_crash_emulation_code = was_in_crash_emulation_code;
	}
}


static inline
void
crash_save_oldvalue(pcm_storeset_t *set, volatile pcm_word_t *addr)
{
	uintptr_t           byte_addr;
	uintptr_t           block_byte_addr;
	uintptr_t           index_byte_addr;
	int                 i;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;
	uint8_t             byte_oldvalue;

	assert(set->in_crash_emulation_code);

	for (i=0; i<sizeof(pcm_word_t); i++) {
		byte_addr = (uintptr_t) addr + i;
		block_byte_addr = (uintptr_t) BLOCK_ADDR(byte_addr);
		index_byte_addr = (uintptr_t) INDEX_ADDR(byte_addr);
		cacheline = (cacheline_t *) PointerHash_at_(set->hashtbl, (void *) block_byte_addr);
		if (!cacheline) {
			cacheline = cacheline_alloc();
			PointerHash_at_put_(set->hashtbl, (void *) block_byte_addr, (void *) cacheline);
		}
		bitmask = 1ULL << index_byte_addr;
		if (!(cacheline->bitmask & bitmask)) {
			byte_oldvalue = *((uint8_t *) byte_addr);
			cacheline->bytes[index_byte_addr] = byte_oldvalue;
			cacheline->bitmask |= bitmask;
		}
	}

}


static inline
void
crash_restore_unflushed_values(pcm_storeset_t *set)
{
	uintptr_t           byte_addr;
	uintptr_t           block_byte_addr;
	int                 i;
	int                 j;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;

	assert(set->in_crash_emulation_code);

	for(i = 0; i < set->hashtbl->size; i++) {
		PointerHashRecord *r = PointerHashRecords_recordAt_(set->hashtbl->records, i);
		if ((block_byte_addr = (uintptr_t) r->k)) {
			cacheline = (cacheline_t *) r->v;
			for (j=0; j<CACHELINE_SIZE; j++) {
				byte_addr = block_byte_addr + j;
				bitmask = 1ULL << j;
				if (cacheline->bitmask & bitmask) {
					*((uint8_t*)byte_addr) = cacheline->bytes[j];
				}
			}
		}
	}
}


/*
 * Flush a cacheline by removing the line from the list of outstanding stores.
 */
void
crash_flush_cacheline(pcm_storeset_t *set, volatile pcm_word_t *addr, int allow_partial_crash)
{
	uintptr_t           byte_addr;
	uintptr_t           block_byte_addr;
	int                 random_number;
	int                 i;
	int                 sum;
	int                 successfully_flushed_words_num=0;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;

	assert(set->in_crash_emulation_code);

	byte_addr = (uintptr_t) addr;
	block_byte_addr = (uintptr_t) BLOCK_ADDR(addr);

flush_cacheline:
	cacheline = (cacheline_t *) PointerHash_at_(set->hashtbl, (void *) block_byte_addr);
	if (!cacheline) {
		/* Cache line already flushed. */
		return;
	}
	
	/* If there is an outstanding failure then the currently flushed line
	 * can be treaded as not flushed, partially flushed, or completely
	 * flushed based on some distribution.
	 * Remove from the outstanding stores just the part of the cache line
	 * that has been successfully flushed.
	 */
	if (pcm_storeset_list.outstanding_crash && allow_partial_crash) {
		random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
		for (i=0, sum=0; i<CACHELINE_SIZE/sizeof(pcm_word_t)+1; i++) {
			sum += cacheline_crash_cdf.word[i];
			if (random_number <= sum) {
				successfully_flushed_words_num = i;
				break;
			}
		}
	} else {
		successfully_flushed_words_num = CACHELINE_SIZE/sizeof(pcm_word_t);
	}	
	if (successfully_flushed_words_num == CACHELINE_SIZE/sizeof(pcm_word_t)) {
		bitmask = 0x0;
	} else {
		//FIXME: what is the correct bitmask here?
		//bitmask = ((~(1ULL << CACHELINE_SIZE - 1))
		//           << (successfully_flushed_words_num * sizeof(pcm_word_t))) & (~(1ULL << CACHELINE_SIZE - 1));
		bitmask = (((uint64_t) -1)
		           << (successfully_flushed_words_num * sizeof(pcm_word_t))) & ((uint64_t) -1);
	}	
	cacheline->bitmask = cacheline->bitmask & bitmask;

	if (cacheline->bitmask == 0x0) {
		cacheline_free(cacheline);
		PointerHash_removeKey_(set->hashtbl, (void *) block_byte_addr);
	}

	/* The word could fall in two cachelines. */
	byte_addr += sizeof(pcm_word_t)-1;
	block_byte_addr += sizeof(pcm_word_t)-1;
	goto flush_cacheline;

}


/*
 * Either flush all cachelines, or randomly flush some based on a given
 * probability distribution.
 */
static inline
void
crash_flush_cachelines(pcm_storeset_t *set, int all, int likelihood_flush_cacheline)
{
	int                 i;
	int                 count;
	int                 random_number;
	int                 do_flush;
	PointerHashRecord   *ra[1024];

	assert(set->in_crash_emulation_code);

	for(i = 0, count=0, do_flush=0; i < set->hashtbl->size; i++) {
		PointerHashRecord *r = PointerHashRecords_recordAt_(set->hashtbl->records, i);
		if (r->k) {
			if (!all) {
				random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
				if (random_number < likelihood_flush_cacheline) {
					do_flush = 1;
				} else {
					do_flush = 0;
				}
			}
			if (all || (!all && do_flush)) {
				assert(count < 1024);
				ra[count++] = r;
			}	
		}
	}

	for(i = 0; i < count; i++) {
		crash_flush_cacheline(set, (pcm_word_t *) ra[i]->k, 0);
	}	
}



void 
pcm_trigger_crash(pcm_storeset_t *set, int wait_storesets_halt)
{
	int             waiters;
	pcm_storeset_t  *set_iter;


	pthread_mutex_lock(&pcm_storeset_list.lock);
	if (pcm_storeset_list.outstanding_crash) {
		/* Someone else has already triggered a crash */
		pthread_mutex_unlock(&pcm_storeset_list.lock);
		pcm_check_crash(set);
		return;
	}
	pcm_storeset_list.outstanding_crash = 1;
	pcm_storeset_list.waiters = 0;

	if (wait_storesets_halt) {
		/* Wait for all storesets to halt. */
		waiters = pcm_storeset_list.count; 
		if (set) {
			/* Called in the context of a storeset, so make sure I don't
			 * wait for myself */
			waiters--;
		}	
		while(pcm_storeset_list.waiters < waiters) {
			pthread_cond_wait(&pcm_storeset_list.cond_wait_storesets_halt, &pcm_storeset_list.lock);
		}	
	} {
		/* If I don't wait for store sets to halt, at least make sure
		 * that there is no concurrent action on a store set.
		 */
		list_for_each_entry(set_iter, &pcm_storeset_list.list, list)		 
		{
			while (set_iter->in_crash_emulation_code);
		}
		 
	}

	/* We are at a safe point; either all store sets are halted or 
	 * there is no concurrent activity on any store set. Proceed with crash. 
	 * To emulate evictions that might had happened, first randomly flush some 
	 * cache lines based on a probability distribution, and then restore
	 * old values of any outstanding stores.
	 */
	list_for_each_entry(set_iter, &pcm_storeset_list.list, list)		 
	{
		crash_flush_cachelines(set, 0, pcm_likelihood_evicted_cacheline);
		crash_restore_unflushed_values(set_iter);
	}
	
	/* Done with crash. Release anyone that has been halted. */
	pcm_storeset_list.outstanding_crash = 0;
	pthread_cond_broadcast(&pcm_storeset_list.cond_halt);
	pthread_mutex_unlock(&pcm_storeset_list.lock);
}


/* 
 * We emulate the write-combining buffers using a hash table that never contains 
 * more than WRITE_COMBINING_BUFFERS_NUM keys.
 */
static inline
void
nt_flush_buffers(pcm_storeset_t *set)
{
	int                 i;
	int                 count;
	PointerHashRecord   *ra[WRITE_COMBINING_BUFFERS_NUM];

	assert(set->in_crash_emulation_code);

	for(i = 0, count=0; i < set->hashtbl->size; i++) {
		PointerHashRecord *r = PointerHashRecords_recordAt_(set->hashtbl->records, i);
		if (r->k) {
			assert(count < WRITE_COMBINING_BUFFERS_NUM);
			ra[count++] = r;
		}
	}

	for(i = 0; i < count; i++) {
		crash_flush_cacheline(set, (pcm_word_t *) ra[i]->k, 1);
	}	
}


void
pcm_wb_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	if (!set) {
		return;
	}
	pcm_check_crash(set);
	set->in_crash_emulation_code = 1;
	crash_save_oldvalue(set, addr);
	set->in_crash_emulation_code = 0;
}


void
pcm_wb_flush_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr)
{
	set->in_crash_emulation_code = 1;
	crash_flush_cacheline(set, addr, 1);
	set->in_crash_emulation_code = 0;
	pcm_check_crash(set);
}


void
pcm_nt_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	unsigned int active_buffers_count;
	uintptr_t    byte_addr;
	uintptr_t    block_byte_addr1;
	uintptr_t    block_byte_addr2;
	cacheline_t  *cacheline;
	int          buffers_needed = 0;

	pcm_check_crash(set);
	set->in_crash_emulation_code = 1;
	byte_addr = (uintptr_t) addr;

	/* Check if a buffer already holds the cacheline containing addr.
	 * If not then find out how many empty buffers we need. 
	 */
	block_byte_addr1 = (uintptr_t) BLOCK_ADDR(byte_addr);
	block_byte_addr2 = (uintptr_t) BLOCK_ADDR(byte_addr+sizeof(pcm_word_t)-1);
	if (!(cacheline = PointerHash_at_(set->hashtbl, (void *) block_byte_addr1))) {
		buffers_needed++;
	}
	if (block_byte_addr1 != block_byte_addr2 /* Word spans more than one cacheline. */
		&& (!(cacheline = PointerHash_at_(set->hashtbl, (void *) block_byte_addr2)))) 
	{
		buffers_needed++;
	}

	/* Check if there is space. If not then flush buffers */
	active_buffers_count = PointerHash_count(set->hashtbl);
	if (active_buffers_count + buffers_needed > WRITE_COMBINING_BUFFERS_NUM) {
		nt_flush_buffers(set);
	}
	crash_save_oldvalue(set, addr);
	set->in_crash_emulation_code = 0;
}


void
pcm_nt_flush_emulate_crash(pcm_storeset_t *set)
{
	pcm_check_crash(set);
	set->in_crash_emulation_code = 1;
	nt_flush_buffers(set);
	set->in_crash_emulation_code = 0;
}
