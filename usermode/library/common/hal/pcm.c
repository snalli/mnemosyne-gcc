#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <mmintrin.h>
#include <cuckoo_hash/PointerHashInline.h>
#include "pcm.h"

/* Static configuration */

/* Emulate crashes */
//#define EMULATE_CRASH 0x1
#undef EMULATE_CRASH

/* Emulate latency */
//#define EMULATE_LATENCY 0x1
#undef EMULATE_LATENCY

/* CPU frequency */
#define CPUFREQ 2000LLU /* GHz */

/* PCM write latency*/
#define LATENCY_PCM_WRITE 0 /* ns */

/* 
 * Stores may block wait to find space in the cache (write buffer is full and 
 * must wait to evict other cacheline from the cache) or to find an empty 
 * write-combining buffer.
 * 
 * For write-back, we emulate by using some probability to block-wait.
 * For write-combining, we keep track of the number of WC buffers being 
 * used. We conservatively assume that no implicit evictions happen.
 */
#define EMULATE_LATENCY_BLOCKING_STORES 0x1


/* Machine has the RDTSCP instruction. 
 * 
 * RDTSCP can be handy when measuring short intervals because it doesn't need
 * to serialize the processor first. Thus, it can be used to measure the actual 
 * latency of instructions such as CLFLUSH. This allows us to add an extra latency
 * to meet the desirable emulated latency instead of adding a fixed latency.
 */
//#define HAS_RDTSCP
#undef HAS_RDTSCP

/* The number of available write-combining buffers. */
#define WRITE_COMBINING_BUFFERS_NUM 8

/* The size of the WC-buffer hash table. Must be a power of 2. */
#define WCBUF_HASHTBL_SIZE WRITE_COMBINING_BUFFERS_NUM*4

/* 
 * Probabilities are derived using total number of outcomes equal to 
 * TOTAL_OUTCOMES_NUM
 */
#define TOTAL_OUTCOMES_NUM 1000000

#if (RAND_MAX < TOTAL_OUTCOMES_NUM)
# error "RAND_MAX must be at least equal to PROB_TOTAL_OUTCOMES_NUM."
#endif




/* Definitions */

#define NS2CYCLE(__ns) ((__ns) * CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / CPUFREQ)


#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)


/* Types */

typedef uint64_t cacheline_bitmask_t;

typedef struct pcm_storeset_list_s pcm_storeset_list_t; 

struct pcm_storeset_list_s {
	uint32_t          count;
	pcm_storeset_t    *head;
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


typedef struct cacheline_tbl_s cacheline_tbl_t;

struct cacheline_tbl_s {
	cacheline_t   *cachelines;
	unsigned int  cachelines_count;
	unsigned int  cachelines_size;
};

struct pcm_storeset_s {
	uint32_t              id;
	uint32_t              state;
	unsigned int          rand_seed;
	PointerHash           *hashtbl;
	uint16_t              wcbuf_hashtbl[WCBUF_HASHTBL_SIZE];
	uint16_t              wcbuf_hashtbl_count;
	cacheline_tbl_t       *cacheline_tbl;
	pcm_storeset_t        *next;
	volatile unsigned int in_crash_emulation_code;
};


/* Dynamic configuration */

pcm_storeset_list_t pcm_storeset_list = { 0, 
                                          NULL, 
                                          0, 
                                          PTHREAD_MUTEX_INITIALIZER, 
                                          PTHREAD_COND_INITIALIZER, 
                                          PTHREAD_COND_INITIALIZER, 
                                          0 };


/* CDF for a cacheline partial crash. */

#define NO_PARTIAL_CRASH {{0,0,0,0,0,0,0,0,1000000}}

cacheline_crash_cdf_t cacheline_crash_cdf = NO_PARTIAL_CRASH;


/* Likelihood a store will block wait. */

unsigned int likelihood_store_blockwaits = 1000;  

/* Likelihood a cacheline has been evicted. */

unsigned int likelihood_evicted_cacheline = 10000;  


typedef uint64_t hrtime_t;

static inline void asm_cpuid() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}


#if defined(__i386__)

static inline unsigned long long asm_rdtsc(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

static inline unsigned long long asm_rdtscp(void)
{
		unsigned hi, lo;
	__asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi)::"ecx");
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );

}
#elif defined(__x86_64__)

static inline unsigned long long asm_rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static inline unsigned long long asm_rdtscp(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi)::"rcx");
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#else
#error "What architecture is this???"
#endif


static inline void asm_movnti(pcm_word_t *addr, pcm_word_t val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}


static inline void asm_clflush(pcm_word_t *addr)
{
	__asm__ __volatile__ ("clflush %0" : : "m"(*addr));
}


static inline void asm_mfence(void)
{
	__asm__ __volatile__ ("mfence");
}


static inline void asm_sfence(void)
{
	__asm__ __volatile__ ("sfence");
}


static inline
int rand_int(unsigned int *seed)
{
    *seed=*seed*196314165+907633515;
    return *seed;
}

static inline
cacheline_t *
cacheline_alloc()
{
	cacheline_t *l;

	l = (cacheline_t*) malloc(sizeof(cacheline_t));
	memset(l, 0x0, sizeof(cacheline_t));
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
	set->next = pcm_storeset_list.head;
	pcm_storeset_list.head = set;
	pthread_mutex_unlock(&pcm_storeset_list.lock);
	set->hashtbl = PointerHash_new();
	memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
	set->wcbuf_hashtbl_count = 0;
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
	assert(0 && "Not yet implemented");
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
crash_save_oldvalue(pcm_storeset_t *set, pcm_word_t *addr)
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
	uintptr_t           index_byte_addr;
	int                 i;
	int                 j;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;
	uint8_t             byte_oldvalue;

	assert(set->in_crash_emulation_code);

	for(i = 0; i < set->hashtbl->size; i++) {
		PointerHashRecord *r = PointerHashRecords_recordAt_(set->hashtbl->records, i);
		if (block_byte_addr = (uintptr_t) r->k) {
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
crash_flush_cacheline(pcm_storeset_t *set, pcm_word_t *addr, int allow_partial_crash)
{
	uintptr_t           byte_addr;
	uintptr_t           block_byte_addr;
	uintptr_t           index_byte_addr;
	int                 random_number;
	int                 i;
	int                 sum;
	int                 successfully_flushed_words_num;
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
		bitmask = ((~(1ULL << CACHELINE_SIZE)) << (successfully_flushed_words_num * sizeof(pcm_word_t))) & (~(1ULL << CACHELINE_SIZE));
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
	pcm_word_t      *addr;
	int             i;
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
		for (set_iter = pcm_storeset_list.head;
		     set_iter != NULL;
		     set_iter = set_iter->next)
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
	for (set_iter = pcm_storeset_list.head;
	     set_iter != NULL;
	     set_iter = set_iter->next)
	{
		crash_flush_cachelines(set, 0, likelihood_evicted_cacheline);
		crash_restore_unflushed_values(set_iter);
	}
	
	/* Done with crash. Release anyone that has been halted. */
	pcm_storeset_list.outstanding_crash = 0;
	pthread_cond_broadcast(&pcm_storeset_list.cond_halt);
	pthread_mutex_unlock(&pcm_storeset_list.lock);
}


/*
 * WRITE BACK CACHE MODE
 */

static inline
void
emulate_latency_ns(int ns)
{
	hrtime_t cycles;
	hrtime_t start;
	hrtime_t stop;
	
	start = asm_rdtsc();
	cycles = NS2CYCLE(ns);

	do { 
		/* RDTSC doesn't necessarily wait for previous instructions to complete 
		 * so a serializing instruction is usually used to ensure previous 
		 * instructions have completed. However, in our case this is a desirable
		 * property since we want to overlap the latency we emulate with the
		 * actual latency of the emulated instruction. 
		 */
		stop = asm_rdtsc();
	} while (stop - start < cycles);
}


/*!
 * Emulates the latency of a write to PCM. In reality, this method simply stores to an address and
 * spins to emulate a nonzero latency.
 *
 * \param is something that Haris needs to explain.
 * \param addr is the address being written.
 * \param val is the word written at address.
 */
void
pcm_wb_store(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{

#ifdef EMULATE_CRASH
	pcm_check_crash(set);
	set->in_crash_emulation_code = 1;
	crash_save_oldvalue(set, addr);
	set->in_crash_emulation_code = 0;
#endif	

	// This is a fake store to PCM. For now, we just store the data at the given address in DRAM.
	// This will change when we have a real 
	*addr = val;
	//printf("STORE: (0x%lx, %lx)\n", addr, val);

#ifdef EMULATE_LATENCY
# ifdef EMULATE_LATENCY_BLOCKING_STORES
	if (likelihood_store_blockwaits > 0) {
		int random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
		if (random_number < likelihood_store_blockwaits) {
			emulate_latency_ns(LATENCY_PCM_WRITE);
		}
	}
# endif
#endif
}


/*
 * Flush the cacheline containing address addr.
 */
void
pcm_wb_flush(pcm_storeset_t *set, pcm_word_t *addr)
{
	uintptr_t           byte_addr;
	uintptr_t           block_byte_addr;
	uintptr_t           index_byte_addr;
	int                 random_number;
	int                 i;
	int                 sum;
	int                 successfully_flushed_words_num;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;

#ifdef EMULATE_CRASH
	set->in_crash_emulation_code = 1;
	crash_flush_cacheline(set, addr, 1);
	set->in_crash_emulation_code = 0;
	pcm_check_crash(set);
#endif

	//printf("FLUSH: (0x%lx)\n", addr);
#ifdef EMULATE_LATENCY
	{
		/* FIXME: We don't model bandwidth.
		 * Delaying each flush by a fixed latency is very conservative.
		 * What if we have a series of cacheline flushes that fall to different
		 * banks? In this case you can have multiple outstanding flushes which
		 * are limited by the available bandwidth. */

		/* Measure the latency of a clflush and add an additional delay to
		 * meet the latency to write to PCM */
		hrtime_t cycles;
		hrtime_t start;
		hrtime_t stop;

#ifdef HAS_RDTSCP
		start = asm_rdtscp();
		asm_clflush(addr); 	
		stop = asm_rdtscp();
		emulate_latency_ns(LATENCY_PCM_WRITE - CYCLE2NS(stop-start));
#else
		asm_clflush(addr); 	
		emulate_latency_ns(LATENCY_PCM_WRITE);
#endif		
		asm_mfence();
	}	

#else /* !EMULATE_LATENCY */ 
	asm_clflush(addr); 	
	asm_mfence();
#endif /* !EMULATE_LATENCY */ 

}


/*
 * NON-TEMPORAL STREAM MODE
 *
 */

/* 
 * We emulate the write-combining buffers using a hash table that never contains 
 * more than WRITE_COMBINING_BUFFERS_NUM keys.
 */

static inline
void
stream_flush_buffers(pcm_storeset_t *set)
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
pcm_stream_store(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	unsigned int active_buffers_count;
	uintptr_t    byte_addr;
	uintptr_t    block_byte_addr;
	uintptr_t    block_byte_addr1;
	uintptr_t    block_byte_addr2;
	cacheline_t  *cacheline;
	int          buffers_needed = 0;

#ifdef EMULATE_CRASH
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
		stream_flush_buffers(set);
	}
	crash_save_oldvalue(set, addr);
	set->in_crash_emulation_code = 0;
#endif	

	asm_movnti(addr, val);

#ifdef EMULATE_LATENCY
	uint16_t i;
	uint16_t index_addr;
	uint16_t index;
	uint16_t index_i;
	
	byte_addr = (uintptr_t) addr;
	block_byte_addr = (uintptr_t) BLOCK_ADDR(byte_addr);
	index_addr = (uint16_t) ((block_byte_addr >> CACHELINE_SIZE_LOG)  & ((uint16_t) (-1)));

retry:
	if (set->wcbuf_hashtbl_count < WRITE_COMBINING_BUFFERS_NUM) {
		for (i=0; i<WCBUF_HASHTBL_SIZE; i++) {
			index_i = (index_addr + i) &  (WCBUF_HASHTBL_SIZE-1);
			if (set->wcbuf_hashtbl[index_i] == index_addr) {
				/* hit -- do nothing */
				break;
			} else if (set->wcbuf_hashtbl[index_i] == 0) {
				set->wcbuf_hashtbl[index_i] = index_addr;
				set->wcbuf_hashtbl_count++;
				break;
			}	
		}
	} else {
		memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
		emulate_latency_ns(LATENCY_PCM_WRITE * set->wcbuf_hashtbl_count);
		set->wcbuf_hashtbl_count = 0;
		goto retry;
	}

#endif
}


void
pcm_stream_flush(pcm_storeset_t *set)
{
#ifdef EMULATE_CRASH
	pcm_check_crash(set);
	set->in_crash_emulation_code = 1;
	stream_flush_buffers(set);
	set->in_crash_emulation_code = 0;
#endif	

	asm_sfence();
#ifdef EMULATE_LATENCY
	emulate_latency_ns(LATENCY_PCM_WRITE * set->wcbuf_hashtbl_count);
	memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
	set->wcbuf_hashtbl_count = 0;
#endif
}
