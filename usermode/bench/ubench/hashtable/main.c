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
#include <malloc.h>
#include "ut_barrier.h"
#include <db.h>
#include "uthash.h"

//#define _DEBUG_THIS
//#define _USE_RWLOCK

#define INTERNAL_ERROR(msg)                                             \
do {                                                                    \
  fprintf(stderr, "ERROR [%s:%d] %s.\n", __FILE__, __LINE__, msg);      \
  exit(-1);                                                             \
} while(0);

#define PAGE_SIZE                 4096
#define PRIVATE_REGION_SIZE       (64*1024*PAGE_SIZE)

#define MAX(x,y) ((x)>(y)?(x):(y))

static const char __whitespaces[] = "                                                              ";
#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]

#define MAX_NUM_THREADS 16
#define OPS_PER_CHUNK   4

enum {
	OP_HASH_PUT = 0,
	OP_HASH_GET = 1,
	OP_HASH_DEL = 2
};

typedef enum {
	SYSTEM_UNKNOWN = -1,
	SYSTEM_MTM = 0,
	SYSTEM_BDB = 1,
	num_of_systems
} system_t;	

typedef enum {
	UBENCH_UNKNOWN = -1,
	UBENCH_MIX = 0,
	UBENCH_MIX_THINK = 1,
	num_of_benchs
} ubench_t;

char                  *prog_name = "hashtable";
char                  *db_home_dir = "/mnt/pcmfs";
int                   num_threads;
int                   percent_put;
int                   percent_del;
int                   percent_get;
int                   work_percent;
int                   vsize;
int                   num_keys;
ubench_t              ubench_to_run;
system_t              system_to_use;
unsigned long long    runtime;
struct timeval        global_begin_time;
ut_barrier_t          start_timer_barrier;
ut_barrier_t          start_ubench_barrier;
volatile unsigned int short_circuit_terminate;
unsigned long long    thread_total_iterations[MAX_NUM_THREADS];
unsigned long long    thread_actual_runtime[MAX_NUM_THREADS];
void                  *experiment_global_state=NULL;

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
	{ "bdb", SYSTEM_BDB},
};

struct {
	char     *str;
	ubench_t val;
} ubenchs[] = { 
	{ "mix", UBENCH_MIX},
	{ "mix_think", UBENCH_MIX_THINK},
};

static void run(void* arg);
int experiment_global_init(void);
int experiment_global_fini(void);
int bdb_init(void);
int bdb_fini(void);
void fixture_bdb_mix(void *arg);
void bdb_mix(void *);
int mtm_init(void);
int mtm_fini(void);
void fixture_mtm_mix(void *arg);
void mtm_mix(void *);
void mtm_mix_think(void *);

void (*ubenchf_array[2][2])(void *) = {
	{ mtm_mix, bdb_mix },
	{ mtm_mix_think, NULL },
};	

void (*ubenchf_array_fixture[2][2])(void *) = {
	{ fixture_mtm_mix, fixture_bdb_mix },
	{ fixture_mtm_mix, NULL },
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


__attribute__((tm_callable))
int tx_memcmp(const void *m1, const void *m2, size_t n)
{
	unsigned char *s1 = (unsigned char *) m1;
	unsigned char *s2 = (unsigned char *) m2;

	while (n--) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		s1++;
		s2++;
	}
	return 0;
}
static
void usage(char *name) 
{
	printf("usage: %s   %s\n", name                    , "--system=SYSTEM_TO_USE");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--ubench=MICROBENCHMARK_TO_RUN");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--runtime=RUNTIME_OF_EXPERIMENT_IN_SECONDS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--threads=NUMBER_OF_THREADS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--put=PERCENTAGE_OF_PUT_OPERATIONS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--del=PERCENTAGE_OF_DELETE_OPERATIONS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--get=PERCENTAGE_OF_GET_OPERATIONS");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--vsize=VALUE_SIZE");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--nkeys=KEY_SPACE");
	printf("       %s   %s\n", WHITESPACE(strlen(name)), "--work=WORK_PERCENT");
	printf("\nValid arguments:\n");
	printf("  --ubench     [mix]\n");
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
	double             throughput_its;

	/* Default values */
	system_to_use = SYSTEM_MTM;
	ubench_to_run = UBENCH_MIX;
	runtime = 30 * 1000 * 1000;
	num_threads = 1;
	percent_put = 0;
	percent_del = 0;
	percent_get = 0;
	num_keys = 1000;
	work_percent = 100; /* No thinking; just work */
	vsize = sizeof(word_t);


	while (1) {
		static struct option long_options[] = {
			{"system",  required_argument, 0, 's'},
			{"ubench",  required_argument, 0, 'b'},
			{"runtime",  required_argument, 0, 'r'},
			{"threads", required_argument, 0, 't'},
			{"put", required_argument, 0, 'p'},
			{"get", required_argument, 0, 'g'},
			{"del", required_argument, 0, 'd'},
			{"vsize", required_argument, 0, 'v'},
			{"nkeys", required_argument, 0, 'k'},
			{"work", required_argument, 0, 'w'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "b:r:s:t:p:v:g:d:w:",
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
					usage(prog_name);
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
					usage(prog_name);
				}
				break;

			case 't':
				num_threads = atoi(optarg);
				break;

			case 'r':
				runtime = atoi(optarg) * 1000 * 1000; 
				break;

			case 'p':
				percent_put	= atoi(optarg);
				break;

			case 'd':
				percent_del	= atoi(optarg);
				break;

			case 'g':
				percent_get	= atoi(optarg);
				break;

			case 'v':
				vsize = atoi(optarg);
				break;

			case 'k':
				num_keys = atoi(optarg);
				break;

			case 'w':
				work_percent = atoi(optarg);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				usage(prog_name);
				break;
     
			default:
				abort ();
		}
	}

	if (percent_put + percent_del + percent_get != 100) {
		printf("Percentages must add to 100.\n");
		exit(-1);	
	}

	ut_barrier_init(&start_timer_barrier, num_threads+1);
	ut_barrier_init(&start_ubench_barrier, num_threads+1);
	short_circuit_terminate = 0;

	if (experiment_global_init() != 0) {
		goto out;
	}

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

	printf("runtime           %llu ms\n", avg_runtime/1000);
	printf("total_its         %llu\n", total_iterations);
	printf("throughput_its    %f (iterations/s) \n", throughput_its * 1000 * 1000);

	m_logmgr_fini();
out:
	experiment_global_fini();
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


int experiment_global_init(void)
{
	if (system_to_use == SYSTEM_BDB && ubench_to_run == UBENCH_MIX) {
		return bdb_init();
	}
	if ((system_to_use == SYSTEM_MTM && ubench_to_run == UBENCH_MIX) ||
	    (system_to_use == SYSTEM_MTM && ubench_to_run == UBENCH_MIX_THINK)) {
		return mtm_init();
	}
	return 0;
}


int experiment_global_fini(void)
{
	if (system_to_use == SYSTEM_BDB && ubench_to_run == UBENCH_MIX) {
		return bdb_fini();
	}
	if (system_to_use == SYSTEM_MTM && ubench_to_run == UBENCH_MIX) {
		return mtm_fini();
	}
	return 0;
}


/* 
 * MTM MIX
 */

typedef struct object_s {
	int            id;
	UT_hash_handle hh;
	char           value[0];
} object_t;

typedef struct mtm_hashtable_s {
	object_t         *ht;
	pthread_rwlock_t rwlock;
	pthread_mutex_t  mutex[MAX_NUM_THREADS];
	char             tmp_buf[16384];
} mtm_hashtable_t;


typedef struct mtm_global_s {
	mtm_hashtable_t *hashtable;
} mtm_global_t;

static inline int mtm_op_add(unsigned int tid, mtm_hashtable_t *hashtable, int id, int lock);

int mtm_init(void)
{
	mtm_global_t *mtm_global = NULL;
	object_t     *obj;
	int          i;

	mtm_global = (mtm_global_t *) malloc(sizeof(mtm_global_t));

	MNEMOSYNE_ATOMIC 
	{
		mtm_global->hashtable = (mtm_hashtable_t *) pmalloc(sizeof(mtm_hashtable_t));
	}	
	mtm_global->hashtable->ht = NULL;
	pthread_rwlock_init(&mtm_global->hashtable->rwlock, NULL);
	for (i=0; i<MAX_NUM_THREADS; i++) {
		pthread_mutex_init(&mtm_global->hashtable->mutex[i], NULL);
	}
	experiment_global_state = (void *) mtm_global;

	/* populate the hash table */
	for(i=0;i<num_keys;i++) {
		{
			if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
				INTERNAL_ERROR("Could not allocate memory.\n")
			}
			obj->id = i;
			memcpy(obj->value, mtm_global->hashtable->tmp_buf, vsize); 
			HASH_ADD_INT(mtm_global->hashtable->ht, id, obj);
		}
	}

	return 0;
}

int add=0;
int del=0;

int mtm_fini(void)
{
	mtm_global_t              *mtm_global = (mtm_global_t *) experiment_global_state;
	object_t                  *iter;
	int                       count;

	for (count=0, iter=mtm_global->hashtable->ht; iter != NULL; count++, iter=iter->hh.next) {
	}
	printf("Adds: %d\n", add);
	printf("Dels: %d\n", del);
	printf("Count: %d\n", count);

	return 0;
}


typedef struct fixture_state_mtm_mix_s fixture_state_mtm_mix_t;
struct fixture_state_mtm_mix_s {
	unsigned int  seed;
	char          buf[16384];
};


void fixture_mtm_mix(void *arg)
{
 	ubench_args_t*            args = (ubench_args_t *) arg;
 	unsigned int              tid = args->tid;
	fixture_state_mtm_mix_t*  fixture_state;
	int                       i;
	mtm_global_t              *mtm_global = (mtm_global_t *) experiment_global_state;

	fixture_state = (fixture_state_mtm_mix_t*) malloc(sizeof(fixture_state_mtm_mix_t));

	args->fixture_state = (void *) fixture_state;
}


int random_operation(unsigned int *seedp)
{
	int n;

	n = rand_r(seedp) % 100;

	if (n < percent_put) {
		return OP_HASH_PUT;
	} else if (n < percent_put + percent_del) {
		return OP_HASH_DEL;
	} else {
		return OP_HASH_GET;
	}
}


static inline int mtm_op_add(unsigned int tid, mtm_hashtable_t *hashtable, int id, int lock)
{
	object_t     *obj;
	int          i;

	if (lock) {
#ifdef	_USE_RWLOCK
		pthread_rwlock_wrlock(&hashtable->rwlock);
#else		
		if (percent_get>0) {
			for (i=0; i<num_threads; i++) {
				pthread_mutex_lock(&hashtable->mutex[i]);
			}
		} else {
			pthread_mutex_lock(&hashtable->mutex[0]);
		}
#endif		
	}	
	MNEMOSYNE_ATOMIC 
	{
		if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
			INTERNAL_ERROR("Could not allocate memory.\n")
		}
		obj->id = id;
		memcpy(obj->value, hashtable->tmp_buf, vsize); 
		HASH_ADD_INT(hashtable->ht, id, obj);
	}
	if (lock) {
#ifdef	_USE_RWLOCK
		pthread_rwlock_unlock(&hashtable->rwlock);
#else
		if (percent_get>0) {
			for (i=0; i<num_threads; i++) {
				pthread_mutex_unlock(&hashtable->mutex[i]);
			}	
		} else {	
			pthread_mutex_unlock(&hashtable->mutex[0]);
		}	
#endif		
	}	

	return 0;
}


static inline int mtm_op_del(unsigned int tid, mtm_hashtable_t *hashtable, int id, int lock)
{
	object_t     *obj;
	int          local_id = id;
	int          i;

	if (lock) {
#ifdef	_USE_RWLOCK
		pthread_rwlock_wrlock(&hashtable->rwlock);
#else
		if (percent_get>0) {
			for (i=0; i<num_threads; i++) {
				pthread_mutex_lock(&hashtable->mutex[i]);
			}
		} else {
			pthread_mutex_lock(&hashtable->mutex[0]);
		}
#endif		
	}	
	MNEMOSYNE_ATOMIC 
	{
		HASH_FIND_INT(hashtable->ht, &local_id, obj);
		if (obj) {
			HASH_DEL(hashtable->ht, obj);
			pfree(obj);
		}
	}	
	if (lock) {
#ifdef	_USE_RWLOCK
		pthread_rwlock_unlock(&hashtable->rwlock);
#else		
		if (percent_get>0) {
			for (i=0; i<num_threads; i++) {
				pthread_mutex_unlock(&hashtable->mutex[i]);
			}	
		} else {
			pthread_mutex_unlock(&hashtable->mutex[0]);
		}
#endif		
	}	

	return 0;
}


static inline int mtm_op_get(unsigned int tid, mtm_hashtable_t *hashtable, int id, int lock)
{
	object_t     *obj;
	int          local_id = id;

	if (lock) {
#ifdef	_USE_RWLOCK
		pthread_rwlock_rdlock(&hashtable->rwlock);
#else		
		pthread_mutex_lock(&hashtable->mutex[tid]);
#endif		
	}	
	HASH_FIND_INT(hashtable->ht, &local_id, obj);
	if (lock) {
#ifdef	_USE_RWLOCK
		pthread_rwlock_unlock(&hashtable->rwlock);
#else		
		pthread_mutex_unlock(&hashtable->mutex[tid]);
#endif		
	}	

	return 0;
}


void mtm_mix(void *arg)
{
 	ubench_args_t*            args = (ubench_args_t *) arg;
 	unsigned int              tid = args->tid;
 	unsigned int              iterations_per_chunk = args->iterations_per_chunk;
	fixture_state_mtm_mix_t*  fixture_state = args->fixture_state;
	mtm_global_t              *mtm_global = (mtm_global_t *) experiment_global_state;
	unsigned int*             seedp = &fixture_state->seed;
	int                       i;
	int                       id;
	int                       op;


	/* 
	 * Before start wondering why the code is duplicated  below, here is 
	 * the explanation: it's better to check num_threads once and then set 
	 * the lock parameter of each individual operations as constant since 
	 * this will effectively allow the compiler to optimize out any 
	 * unnecessary branches during inlining.
	 */
	if (num_threads > 1) {
		for (i=0; i<iterations_per_chunk; i++) {
			op = random_operation(seedp);
			id = rand_r(seedp) % num_keys;
			switch(op) {
				case OP_HASH_PUT:
					add++;
					mtm_op_add(tid, mtm_global->hashtable, id, 1);
					break;
				case OP_HASH_DEL:
					del++;
					mtm_op_del(tid, mtm_global->hashtable, id, 1);
					break;
				case OP_HASH_GET:
					mtm_op_get(tid, mtm_global->hashtable, id, 1);
					break;
			}
		}
	} else {
		for (i=0; i<iterations_per_chunk; i++) {
			op = random_operation(seedp);
			id = rand_r(seedp) % num_keys;
			switch(op) {
				case OP_HASH_PUT:
					add++;
					mtm_op_add(tid, mtm_global->hashtable, id, 0);
					break;
				case OP_HASH_DEL:
					del++;
					mtm_op_del(tid, mtm_global->hashtable, id, 0);
					break;
				case OP_HASH_GET:
					mtm_op_get(tid, mtm_global->hashtable, id, 0);
					break;
			}
		}
	}
}


/* mix workload with thinking/sleeping time  */
void mtm_mix_think(void *arg)
{
 	ubench_args_t*            args = (ubench_args_t *) arg;
 	unsigned int              tid = args->tid;
 	unsigned int              iterations_per_chunk = args->iterations_per_chunk;
	fixture_state_mtm_mix_t*  fixture_state = args->fixture_state;
	mtm_global_t              *mtm_global = (mtm_global_t *) experiment_global_state;
	unsigned int*             seedp = &fixture_state->seed;
	int                       i;
	int                       j;
	int                       id;
	int                       op;
	int                       block = 128;
	struct timeval            start_time;
	struct timeval            stop_time;
	unsigned long long        work_time;    /* in usec */
	unsigned long long        think_time;      /* in usec */

	for (i=0; i<iterations_per_chunk; i+=block) {
		gettimeofday(&start_time, NULL);
		for (j=0; j<block; j++) {
			op = random_operation(seedp);
			id = rand_r(seedp) % num_keys;
			switch(op) {
				case OP_HASH_PUT:
					add++;
					mtm_op_add(tid, mtm_global->hashtable, id, 0);
					break;
				case OP_HASH_DEL:
					del++;
					mtm_op_del(tid, mtm_global->hashtable, id, 0);
					break;
				case OP_HASH_GET:
					mtm_op_get(tid, mtm_global->hashtable, id, 0);
					break;
			}
		}
		gettimeofday(&stop_time, NULL);
		work_time = 1000000 * (stop_time.tv_sec - start_time.tv_sec) +
	                              stop_time.tv_usec - start_time.tv_usec;
		think_time = ((100 - work_percent) * work_time) / work_percent;
		usleep(think_time);
	}
}


/* 
 * BDB MIX 
 */

int bdb_op_add(DB *dbp, DB_ENV *envp, int id, char *buf);

/* Open a Berkeley DB database */
int
open_db(DB **dbpp, const char *prog_name, const char *file_name,
        DB_ENV *envp, u_int32_t extra_flags)
{
	int       ret;
	u_int32_t open_flags;
	DB        *dbp;

    /* Initialize the DB handle */
	ret = db_create(&dbp, envp, 0);
	if (ret != 0) {
        fprintf(stderr, "%s: %s\n", prog_name, db_strerror(ret));
        return (EXIT_FAILURE);
    }

	/* Point to the memory malloc'd by db_create() */
	*dbpp = dbp;

	if (extra_flags != 0) {
		ret = dbp->set_flags(dbp, extra_flags);
		if (ret != 0) {
			dbp->err(dbp, ret, 
			         "open_db: Attempt to set extra flags failed.");
			return (EXIT_FAILURE);
		}
	}

	/* Now open the database */
	open_flags = DB_CREATE              | /* Allow database creation */ 
				 DB_AUTO_COMMIT;          /* Allow autocommit */
	ret = dbp->open(dbp,        /* Pointer to the database */
					NULL,       /* Txn pointer */
					file_name,  /* File name */
					NULL,       /* Logical db name */
					DB_HASH,    /* Database type (using btree) */
					open_flags, /* Open flags */
					0);         /* File mode. Using defaults */

	if (ret != 0) {
		dbp->err(dbp, ret, "Database '%s' open failed",
		         file_name);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}


typedef struct bdb_global_s {
	DB     *dbp;
	DB_ENV *envp;
	char   file_name[128];
} bdb_global_t;


int bdb_init()
{
	DB           *dbp = NULL;
	DB_ENV       *envp = NULL;
	bdb_global_t *bdb_global = NULL;
    int          ret;
	int          ret_t;
    u_int32_t    env_flags;
	char         buf[16384];
	int          i;
    const char   *file_name = "mydb.db";  /* Database file name */


	/* Create the environment */
	ret = db_env_create(&envp, 0);
	if (ret != 0) {
		fprintf(stderr, "Error creating environment handle: %s\n", db_strerror(ret));
		goto err;
	}

	/*
	 * Indicate that we want db to perform lock detection internally.
	 * Also indicate that the transaction with the fewest number of
	 * write locks will receive the deadlock notification in 
	 * the event of a deadlock.
	 */  
	ret = envp->set_lk_detect(envp, DB_LOCK_MINWRITE);
	if (ret != 0) {
		fprintf(stderr, "Error setting lock detect: %s\n",
			db_strerror(ret));
		goto err;
	}

	env_flags =
	  DB_CREATE     |  /* Create the environment if it does not exist */ 
	  DB_RECOVER    |  /* Run normal recovery. */
	  DB_INIT_LOG   |  /* Initialize the logging subsystem */
	  DB_INIT_TXN   |  /* Initialize the transactional subsystem. This
						* also turns on logging. */
	  DB_INIT_MPOOL |  /* Initialize the memory pool (in-memory cache) */
	  DB_THREAD;       /* Cause the environment to be free-threaded */

	if (num_threads > 1) {
		env_flags |=
		  DB_INIT_LOCK;  /* Initialize the locking subsystem */
	}
	ret = envp->set_cachesize(envp, 0, 128*1024*1024, 0);
	if (ret != 0) {
		goto err;
	}

	/* Now actually open the environment */
	ret = envp->open(envp, db_home_dir, env_flags, 0);
	if (ret != 0) {
		fprintf(stderr, "Error opening environment: %s\n", db_strerror(ret));
		goto err;
	}

	/*
	 * If we had utility threads (for running checkpoints or 
	 * deadlock detection, for example) we would spawn those
	 * here. However, for a simple example such as this,
	 * that is not required.
	 */

	/* Open the database */
	ret = open_db(&dbp, prog_name, file_name, envp, 0);
	if (ret != 0) {
		goto err;
	}

	bdb_global = (bdb_global_t *) malloc(sizeof(bdb_global_t));

	bdb_global->dbp = dbp;
	bdb_global->envp = envp;
	strcpy(bdb_global->file_name, file_name);
	experiment_global_state = (void *) bdb_global;

	/* populate the hash table */
	for(i=0;i<num_keys;i++) {
		bdb_op_add(dbp, envp, i, buf);
    }

	return 0;

err:
	return -1;
}

int bdb_fini()
{
	bdb_global_t *bdb_global;
    int          ret;
	int          ret_t;

	bdb_global = (bdb_global_t *) experiment_global_state;

    /* Close our database handle, if it was opened. */
    if (bdb_global->dbp != NULL) {
        ret_t = bdb_global->dbp->close(bdb_global->dbp, 0);
        if (ret_t != 0) {
            fprintf(stderr, "%s database close failed: %s\n",
                bdb_global->file_name, db_strerror(ret_t));
            ret = ret_t;
        }
    }

    /* Close our environment, if it was opened. */
    if (bdb_global->envp != NULL) {
        ret_t = bdb_global->envp->close(bdb_global->envp, 0);
        if (ret_t != 0) {
            fprintf(stderr, "environment close failed: %s\n", db_strerror(ret_t));
                ret = ret_t;
        }
    }

	free(experiment_global_state);

    /* Final status message and return. */
    printf("I'm all done.\n");
	return 0;
}

typedef struct fixture_state_bdb_mix_s fixture_state_bdb_mix_t;
struct fixture_state_bdb_mix_s {
	unsigned int  seed;
	DB            *dbp;
	DB_ENV        *envp;
	char          buf[16384];
};


void fixture_bdb_mix(void *arg)
{
 	ubench_args_t*            args = (ubench_args_t *) arg;
 	unsigned int              tid = args->tid;
	fixture_state_bdb_mix_t*  fixture_state;
	object_t                  *obj;
	object_t                  *iter;
	int                       i;
	bdb_global_t              *bdb_global = (bdb_global_t *) experiment_global_state;

	fixture_state = (fixture_state_bdb_mix_t*) malloc(sizeof(fixture_state_bdb_mix_t));

	fixture_state->dbp = bdb_global->dbp;
    fixture_state->envp = fixture_state->dbp->get_env(fixture_state->dbp);

	args->fixture_state = (void *) fixture_state;
}

int
bdb_op_add(DB *dbp, DB_ENV *envp, int id, char *buf)
{
    int  ret;
	int  ret_t;
    DBT  key;
	DBT  value;

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));
	memset(&value, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof(int);
	value.data = buf;
	value.size = vsize;


	/*
	 * Perform the database write. A txn handle is not provided, but the
	 * database support auto commit, so auto commit is used for the write.
	 */

	ret = dbp->put(dbp, NULL, &key, &value, 0);
	if (ret != 0) {
#ifdef _DEBUG_THIS	
		envp->err(envp, ret, "Database put failed.");
#endif		
		goto err;
	}
	return 0;

err:
	return -1;
}


int
bdb_op_del(DB *dbp, DB_ENV *envp, int id, char *buf)
{
    int  ret;
    DBT  key;

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof(int);

	ret = dbp->del(dbp, NULL, &key, 0);
	if (ret != 0) {
#ifdef _DEBUG_THIS	
		envp->err(envp, ret, "Database delete failed.");
#endif		
		goto err;
	}
	return 0;

err:
	return -1;
}


int
bdb_op_get(DB *dbp, DB_ENV *envp, int id, char *buf)
{
    int  ret;
    DBT  key;
    DBT  value;

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof(int);

	value.data = buf;
	value.ulen = vsize;
	value.flags = DB_DBT_USERMEM; 

	ret = dbp->get(dbp, NULL, &key, &value, 0);
	if (ret != 0) {
#ifdef _DEBUG_THIS	
		envp->err(envp, ret, "Database get failed.");
#endif		
		goto err;
	}
	return 0;

err:
	return -1;
}


void bdb_mix(void *arg)
{
 	ubench_args_t*            args = (ubench_args_t *) arg;
 	unsigned int              tid = args->tid;
 	unsigned int              iterations_per_chunk = args->iterations_per_chunk;
	fixture_state_bdb_mix_t*  fixture_state = args->fixture_state;
	unsigned int*             seedp = &fixture_state->seed;
	int                       i;
	int                       op;
	DB                        *dbp;
	DB_ENV                    *envp;
	int                       id;

	dbp = fixture_state->dbp;
	envp = fixture_state->envp;

	for (i=0; i<iterations_per_chunk; i++) {
    	op = random_operation(seedp);
		id = rand_r(seedp) % num_keys;
		switch(op) {
			case OP_HASH_PUT:
				bdb_op_add(dbp, envp, id, fixture_state->buf);
				break;
			case OP_HASH_DEL:
				bdb_op_del(dbp, envp, id, fixture_state->buf);
				break;
			case OP_HASH_GET:
				bdb_op_get(dbp, envp, id, fixture_state->buf);
				break;
		}
	}
}
