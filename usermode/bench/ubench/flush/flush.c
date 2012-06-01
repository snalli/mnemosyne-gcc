#include <xmmintrin.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include "../../../library/common/ut_barrier.h"

#define PAGE_SIZE      4096
#define CACHELINE_SIZE 64

ut_barrier_t global_barrier;
int nloops;
int objsize;
void (*experiment_function)(int tid);
volatile int global;

typedef uint64_t hrtime_t;
typedef uint64_t pcm_word_t;

#define gethrtime asm_rdtsc
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define CPUFREQ 2500LLU /* MHz */
#define NS2CYCLE(__ns) ((__ns) * CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / CPUFREQ)
#define USEC(val) (val.tv_sec*1000000LLU + val.tv_usec)

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

/* simple loop based delay: */
static void delay_loop(unsigned long loops)
{
        asm volatile(
                "       test %0,%0      \n"
                "       jz 3f           \n"
                "       jmp 1f          \n"

                ".align 16              \n"
                "1:     jmp 2f          \n"

                ".align 16              \n"
                "2:     dec %0          \n"
                "       jnz 2b          \n"
                "3:     dec %0          \n"

                : /* we don't need output */
                :"a" (loops)
        );
}



static inline
void
emulate_latency_ns(int ns)
{
	hrtime_t cycles;
	cycles = NS2CYCLE(ns);
	emulate_latency_cycles(cycles);
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

static inline void asm_sse_write_cacheline(uintptr_t *addr, pcm_word_t val)
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
		//printf("Thread %d bound on CPU %d\n", tid, cpuid);
	}
	fflush(stdout);
}

void write_object(void* addr, int size)
{
	memset(addr, 0, size);
}

void ntwrite_object(void* addr, int size)
{
	int      i;
	uint64_t uaddr;

	for (i=0; i<size; i+=8) {
		uaddr = (uint64_t) addr + i;
		asm_movnti(uaddr, 0); 
	}
}


void clflush_object(void* addr, int size) 
{
	int      i;
	uint64_t uaddr;

	asm_mfence();
	for (i=0; i<size; i+=CACHELINE_SIZE) {
		uaddr = (uint64_t) addr + i;
		asm_clflush((void*) uaddr);
	}
	asm_mfence();
}


void ntflush_object(void* addr, int size) 
{
	asm_mfence();
}


void experiment_clflush(int tid)
{
	hrtime_t start;
	hrtime_t stop;
	hrtime_t duration;
	char *A;
	char *B;
	int i;
	int j;
	int n;
	int nops = 0;
	int offset;
	size_t size;
	double throughput;
	uintptr_t local[1024];
	struct timeval starttime;
	struct timeval stoptime;
	size = 64*1024*1024;

	posix_memalign((void **)&A, CACHELINE_SIZE, size);
	posix_memalign((void **)&B, CACHELINE_SIZE, size);
	memset(A, 0, size);
	memset(B, 0, size);
	ut_barrier_wait(&global_barrier);

	start = gethrtime();
	for (n=0; n<=nloops; n++) {
		asm_cpuid();
		for (j=0; j<size;j+=objsize) {
			write_object((void*)&A[j], objsize);
			clflush_object((void*)&A[j], objsize);
			nops++;
		}
	}
	stop = gethrtime();
	duration = (stop - start);
	throughput = 1000*((double) nops) / ((double) CYCLE2NS(duration));
	printf("T%d: objsize = %d, nops = %llu, duration = %llu cycles, duration/op = %llu cycles, throughput = %lf (Mflushes/s)\n", tid, objsize, nops, duration, duration/nops, throughput);
	fflush(stdout);
}


void experiment_ntflush(int tid)
{
	hrtime_t start;
	hrtime_t stop;
	hrtime_t duration;
	char *A;
	char *B;
	int i;
	int j;
	int n;
	int nops = 0;
	int offset;
	size_t size;
	double throughput;
	uintptr_t local[1024];
	struct timeval starttime;
	struct timeval stoptime;
	size = 64*1024*1024;

	posix_memalign((void **)&A, CACHELINE_SIZE, size);
	posix_memalign((void **)&B, CACHELINE_SIZE, size);
	memset(A, 0, size);
	memset(B, 0, size);
	ut_barrier_wait(&global_barrier);

	start = gethrtime();
	for (n=0; n<=nloops; n++) {
		asm_cpuid();
		for (j=0; j<size;j+=objsize) {
			ntwrite_object((void*)&A[j], objsize);
			ntflush_object((void*)&A[j], objsize);
			nops++;
		}
	}
	stop = gethrtime();
	duration = (stop - start);
	throughput = 1000*((double) nops) / ((double) CYCLE2NS(duration));
	printf("T%d: objsize = %d, nops = %llu, duration = %llu cycles, duration/op = %llu cycles, throughput = %lf (Mflushes/s)\n", tid, objsize, nops, duration, duration/nops, throughput);
	fflush(stdout);
}

void *slave(void *arg)
{
	int tid = (int) arg;

	bind_thread(tid, tid);
	experiment_function(tid);
}


main(int argc, char *argv[])
{
	pthread_t threads[16];
	int i;
	int experiment = atoi(argv[1]);
	int nthreads = atoi(argv[2]);
	objsize = atoi(argv[3]);
	nloops = atoi(argv[4]);
	switch (experiment) {
		case 1:
			experiment_function = experiment_clflush;
			break;
		case 2:
			experiment_function = experiment_ntflush;
			break;
		default:
			fprintf(stderr, "ERROR: Unknown experiment");
			exit(1);
	}

	ut_barrier_init(&global_barrier, nthreads);

	for (i=0; i<nthreads; i++) {
		pthread_create(&threads[i], NULL, slave, (void *) i);
	}	

	for (i=0; i<nthreads; i++) {
		pthread_join(threads[i], NULL);
	}	
}
