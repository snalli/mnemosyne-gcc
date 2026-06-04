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

#include <xmmintrin.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <assert.h>
#include <spinlock.h>
#include "ut_barrier.h"


#define PAGE_SIZE      4096
#define CACHELINE_SIZE 64

#define CPUFREQ 2500LLU /* GHz */

typedef enum {
	TOOL_UNKNOWN = -1,
	TOOL_MEMCPY = 0,
	TOOL_STORESEQ = 1,
	TOOL_STOREPCM = 2,
	num_of_tools
} tool_t;

void tool_memcpy(int tid);
void tool_storeseq_pcm(int tid);
void tool_storeseq_ram(int tid);

typedef struct tool_functions_s {
	char *str;
	void (*tool)(int);
} tool_functions_t;

typedef uint64_t hrtime_t;
typedef uint64_t pcm_word_t;

tool_functions_t tools[] = {
	{"memcpy", tool_memcpy},
	{"storeseq_ram", tool_storeseq_ram},
	{"storeseq_pcm", tool_storeseq_pcm}
};
	

ut_barrier_t global_barrier;
int          pcm_bandwidth_MB;
int          dram_system_peak_bandwidth_MB;
char         *prog_name = "bandwidth-pcm";
int          tool_to_run;
int          range_low;
int          range_high;
int          range_step;
int          nloops;
int          nthreads;
uint64_t     idle_time_ns;
volatile int global;
hrtime_t     latency_array[256];


#define gethrtime asm_rdtsc
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define NS2CYCLE(__ns) ((__ns) * CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / CPUFREQ)
#define USEC(val) (val.tv_sec*1000000LLU + val.tv_usec)

static const char __whitespaces[] = "                                                                                                                                    ";
#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]


static inline void asm_cpuid() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}


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


static inline
int rand_int(unsigned int *seed)
{
    *seed=*seed*196314165+907633515;
    return *seed;
}


static inline
void
emulate_latency_cycles(int cycles)
{
	hrtime_t start;
	hrtime_t stop;

	start = asm_rdtsc();

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
emulate_latency_nop_cycles(int cycles)
{
	int      i;
	
	for (i=0; i<cycles; i+=10) {
		asm_nop10();
	}
}

static void learn_latencies(hrtime_t latency_array[])
{
	hrtime_t       cycles;
	hrtime_t       start;
	hrtime_t       stop;
	hrtime_t       duration;
	hrtime_t       duration_ns;
	hrtime_t       cpuid_delay_ns;
	int            td;
	int            i;
	int            j;

	duration = 0;
	for (i=0; i<100; i++) {
		asm_cpuid();
		start = gethrtime();
		asm_cpuid();
		stop = gethrtime();
		duration += (stop - start);
	}	
	cpuid_delay_ns = CYCLE2NS(duration)/100;

	for (td=0; td<256; td++) { 
		for (cycles=0; cycles<4096; cycles++) { 
			duration=0;
			for (i=0; i<100; i++) {
				asm_cpuid();
				start = gethrtime();
				emulate_latency_nop_cycles(cycles);
				asm_cpuid();
				stop = gethrtime();
				duration += (stop - start);
			}
			duration_ns = CYCLE2NS(duration)/100 - cpuid_delay_ns;
				printf("%d, %llu, %llu, %llu\n", td, duration_ns, duration, cpuid_delay_ns);
			if (duration_ns >= td) {
				printf("%d, %llu, %llu, %llu\n", td, duration_ns, duration, cpuid_delay_ns);
				latency_array[td] = cycles;
				break;
			}
		}
	}
}

static void print_latencies(hrtime_t latency_array[])
{
	hrtime_t       cycles;
	hrtime_t       start;
	hrtime_t       stop;
	hrtime_t       duration;
	hrtime_t       duration_ns;
	hrtime_t       cpuid_delay_ns;
	int            td;
	int            i;
	int            j;

	for (td=0; td<256; td++) { 
		printf("target_delay=%d (ns), delay=%llu (cycles)\n", td, latency_array[td]);
	}
}



static inline
void
emulate_latency_ns(int ns)
{
	hrtime_t cycles;
	cycles = NS2CYCLE(ns);
	emulate_latency_cycles(cycles);
}

static inline
void
emulate_latency_ns_array(hrtime_t latency_array[], int ns)
{
	hrtime_t cycles;
	emulate_latency_nop_cycles(latency_array[ns]);
}

static inline void asm_mov(pcm_word_t *addr, pcm_word_t val)
{
	__asm__ __volatile__ ("");
	__asm__ __volatile__ ("mov %1, %0" : "=m"(*addr): "r" (val));
}


static inline void asm_movnti(pcm_word_t *addr, pcm_word_t val)
{
	__asm__ __volatile__ ("");
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}

static inline void asm_sse_write_block64_single_val(uintptr_t *addr, pcm_word_t val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[0]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[1]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[2]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[3]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[4]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[5]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[6]): "r" (val));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[7]): "r" (val));
}

static inline void asm_sse_write_block64(uintptr_t *addr, pcm_word_t *val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[0]): "r" (val[0]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[1]): "r" (val[1]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[2]): "r" (val[2]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[3]): "r" (val[3]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[4]): "r" (val[4]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[5]): "r" (val[5]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[6]): "r" (val[6]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[7]): "r" (val[7]));
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

void bind_thread(int tid, int cpuid)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpuid, &mask);
	if( sched_setaffinity( 0, sizeof(mask), &mask ) == -1 ) {
		printf("WARNING: Could not bind thread %d on CPU %d, continuing...\n", tid, cpuid);
	} else {
		printf("Thread %d bound on CPU %d\n", tid, cpuid);
	}
	fflush(stdout);
}



int WriterSSE (char *ptr, unsigned long loops, unsigned long size, unsigned long value)
{
	unsigned long i;
	unsigned long j;
	unsigned long value_array[8];

	for (j=0; j<loops;j++) {
		for (i=0; i<=size; i+=64)  {
			//asm_sse_write_block64_single_val((void *) &ptr[i], value);
			asm_sse_write_block64((void *) &ptr[i], value_array);
		}
	}
	return 0;
}


inline unsigned int
unsigned_int_fetch_and_add_full (volatile unsigned int *p, unsigned int incr)
{
  unsigned int result;

  __asm__ __volatile__ ("lock; xaddl %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr), "m" (*p)
                        : "memory");
  return result;
}

volatile arch_spinlock_t ticket_lock = {0} ;

void
memset_random(void *ptr, unsigned int seed, unsigned int size)
{
	int          i;
	char         *cptr = (char *) ptr;
	unsigned int myseed = seed;
	int          val;

	for (i=0; i<size; i++) {
		val = rand_r(&myseed) % 128;
		cptr[i] = (char) val;
	}
}


/* 
 * pcm_memcpy_internal_user: this is the user-mode equivalent of the function
 * pcm_memcpy_internal that is used by our PCMdisk
 */

static
void *
pcm_memcpy_internal_user(void *dst, const void *src, size_t n)
{
	volatile uint8_t *saddr=((volatile uint8_t *) src);
	volatile uint8_t *daddr=dst;
	uint8_t          offset;
 	pcm_word_t       *val;
	size_t           size = n;
	int              extra_latency;

	if (size == 0) {
		return dst;
	}

	/* First write the new data. */
	while(size >= sizeof(pcm_word_t) * 8) {
		val = ((pcm_word_t *) saddr);
		asm_sse_write_block64((uintptr_t *) daddr, (pcm_word_t *) val);
		saddr+=8*sizeof(pcm_word_t);
		daddr+=8*sizeof(pcm_word_t);
		size-=8*sizeof(pcm_word_t);
	}
	if (size > 0) {
		/* Ugly hack: asm_sse_write_block64 requires a 64 bytes size value. We move
		 * back a few bytes to form a block of 64 bytes.
		 */ 
		offset = 64 - size;
		saddr-=offset;
		daddr-=offset;
		val = ((pcm_word_t *) saddr);
		asm_sse_write_block64((uintptr_t *) daddr, val);

	}

	size = n;

	/* Now make sure data is flushed out */
	extra_latency = (int) size * (1-(float) (((float) pcm_bandwidth_MB)/1000)/(((float) dram_system_peak_bandwidth_MB)/1000))/(((float)pcm_bandwidth_MB)/1000);
	asm_mfence();
	__ticket_spin_lock(&ticket_lock);
	emulate_latency_ns(extra_latency);
	__ticket_spin_unlock(&ticket_lock);
	asm_cpuid();

	return dst;
}


void tool_memcpy(int tid)
{
	hrtime_t       start;
	hrtime_t       stop;
	hrtime_t       duration;
	char           *A;
	char           *B;
	int            i;
	int            j;
	int            n;
	int            k;
	size_t         size;
	double         throughput;
	uintptr_t      local[1024];
	struct timeval starttime;
	struct timeval stoptime;
	unsigned long  value = 0x1234567689abcdef;
	unsigned long  value_array[8];
	unsigned int   tmp;

	size = 64*1024*1024;
	posix_memalign((void **)&A, CACHELINE_SIZE, size);
	posix_memalign((void **)&B, CACHELINE_SIZE, size);
	memset(A, 0, size);
	memset(B, 0, size);
	fprintf(stderr, "T%d: Memory allocation: DONE\n", tid);

	for (n=range_low; n<=range_high; n+=range_step) {
		ut_barrier_wait(&global_barrier);
		asm_cpuid();
		start = gethrtime();
		for (j=0; j<nloops;j++) {
			pcm_memcpy_internal_user(B, A, n);
			emulate_latency_ns(idle_time_ns);
		}
		asm_cpuid();
		stop = gethrtime();
		duration = (stop - start)/nloops;
		throughput = 1000*((double) n) / ((double) CYCLE2NS(duration));
		printf("T%d: write_size = %d (bytes), duration = %llu cycles, throughput_bytes = %lf (MB/s) \n", tid, n, duration, throughput);
		fflush(stdout);
	}
}


void tool_storeseq_internal(int tid, int emulate_idle_time)
{
	hrtime_t       start;
	hrtime_t       stop;
	hrtime_t       duration;
	char           *A;
	char           *B;
	int            i;
	int            j;
	int            n;
	int            k;
	size_t         size;
	double         throughput;
	uintptr_t      local[1024];
	struct timeval starttime;
	struct timeval stoptime;
	unsigned long  value = 0x1234567689abcdef;
	unsigned long  value_array[8];
	unsigned int   tmp;

	size = 64*1024*1024;
	posix_memalign((void **)&A, CACHELINE_SIZE, size);
	posix_memalign((void **)&B, CACHELINE_SIZE, 64);
	memset(A, 0, size);
	memset(B, 0, 64);
	fprintf(stderr, "T%d: Memory allocation: DONE\n", tid);

	for (n=range_low; n<=range_high; n+=range_step) {
		ut_barrier_wait(&global_barrier);
		asm_cpuid();
		start = gethrtime();
		for (j=0; j<nloops;j++) {
			for (k=0; k<n; k+=64) {
				asm_sse_write_block64((uintptr_t *) &A[k], (pcm_word_t *) B);
				if (emulate_idle_time) { /* compiler will optimize out */
					emulate_latency_ns(idle_time_ns);
			        //for (i=0; i<100; i++) {
		            //    asm volatile("nop");
			        //}
				}	
			}
		}
		asm_mfence();
		asm_cpuid();
		stop = gethrtime();
		duration = (stop - start)/nloops;
		throughput = 1000*((double) n) / ((double) CYCLE2NS(duration));
		printf("T%d: write_size = %d (bytes), duration = %llu cycles (%llu ns), throughput_bytes = %lf (MB/s) \n", tid, n, duration, CYCLE2NS(duration), throughput);
		fflush(stdout);
	}
}


void tool_storeseq_ram(int tid)
{
	tool_storeseq_internal(tid, 0);
}


void tool_storeseq_pcm(int tid)
{
	tool_storeseq_internal(tid, 1);
}


void *slave(void *arg)
{
	int tid = (int) arg;

	bind_thread(tid, tid);
	tools[tool_to_run].tool(tid);
	return 0;
}

static
void usage(FILE *fout, char *name) 
{
	fprintf(fout, "usage:");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--tool=TOOL_TO_RUN");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--nthreads=NUMBER_OF_THREADS");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--range_low=RANGE_LOW_VALUE");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--range_high=RANGE_HIGH_VALUE");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--range_step=RANGE_STEP_INCREMENT");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--idle=IDLE_TIME_BEFORE_WRITING (ns)");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--nloops=NUMBER_OF_LOOPS");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--pcmbw=PCM_BANDWIDTH (MB)");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--drambw=DRAM_MEMORY_SYSTEM_PEAK_BANDWIDTH (MB)");
	fprintf(fout, "\nValid arguments:\n");
	fprintf(fout, "  --tool     [memcpy, storeseq_pcm, storeseq_ram]\n");
	exit(1);
}


main(int argc, char *argv[])
{
	pthread_t threads[16];
	int       i;
	int       c;

	/* Default values */
	tool_to_run = TOOL_MEMCPY;
	nthreads = 1;
	range_low = 0;
	range_high = 4096;
	range_step = 8;
	nloops = 10;
	pcm_bandwidth_MB = 1200;
	dram_system_peak_bandwidth_MB = 6000;
	idle_time_ns = 1000;

	while (1) {
		static struct option long_options[] = {
			{"tool",  required_argument, 0, 't'},
			{"nthreads", required_argument, 0, 'n'},
			{"range_low", required_argument, 0, 'l'},
			{"range_high", required_argument, 0, 'h'},
			{"range_step", required_argument, 0, 's'},
			{"idle", required_argument, 0, 'i'},
			{"nloops", required_argument, 0, 'o'},
			{"pcmbw", required_argument, 0, 'p'},
			{"drambw", required_argument, 0, 'r'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "s:b:t:n:r:w:v:k:o:",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 't':
				tool_to_run = TOOL_UNKNOWN;
				for (i=0; i<num_of_tools; i++) {
					if (strcmp(tools[i].str, optarg) == 0) {
						tool_to_run = (tool_t) i;
						break;
					}
				}
				if (tool_to_run == TOOL_UNKNOWN) {
					usage(stderr, prog_name);
				}
				break;

			case 'n':
				nthreads = atoi(optarg);
				break;

			case 'l':
				range_low = atoi(optarg);
				break;

			case 'h':
				range_high = atoi(optarg);
				break;

			case 's':
				range_step = atoi(optarg);
				break;

			case 'o':
				nloops = atoi(optarg);
				break;

			case 'p':
				pcm_bandwidth_MB = atoi(optarg);
				break;

			case 'd':
				dram_system_peak_bandwidth_MB = atoi(optarg);
				break;

			case 'i':
				idle_time_ns = atoi(optarg);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				usage(stderr, prog_name);
				break;
     
			default:
				abort ();
		}
	}

	ut_barrier_init(&global_barrier, nthreads);

	for (i=0; i<nthreads; i++) {
		pthread_create(&threads[i], NULL, slave, (void *) i);
	}	

	for (i=0; i<nthreads; i++) {
		pthread_join(threads[i], NULL);
	}	
}
