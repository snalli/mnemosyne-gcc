/**
 * \file
 *
 * PCM emulation over DRAM.
 *
 */
#ifndef _PCM_INTERNAL_H
#define _PCM_INTERNAL_H

#include <stdint.h>
#include <mmintrin.h>
#include <list.h>
#include "cuckoo_hash/PointerHashInline.h"


#ifdef __cplusplus
extern "C" {
#endif


/** 
 * Stores may block wait to find space in the cache (write buffer is full and 
 * must wait to evict other cacheline from the cache) or to find an empty 
 * write-combining buffer.
 * 
 * For write-back, we emulate by using some probability to block-wait.
 * For write-combining, we keep track of the number of WC buffers being 
 * used. We conservatively assume that no implicit evictions happen.
 */
//#define M_PCM_EMULATE_LATENCY_BLOCKING_STORES 0x1
#undef M_PCM_EMULATE_LATENCY_BLOCKING_STORES


/** 
 * Machine has the RDTSCP instruction. 
 * 
 * RDTSCP can be handy when measuring short intervals because it doesn't need
 * to serialize the processor first. Thus, it can be used to measure the actual 
 * latency of instructions such as CLFLUSH. This allows us to add an extra latency
 * to meet the desirable emulated latency instead of adding a fixed latency.
 */
//#define HAS_RDTSCP
#undef HAS_RDTSCP

/** The number of available write-combining buffers. */
#define WRITE_COMBINING_BUFFERS_NUM 8

/** The size of the WC-buffer hash table. Must be a power of 2. */
#define WCBUF_HASHTBL_SIZE WRITE_COMBINING_BUFFERS_NUM*4

/** The memory banking factor. Must be a power of 2. */
#define MEMORY_BANKING_FACTOR 8


/* 
 * Probabilities are derived using total number of outcomes equal to 
 * TOTAL_OUTCOMES_NUMTENCY_PCM_WRITE
 */
#define TOTAL_OUTCOMES_NUM 1000000

#if (RAND_MAX < TOTAL_OUTCOMES_NUM)
# error "RAND_MAX must be at least equal to PROB_TOTAL_OUTCOMES_NUM."
#endif


#define NS2CYCLE(__ns) ((__ns) * M_PCM_CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / M_PCM_CPUFREQ)


#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)


/* Memory Pages */

#define PAGE_SIZE 4096

/* Returns the number of pages */
#define NUM_PAGES(size) ((((size) % PAGE_SIZE) == 0? 0 : 1) + (size)/PAGE_SIZE)

/* Returns the size at page granularity */
#define SIZEOF_PAGES(size) (NUM_PAGES((size)) * PAGE_SIZE)

/* Returns the size at page granularity */
#define PAGE_ALIGN(addr) (NUM_PAGES((addr)) * PAGE_SIZE)


/* Hardware Cache */

#ifdef __x86_64__
# define CACHELINE_SIZE     64
# define CACHELINE_SIZE_LOG 6
#else
# define CACHELINE_SIZE     32
# define CACHELINE_SIZE_LOG 5
#endif

#define BLOCK_ADDR(addr) ( (pcm_word_t *) (((pcm_word_t) (addr)) & ~(CACHELINE_SIZE - 1)) )
#define INDEX_ADDR(addr) ( (pcm_word_t *) (((pcm_word_t) (addr)) & (CACHELINE_SIZE - 1)) )


/* Public types */

typedef uintptr_t pcm_word_t;

typedef uint64_t pcm_hrtime_t;

typedef struct pcm_storeset_s pcm_storeset_t;
typedef struct cacheline_tbl_s cacheline_tbl_t;



/** 
 * Per client bookkeeping data structure that keeps several information
 * necessary to emulate latency and crashes for PCM on top of DRAM.
 */
struct pcm_storeset_s {
	uint32_t              id;
	uint32_t              state;
	unsigned int          rand_seed;
	PointerHash           *hashtbl;
	uint16_t              wcbuf_hashtbl[WCBUF_HASHTBL_SIZE];
	uint16_t              wcbuf_hashtbl_count;
	uint32_t              seqstream_len;
	cacheline_tbl_t       *cacheline_tbl;
	struct list_head      list;
	volatile unsigned int in_crash_emulation_code;
};

/*
 * Externally defined global variables.
 */ 

extern unsigned int pcm_likelihood_store_blockwaits;  

/* 
 * Prototypes
 */

int pcm_storeset_create(pcm_storeset_t **setp);
void pcm_storeset_destroy(pcm_storeset_t *set);
pcm_storeset_t* pcm_storeset_get(void);
void pcm_storeset_put(void);
void pcm_wb_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val);
void pcm_wb_flush_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr);
void pcm_nt_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val);
void pcm_nt_flush_emulate_crash(pcm_storeset_t *set);


/*
 * Helper functions.
 */

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


static inline void asm_movnti(volatile pcm_word_t *addr, pcm_word_t val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}


static inline void asm_clflush(volatile pcm_word_t *addr)
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


# ifdef _EMULATE_LATENCY_USING_NOPS
/* So you think nops are more accurate? you might be surprised */
static inline void asm_nop10() {
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
}

static inline
void
emulate_latency_ns(int ns)
{
	int          i;
	pcm_hrtime_t cycles;
	pcm_hrtime_t start;
	pcm_hrtime_t stop;
	
	cycles = NS2CYCLE(ns);
	for (i=0; i<cycles; i+=5) {
		asm_nop10(); /* each nop is 1 cycle */
	}
}

# else

static inline
void
emulate_latency_ns(int ns)
{
	pcm_hrtime_t cycles;
	pcm_hrtime_t start;
	pcm_hrtime_t stop;
	
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

# endif

/**
 * \brief Writes a masked word.
 * 
 * x86 memory model guarantees that single-byte stores are atomic, and
 * also that word-aligned word-size stores are atomic. If we wrote a
 * word-aligned word by reading its old value, masking-in the new value
 * and writing out the result, it would be not be always correct. To see
 * why consider another thread that writes a part of the same word but
 * its mask does not overlap with any other conrcurrent masked store.
 * Normally there is no race because we touch different parts. However
 * because we do word-size writes we introduce non-existing races. We
 * resolve this problem by writing each byte individually in the case
 * when we don't write the whole word. 
 * 
 * Note: Mask MUST not be zero.
 */
static inline 
void
write_aligned_masked(pcm_word_t *addr, pcm_word_t val, pcm_word_t mask)
{
	uintptr_t a;
	int       i;
	int       trailing_0bytes;
	int       leading_0bytes;

	union convert_u {
		pcm_word_t w;
		uint8_t    b[sizeof(pcm_word_t)];
	} valu;

	/* Complete write? */
	if (mask == ((uint64_t) -1)) {
		*addr = val;
	} else {
		valu.w = val;
		a = (uintptr_t) addr;
		trailing_0bytes = __builtin_ctzll(mask) >> 3;
		leading_0bytes = __builtin_clzll(mask) >> 3;
		for (i = trailing_0bytes; i<8-leading_0bytes;i++) {
			*((uint8_t *) (a+i)) = valu.b[i];
		}
	}
}


/*
 * WRITE BACK CACHE MODE
 */


/**
 * Emulates the latency of a write to PCM. In reality, this method simply stores to an address and
 * spins to emulate a nonzero latency.
 *
 * \param set is the client's pcm storeset where outstanding stores are kept.
 * \param addr is the address being written.
 * \param val is the word written at address.
 */
static inline
void
PCM_WB_STORE(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	//printf("STORE: (0x%lx, %lx)\n", addr, val);
#ifdef M_PCM_EMULATE_CRASH
	pcm_wb_store_emulate_crash(set, addr, val);
#endif	

	*addr = val;

#ifdef M_PCM_EMULATE_LATENCY
# ifdef M_PCM_EMULATE_LATENCY_BLOCKING_STORES
	if (pcm_likelihood_store_blockwaits > 0) {
		int random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
		if (random_number < pcm_likelihood_store_blockwaits) {
			emulate_latency_ns(M_PCM_LATENCY_WRITE);
		}
	}
# endif
#endif
}


static inline
void
PCM_WB_STORE_MASKED(pcm_storeset_t *set, 
                    volatile pcm_word_t *addr, 
                    pcm_word_t val, 
                    pcm_word_t mask)
{
	//printf("WB_STORE_MASKED: (0x%lx, %lx, %lx)\n", addr, val, mask);
#ifdef M_PCM_EMULATE_CRASH
	pcm_wb_store_emulate_crash(set, addr, val);
#endif	

	write_aligned_masked((pcm_word_t *) addr, val, mask);

#ifdef M_PCM_EMULATE_LATENCY
# ifdef M_PCM_EMULATE_LATENCY_BLOCKING_STORES
	if (pcm_likelihood_store_blockwaits > 0) {
		int random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
		if (random_number < pcm_likelihood_store_blockwaits) {
			emulate_latency_ns(M_PCM_LATENCY_WRITE);
		}
	}
# endif
#endif
}


static inline
void
PCM_WB_STORE_ALIGNED_MASKED(pcm_storeset_t *set, 
                            volatile pcm_word_t *addr, 
                            pcm_word_t val, 
                            pcm_word_t mask)
{
	PCM_WB_STORE_MASKED(set, addr, val, mask);
}


/*
 * Flush the cacheline containing address addr.
 */
static inline
void
PCM_WB_FLUSH(pcm_storeset_t *set, volatile pcm_word_t *addr)
{
	//printf("FLUSH: (0x%lx)\n", addr);
#ifdef M_PCM_EMULATE_CRASH
	pcm_wb_flush_emulate_crash(set, addr);
#endif

#ifdef M_PCM_EMULATE_LATENCY
	{
#ifdef HAS_RDTSCP
		/* Measure the latency of a clflush and add an additional delay to
		 * meet the latency to write to PCM */
		pcm_hrtime_t start;
		pcm_hrtime_t stop;

		start = asm_rdtscp();
		asm_clflush(addr); 	
		stop = asm_rdtscp();
		emulate_latency_ns(M_PCM_LATENCY_WRITE - CYCLE2NS(stop-start));
#else
		asm_clflush(addr); 	
		emulate_latency_ns(M_PCM_LATENCY_WRITE);
#endif		
		asm_mfence();
	}	

#else /* !M_PCM_EMULATE_LATENCY */ 
	asm_clflush(addr); 	
	asm_mfence();
#endif /* !M_PCM_EMULATE_LATENCY */ 

}


/*
 * NON-TEMPORAL STREAM MODE
 *
 * Stores are non-cacheable but go through the write-combining buffers instead.
 */

static inline
void
PCM_NT_STORE(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
#ifdef M_PCM_EMULATE_CRASH
	pcm_nt_store_emulate_crash(set, addr, val);
#endif	

	asm_movnti(addr, val);

#ifdef M_PCM_EMULATE_LATENCY
	uint16_t  i;
	uint16_t  index_addr;
	uint16_t  index_i;
	uintptr_t byte_addr;
	uintptr_t block_byte_addr;
	
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
		emulate_latency_ns(M_PCM_LATENCY_WRITE * set->wcbuf_hashtbl_count);
		set->wcbuf_hashtbl_count = 0;
		goto retry;
	}

#endif
}


static inline
void
PCM_NT_FLUSH(pcm_storeset_t *set)
{
#ifdef M_PCM_EMULATE_CRASH
	pcm_nt_flush_emulate_crash(set);
#endif	

	asm_sfence();
#ifdef M_PCM_EMULATE_LATENCY
	emulate_latency_ns(M_PCM_LATENCY_WRITE * set->wcbuf_hashtbl_count);
	memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
	set->wcbuf_hashtbl_count = 0;
#endif
}


/*
 * NON-TEMPORAL SEQUENTIAL STREAM MODE
 *
 * Used when we know that stream accesses are sequential so that we 
 * emulate bandwidth and hide some latency. For example, stores to the log
 * are sequential. 
 *
 */

 
static inline
void
PCM_SEQSTREAM_INIT(pcm_storeset_t *set)
{
#ifdef M_PCM_EMULATE_CRASH

#endif	

#ifdef M_PCM_EMULATE_LATENCY
	set->seqstream_len = 0;
#endif
}


static inline
void
PCM_SEQSTREAM_STORE(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	//printf("PCM_SEQSTREAM_STORE: set=%p, addr=%p, val=%llX\n", set, addr, val);
#ifdef M_PCM_EMULATE_CRASH
#endif	

	asm_movnti(addr, val);

#ifdef M_PCM_EMULATE_LATENCY
	set->seqstream_len = (set->seqstream_len + 1) & ((MEMORY_BANKING_FACTOR * 
	                                                  CACHELINE_SIZE / sizeof(pcm_word_t)) - 1);
	if (set->seqstream_len == 0) {
		emulate_latency_ns(M_PCM_LATENCY_WRITE);
	}
#endif
}


static inline
void
PCM_SEQSTREAM_FLUSH(pcm_storeset_t *set)
{
#ifdef M_PCM_EMULATE_CRASH

#endif	

	asm_sfence();
#ifdef M_PCM_EMULATE_LATENCY
	/* If we have pending stores then add latency */
	if (set->seqstream_len > 0) {
		emulate_latency_ns(M_PCM_LATENCY_WRITE);
		set->seqstream_len = 0;
	}

#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _PCM_INTERNAL_H */
