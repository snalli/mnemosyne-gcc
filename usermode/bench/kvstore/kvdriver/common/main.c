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
#include <sched.h>
#include "ut_barrier.h"
#include "common.h"

void print_vmstats(char *prefix) {
	char filename[128];
	pid_t pid;
	FILE *fin;
	char c;

	pid = getpid();
	sprintf(filename, "/proc/%d/stat", pid);
	fin = fopen(filename, "r");
	printf("%s\n", filename);
	while ((c=fgetc(fin))!=EOF) {
		printf("%c", c);
	}
}


char                  *prog_name = "kvdriver";
int                   num_threads;
int                   percent_read;
int                   percent_write;
int                   work_percent;
int                   vsize;
int                   num_keys;
int                   ubench_to_run;
unsigned long long    runtime;
struct timeval        global_begin_time;
ut_barrier_t          start_timer_barrier;
ut_barrier_t          start_ubench_barrier;
volatile unsigned int short_circuit_terminate;
ubench_desc_t         ubench_desc;

static void run(void* arg);

/* nolock versions rely on memory transactions for isolation */


static
void usage(FILE *fout, char *name) 
{
	int i;

	fprintf(fout, "usage:\n");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--ubench=MICROBENCHMARK_TO_RUN");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--runtime=RUNTIME_OF_EXPERIMENT_IN_SECONDS");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--nthreads=NUMBER_OF_THREADS");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--write=PERCENTAGE_OF_WRITE_OPERATIONS (PUT/DEL)");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--read=PERCENTAGE_OF_READ_OPERATIONS (GET)");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--vsize=VALUE_SIZE");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--nkeys=KEY_SPACE");
	fprintf(fout, "       %s   %s\n", WHITESPACE(strlen(name)), "--work=WORK_PERCENT");
	fprintf(fout, "\n");
	fprintf(fout, "Valid arguments:\n");
	fprintf(fout, "  --ubench    [");
	for (i=0; ubenchs[i].str != NULL; i++) {
		printf("%s,", ubenchs[i].str);
	}
	printf("]\n");
	fprintf(fout, "  --nthreads  [1-%d]\n", MAX_NUM_THREADS);
	exit(1);
}

void print_configuration(FILE *fout)
{
	char *ubench_str = ubenchs[ubench_to_run].str;
	char prefix[] = "bench.config";
	char separator[] = "rj,40";

	fprintf(fout, "CONFIGURATION\n");
	fprintf(fout, "%s\n", EQUALSIGNS(60));

	PRINT_NAMEVAL_STR(prefix, "ubench", ubench_str, "", separator);
	PRINT_NAMEVAL_INT(prefix, "num_threads", num_threads, "", separator);
	PRINT_NAMEVAL_INT(prefix, "num_keys", num_keys, "", separator);
	PRINT_NAMEVAL_INT(prefix, "vsize", vsize, "", separator);
	PRINT_NAMEVAL_INT(prefix, "percent_write", percent_write, "", separator);
	PRINT_NAMEVAL_INT(prefix, "percent_read", percent_read, "", separator);
	fprintf(fout, "\n");
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
	int               (*ubench_init)(void *);
	int               (*ubench_fini)(void *);
	int               (*ubench_thread_init)(unsigned int);
	int               (*ubench_thread_fini)(unsigned int);
	void              (*ubench_print_stats)(FILE *);

	/* Default values */
	ubench_to_run = 0;
	runtime = 30 * 1000 * 1000;
	num_threads = 1;
	percent_write = 100;
	percent_read = 0;
	num_keys = 1000;
	work_percent = 100; /* No thinking; just work */
	vsize = sizeof(word_t);


	// TODO: Support passing specific options to a backend experiment 
	while (1) {
		static struct option long_options[] = {
			{"ubench",  required_argument, 0, 'b'},
			{"runtime",  required_argument, 0, 't'},
			{"nthreads", required_argument, 0, 'n'},
			{"read", required_argument, 0, 'r'},
			{"write", required_argument, 0, 'w'},
			{"vsize", required_argument, 0, 'v'},
			{"nkeys", required_argument, 0, 'k'},
			{"work", required_argument, 0, 'o'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "s:b:t:n:r:w:v:k:o:",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 'b':
				if ((ubench_to_run = ubench_str2index(optarg)) == -1) {
					usage(stderr, prog_name);
				}
				break;

			case 'n':
				num_threads = atoi(optarg);
				break;

			case 't':
				runtime = atoi(optarg) * 1000 * 1000; 
				break;

			case 'w':
				percent_write = atoi(optarg);
				break;

			case 'r':
				percent_read = atoi(optarg);
				break;

			case 'v':
				vsize = atoi(optarg);
				break;

			case 'k':
				num_keys = atoi(optarg);
				break;

			case 'o':
				work_percent = atoi(optarg);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				usage(stderr, prog_name);
				break;
     
			default:
				abort ();
		}
	}


	if (percent_write + percent_read != 100) {
		printf("Percentages must add to 100.\n");
		exit(-1);	
	}

	ut_barrier_init(&start_timer_barrier, num_threads+1);
	ut_barrier_init(&start_ubench_barrier, num_threads+1);
	short_circuit_terminate = 0;

	ubench_init = ubenchs[ubench_to_run].init;
	ubench_fini = ubenchs[ubench_to_run].fini;
	ubench_thread_init = ubenchs[ubench_to_run].thread_init;
	ubench_thread_fini = ubenchs[ubench_to_run].thread_fini;
	ubench_print_stats = ubenchs[ubench_to_run].print_stats;

	ubench_desc.iterations_per_chunk = 1024;

	/* 
	 * Setting the transaction mode via environment variable should be okay
	 * as MTM is initialized later at the first transactional thread. 
	 */

	/* initialize the experiment */
	if (ubench_init(NULL) != 0) {
		fprintf(stderr, "Error initializing the experiment\n");
		goto err;
	}

	for (i=0; i<num_threads; i++) {
		if (ubench_thread_init) {
			ubench_thread_init(i);
		} 
		pthread_create(&threads[i], NULL, (void *(*)(void *)) run, (void *) i);
	}

	ut_barrier_wait(&start_timer_barrier);
	gettimeofday(&global_begin_time, NULL);
	ut_barrier_wait(&start_ubench_barrier);

	for (i=0; i<num_threads; i++) {
		pthread_join(threads[i], NULL);
	}
	
	print_configuration(stdout);
	ubench_print_stats(stdout);
	fini_global(); /* report MTM statistics */

	for (i=0; i<num_threads; i++) {
		if (ubench_thread_fini) {
			if (ubench_thread_fini(i) != 0) {
				goto err;
			}
		} 
	}

	if (ubench_fini(NULL) != 0) {
		goto err;
	}

	return 0;
err:

	return -1;
}


static
void run(void* arg)
{
	unsigned int       tid = (unsigned int) arg;
	void               (*ubench_thread_main)(unsigned int);
	struct timeval     current_time;
	unsigned long long experiment_runtime;
	cpu_set_t          cpu_set;

	CPU_ZERO(&cpu_set);
	CPU_SET(tid % 4, &cpu_set);
	sched_setaffinity(0, 4, &cpu_set);

	ubench_thread_main = ubenchs[ubench_to_run].thread_main;

	ut_barrier_wait(&start_timer_barrier);
	fprintf(stderr, "[%d] THREAD START\n", tid);
	ut_barrier_wait(&start_ubench_barrier);

	do {
		ubench_thread_main(tid);
		gettimeofday(&current_time, NULL);
		experiment_runtime = 1000000 * (current_time.tv_sec - global_begin_time.tv_sec) +
		                                     current_time.tv_usec - global_begin_time.tv_usec;
	} while (experiment_runtime < runtime && short_circuit_terminate == 0);
	
	short_circuit_terminate = 1;
	ubench_desc.thread_runtime[tid] = experiment_runtime;

}
