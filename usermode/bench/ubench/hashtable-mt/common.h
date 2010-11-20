#ifndef _COMMON_H_JIKAK1
#define _COMMON_H_JIKAK1

#include "aux.h"

#define USE_ELEMSET
//#undef  USE_ELEMSET

#define USE_PER_THREAD_HASH_TABLE
#undef  USE_PER_THREAD_HASH_TABLE

#define _DEBUG_THIS
#undef _DEBUG_THIS

#define MAX_NUM_THREADS 64
#define OPS_PER_CHUNK   4

#define MAX(x,y) ((x)>(y)?(x):(y))

#define INTERNAL_ERROR(msg)                                             \
do {                                                                    \
  fprintf(stderr, "ERROR [%s:%d] %s.\n", __FILE__, __LINE__, msg);      \
  exit(-1);                                                             \
} while(0);

static const char __whitespaces[] = "                                                                                                                                    ";
#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]

static const char __equalsigns[] = "======================================================================================================================================";
#define EQUALSIGNS(len) &__equalsigns[sizeof(__equalsigns) - (len) -1]


typedef uint64_t word_t;

enum {
	OP_HASH_READ = 0,
	OP_HASH_WRITE = 1,
};

typedef enum {
	UBENCH_UNKNOWN = -1,
	UBENCH_MTM_MIX_LATENCY = 0,
	UBENCH_MTM_MIX_THROUGHPUT = 1,
	UBENCH_MTM_MIX_LATENCY_THINK = 2,
	UBENCH_MTM_MIX_THROUGHPUT_THINK = 3,
	UBENCH_BDB_MIX_LATENCY = 4,
	UBENCH_BDB_MIX_THROUGHPUT = 5,
	num_of_benchs
} ubench_t;


typedef struct ubench_functions_s {
	char *str;
	int (*init)(void *);
	int (*fini)(void *);
	int (*thread_init)(unsigned int);
	int (*thread_fini)(unsigned int);
	void (*thread_main)(unsigned int);
	void (*print_stats)(FILE *);
} ubench_functions_t;


typedef struct ubench_desc_s {
	unsigned int       iterations_per_chunk;
	void               *global_state;
	void               *thread_state[MAX_NUM_THREADS];
	uint64_t           thread_runtime[MAX_NUM_THREADS];
	ubench_functions_t functions;
} ubench_desc_t;


extern char                  *prog_name;
extern char                  *db_home_dir_prefix;
extern int                   num_threads;
extern int                   percent_read;
extern int                   percent_write;
extern int                   work_percent;
extern int                   vsize;
extern int                   num_keys;
extern ubench_t              ubench_to_run;
extern unsigned long long    runtime;
extern struct timeval        global_begin_time;
extern ut_barrier_t          start_timer_barrier;
extern ut_barrier_t          start_ubench_barrier;
extern volatile unsigned int short_circuit_terminate;
extern ubench_desc_t         ubench_desc;


#include <sys/types.h>
#include <unistd.h>





static inline 
int random_operation(unsigned int *seedp, int percent_write)
{
	int n;

	n = rand_r(seedp) % 100;

	if (n < percent_write) {
		return OP_HASH_WRITE;
	} else {
		return OP_HASH_READ;
	}
}

static inline
double mydiv(uint64_t a, uint64_t b) {
	if (b == 0) {
		return -0.0;
	}
	return ((double) a) / ((double) b);
}


#define __PRINT_NAMEVAL(prefix, name, value, units, separator)              \
  char separator_str[128];                                                  \
  char prefix_name[128];                                                    \
  char unitstr[32];                                                         \
                                                                            \
  sprintf(prefix_name, "%s.%s", prefix, name);                              \
  if (strlen(units)>0) {                                                    \
    sprintf(unitstr, " (%s)", units);                                       \
  } else {                                                                  \
    sprintf(unitstr, "");                                                   \
  }                                                                         \
                                                                            \
  if (strncmp(separator, "ws,", 3)==0) {                                    \
    char *slen = &separator[3];                                             \
    int  len = atoi(slen);                                                  \
    strcpy(separator_str, WHITESPACE(len));                                 \
  } else if (strncmp(separator, "rj,", 3)==0) {                             \
    char *slen = &separator[3];                                             \
    int  len = atoi(slen);                                                  \
    strcpy(separator_str, WHITESPACE(len-strlen(prefix_name)));             \
  } else {                                                                  \
    strcpy(separator_str, separator);                                       \
  }                                                                         


#define PRINT_NAMEVAL_DOUBLE(prefix, name, value, units, separator)           \
do {                                                                          \
  __PRINT_NAMEVAL(prefix, name, value, units, separator)                      \
  fprintf(fout, "%s%s%f%s\n", prefix_name, separator_str, value, unitstr);    \
} while (0);
 
 
#define PRINT_NAMEVAL_INT(prefix, name, value, units, separator)              \
do {                                                                          \
  __PRINT_NAMEVAL(prefix, name, value, units, separator)                      \
  fprintf(fout, "%s%s%d%s\n", prefix_name, separator_str, value, unitstr);    \
} while (0);
 

#define PRINT_NAMEVAL_U64(prefix, name, value, units, separator)              \
do {                                                                          \
  __PRINT_NAMEVAL(prefix, name, value, units, separator)                      \
  fprintf(fout, "%s%s%llu%s\n", prefix_name, separator_str, value, unitstr);  \
} while (0);


#define PRINT_NAMEVAL_STR(prefix, name, value, units, separator)              \
do {                                                                          \
  __PRINT_NAMEVAL(prefix, name, value, units, separator)                      \
  fprintf(fout, "%s%s%s%s\n", prefix_name, separator_str, value, unitstr);    \
} while (0);


#endif
