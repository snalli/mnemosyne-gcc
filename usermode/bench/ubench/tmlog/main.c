#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <mnemosyne.h>
#include <mtm.h>
#include "ut_barrier.h"


//#define __DEBUG_BUILD
#define PAGE_SIZE                 4096
#define PRIVATE_REGION_SIZE       (64*1024*PAGE_SIZE)

#define MAX(x,y) ((x)>(y)?(x):(y))

static const char __whitespaces[] = "                                                              ";
#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]

#define MAX_NUM_THREADS 16
#define OPS_PER_CHUNK   4

MNEMOSYNE_PERSISTENT void         *psegment[MAX_NUM_THREADS];

typedef enum {
	SYSTEM_UNKNOWN = -1,
	SYSTEM_MTM = 0,
	num_of_systems
} system_t;	

typedef enum {
	UBENCH_UNKNOWN = -1,
	UBENCH_ATOMIC = 0,
	num_of_benchs
} ubench_t;

char                  *progname = "tmlog";
int                   num_threads;
int                   num_writes;
int                   wsize;
ubench_t              ubench_to_run;
system_t              system_to_use;
unsigned long long    runtime;
struct timeval        global_begin_time;
ut_barrier_t          start_timer_barrier;
ut_barrier_t          start_ubench_barrier;
volatile unsigned int short_circuit_terminate;
unsigned long long    thread_total_iterations[MAX_NUM_THREADS];
unsigned long long    thread_actual_runtime[MAX_NUM_THREADS];

typedef struct {
	unsigned int tid;
	unsigned int iterations_per_chunk;
	void*        fixture_state;
} ubench_args_t;

struct {
	char     *str;
	system_t val;
} systems[] = { 
	{ "mtm", SYSTEM_MTM},
};

struct {
	char     *str;
	ubench_t val;
} ubenchs[] = { 
	{ "atomic", UBENCH_ATOMIC},
};

static void run(void* arg);
void experiment_global_init(void);
void ubench_mtm_atomic(void *);
void fixture_ubench_mtm_atomic(void *arg);

void (*ubenchf_array[1][1])(void *) = {
	{ ubench_mtm_atomic },
};	

void (*ubenchf_array_fixture[1][1])(void *) = {
	{ fixture_ubench_mtm_atomic },
};	

typedef uint64_t word_t;

typedef uint64_t m_hrtime_t;

static inline void asm_cpuid() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}


static inline unsigned long long asm_rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#define gethrtime asm_rdtsc

static
void usage(char *name) 
{
	printf("usage: %s   %s\n", name                    , "--system=SYSTEM_TO_USE");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--ubench=MICROBENCHMARK_TO_RUN");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--runtime=RUNTIME_OF_EXPERIMENT_IN_SECONDS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--threads=NUMBER_OF_THREADS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--nwrites=NUMBER_OF_WRITES_PER_TRANSACTION");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--wsize=SEQBYTES_WRITTEN_PER_OPERATION (multiple of 8)");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--factor=LOCALITY_FACTOR");
	printf("\nValid arguments:\n");
	printf("  --ubench     [atomic]\n");
	printf("  --system     [mtm]\n");
	printf("  --numthreads [1-%d]\n", MAX_NUM_THREADS);
	exit(1);
}


int
main(int argc, char *argv[])
{
	extern char        *optarg;
	pthread_t          threads[MAX_NUM_THREADS];
	int                c;
	int                i;
	int                j;
	char               pathname[512];
	unsigned long long total_iterations;
	unsigned long long avg_runtime;
	double             throughput_writes;
	double             throughput_its;

	/* Default values */
	system_to_use = SYSTEM_MTM;
	ubench_to_run = UBENCH_ATOMIC;
	runtime = 30 * 1000 * 1000;
	num_threads = 1;
	num_writes = 16;
	wsize = sizeof(word_t);


	while (1) {
		static struct option long_options[] = {
			{"system",  required_argument, 0, 's'},
			{"ubench",  required_argument, 0, 'b'},
			{"runtime",  required_argument, 0, 'r'},
			{"threads", required_argument, 0, 't'},
			{"nwrites", required_argument, 0, 'n'},
			{"wsize", required_argument, 0, 'z'},
			{"factor", required_argument, 0, 'f'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "b:r:s:t:n:z:f:",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 's':
				system_to_use = SYSTEM_UNKNOWN;
				for (i=0; i<num_of_systems; i++) {
					if (strcmp(systems[i].str, optarg) == 0) {
						system_to_use = (system_t) i;
						break;
					}
				}
				if (system_to_use == SYSTEM_UNKNOWN) {
					usage(progname);
				}
				break;

			case 'b':
				ubench_to_run = UBENCH_UNKNOWN;
				for (i=0; i<num_of_benchs; i++) {
					if (strcmp(ubenchs[i].str, optarg) == 0) {
						ubench_to_run = (ubench_t) i;
						break;
					}
				}
				if (ubench_to_run == UBENCH_UNKNOWN) {
					usage(progname);
				}
				break;

			case 't':
				num_threads = atoi(optarg);
				break;

			case 'r':
				runtime = atoi(optarg) * 1000 * 1000; 
				break;

			case 'n':
				num_writes	= atoi(optarg);
				break;

			case 'z':
				wsize = atoi(optarg);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				usage(progname);
				break;
     
			default:
				abort ();
		}
	}

	ut_barrier_init(&start_timer_barrier, num_threads+1);
	ut_barrier_init(&start_ubench_barrier, num_threads+1);
	short_circuit_terminate = 0;

	experiment_global_init();

	for (i=0; i<num_threads; i++) {
		pthread_create(&threads[i], NULL, (void *(*)(void *)) run, (void *) i);
	}

	ut_barrier_wait(&start_timer_barrier);
	gettimeofday(&global_begin_time, NULL);
	ut_barrier_wait(&start_ubench_barrier);

	total_iterations = 0;
	avg_runtime = 0;
	for (i=0; i<num_threads; i++) {
		pthread_join(threads[i], NULL);
		total_iterations += thread_total_iterations[i];
		avg_runtime += thread_actual_runtime[i];
	}
	avg_runtime = avg_runtime/num_threads;
	throughput_its = ((double) total_iterations) / ((double) avg_runtime);
	throughput_writes = ((double) (total_iterations * num_writes) ) / ((double) avg_runtime);

	printf("runtime           %llu ms\n", avg_runtime/1000);
	printf("total_its         %llu\n", total_iterations);
	printf("total_writes      %llu\n", total_iterations * num_writes);
	printf("throughput_its    %f (iterations/s) \n", throughput_its * 1000 * 1000);
	printf("throughput_writes %f (ops/s) \n", throughput_writes * 1000 * 1000);

	return 0;
}


static
void run(void* arg)
{
 	unsigned int       tid = (unsigned int) arg;
	ubench_args_t      args;
	void               (*ubenchf)(void *);
	void               (*ubenchf_fixture)(void *);
	struct timeval     current_time;
	unsigned long long experiment_time_runtime;
	unsigned long long n;

	args.tid = tid;
	args.iterations_per_chunk = 1024;

	ubenchf = ubenchf_array[ubench_to_run][system_to_use];

	ubenchf_fixture = ubenchf_array_fixture[ubench_to_run][system_to_use];

	if (ubenchf_fixture) {
		args.fixture_state = NULL;
		ubenchf_fixture(&args);
	}
	ut_barrier_wait(&start_timer_barrier);
	ut_barrier_wait(&start_ubench_barrier);

	n = 0;
	do {
		ubenchf(&args);
		n++;
		gettimeofday(&current_time, NULL);
		experiment_time_runtime = 1000000 * (current_time.tv_sec - global_begin_time.tv_sec) +
		                           current_time.tv_usec - global_begin_time.tv_usec;
	} while (experiment_time_runtime < runtime && short_circuit_terminate == 0);
	
	short_circuit_terminate = 1;
	thread_actual_runtime[tid] = experiment_time_runtime;
	thread_total_iterations[tid] = n * args.iterations_per_chunk;

	if (ubenchf_fixture) {
		if (args.fixture_state) {
			free(args.fixture_state);
		}
	}
}


void experiment_global_init(void)
{
	/* nothing to do here */
}


/* 
 * MTM ATOMIC
 */

typedef struct fixture_state_ubench_mtm_atomic_s fixture_state_ubench_mtm_atomic_t;
struct fixture_state_ubench_mtm_atomic_s {
	unsigned int  seed;
	word_t        *segment;
};


void fixture_ubench_mtm_atomic(void *arg, system_t system)
{
 	ubench_args_t*                      args = (ubench_args_t *) arg;
 	unsigned int                        tid = args->tid;
	fixture_state_ubench_mtm_atomic_t*  fixture_state;
	char                                filename[128];

	fixture_state = (fixture_state_ubench_mtm_atomic_t*) malloc(sizeof(fixture_state_ubench_mtm_atomic_t));
	fixture_state->seed = 0;

	if (!psegment[tid]) {
		psegment[tid] = m_pmap((void *) 0xb0000000, PRIVATE_REGION_SIZE, PROT_READ|PROT_WRITE, 0);
	}
	assert(psegment[tid] != (void *) -1);
	fixture_state->segment = (word_t *) psegment[tid];

	args->fixture_state = (void *) fixture_state;
}



void ubench_mtm_atomic(void *arg)
{
 	ubench_args_t*                      args = (ubench_args_t *) arg;
 	unsigned int                        tid = args->tid;
 	unsigned int                        iterations_per_chunk = args->iterations_per_chunk;
	fixture_state_ubench_mtm_atomic_t*  fixture_state = args->fixture_state;
	unsigned int*                       seedp = &fixture_state->seed;
	uint64_t                            private_region_base = (uint64_t) fixture_state->segment;
	uint64_t                            block_addr;
	uint64_t*                           word_addr;
	int                                 i;
	int                                 j;
	int                                 k;
	int                                 n;
	uint64_t                            block_size = MAX(wsize, PAGE_SIZE);
	int                                 local_num_writes = num_writes;

	for (i=0; i<iterations_per_chunk; i++) {
		MNEMOSYNE_ATOMIC {		
			for (j=0; j<num_writes; j++) {
				__tm_waiver {
					block_addr = (private_region_base + 
					              block_size * (rand_r(seedp) % 
			                            (PRIVATE_REGION_SIZE/block_size)));
				}
				for (k=0; k<wsize; k+=sizeof(word_t)) {
					word_addr = (uint64_t *) (block_addr+k); 
					*word_addr = (uint64_t) word_addr;
				}
			}
		}	
	}
}
