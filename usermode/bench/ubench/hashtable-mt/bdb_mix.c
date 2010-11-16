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
#include <db.h>
#include "keyset.h"
#include "bdb_mix.h"
#include "common.h"


static int bdb_op_add(DB *dbp, DB_ENV *envp, int id, char *buf);

typedef struct bdb_mix_stat_s {
	int      num_ins;
	int      num_del;
	int      num_get;
	uint64_t total_latency_cycles_ins;
	uint64_t total_latency_cycles_del;
	uint64_t total_latency_cycles_get;
	uint64_t work_transactions;
} bdb_mix_stat_t;


typedef struct bdb_mix_thread_state_s {
	unsigned int   seed;
	DB             *dbp;
	DB_ENV         *envp;
	char           buf[16384];
	keyset_t       keyset_ins;        /* the set of the keys inserted in the table */
	keyset_t       keyset_del;        /* the set of the keys deleted from the table */
	int            keys_range_len;
	int            keys_range_min;
	int            keys_range_max;
	bdb_mix_stat_t stat;
} bdb_mix_thread_state_t;


typedef struct bdb_mix_state_s {
	DB                 *dbp[MAX_NUM_THREADS];
	DB_ENV             *envp[MAX_NUM_THREADS];
	char               file_name[128];
	bdb_mix_stat_t     stat;
} bdb_mix_state_t;


typedef struct object_s {
	unsigned int   key;
	char           value[0];
} object_t;




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
	open_flags =  DB_CREATE;               /* Allow database creation */ 
	open_flags |= DB_AUTO_COMMIT;          /* Allow autocommit */

	//dbp->set_h_nelem(dbp, num_keys*100);
	//dbp->set_h_ffactor(dbp, 4);

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


int bdb_mix_init(void *args)
{
	DB              *dbp = NULL;
	DB_ENV          *envp = NULL;
	bdb_mix_state_t *global_state = NULL;
	int             ret;
	int             ret_t;
	u_int32_t       env_flags;
	char            buf[16384];
	int             i;
	int             j;
	const char      *file_name_prefix = "mydb.db";  /* Database file name */
	char            file_name[128];
	char            num_env;
	char            num_db_per_env;
	char            db_home_dir[128];
	int             keys_range_min;
	int             keys_range_max;
	int             keys_range_len;

#ifdef USE_PER_THREAD_HASH_TABLE	
	num_env = num_threads;
	num_db_per_env = 1;
#else
	num_env = 1;
	num_db_per_env = 1;
#endif

	global_state = (bdb_mix_state_t *) malloc(sizeof(bdb_mix_state_t));

	env_flags =
	  DB_CREATE     |  /* Create the environment if it does not exist */ 
	  DB_RECOVER    |  /* Run normal recovery. */
	  DB_INIT_LOG   |  /* Initialize the logging subsystem */
	  DB_INIT_TXN   |  /* Initialize the transactional subsystem. This
	                    * also turns on logging. */
	  DB_INIT_MPOOL |  /* Initialize the memory pool (in-memory cache) */
	  DB_THREAD     ;  /* Cause the environment to be free-threaded */

	env_flags |=  DB_INIT_LOCK;    /* Initialize the locking subsystem */

	for (i=0; i<num_env; i++) {

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

		ret = envp->set_cachesize(envp, 0, 512*1024*1024, 0);
		if (ret != 0) {
			goto err;
		}

		/* Now actually open the environment */
		sprintf(db_home_dir, "%s/db.%d", db_home_dir_prefix, i);
		mkdir(db_home_dir, S_IRWXU);
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
		sprintf(file_name, "%s.%d", file_name_prefix, i);
		ret = open_db(&dbp, prog_name, file_name, envp, 0);
		if (ret != 0) {
			fprintf(stderr, "Error opening database: %s\n", db_strerror(ret));
			goto err;
		}

		global_state->dbp[i] = dbp;
		global_state->envp[i] = envp;
	}

	strcpy(global_state->file_name, file_name);
	ubench_desc.global_state = (void *) global_state;

	/* Populate the hash table
	 * 
	 * We want to have a steady state experiment where deletes have the
	 * same rate as inserts, and the number of elements in the table is
	 * always num_keys.
	 *
	 * To achieve this, we have a range of keys [0, 2*num_keys-1], and 
	 * only half of the keys are found in the hash table at anytime during
	 * the experiment's execution.
	 */

	keys_range_len = 2*num_keys / num_threads;
	for (i=0; i<num_threads; i++) {
		keys_range_min = i*keys_range_len;
		keys_range_max = (i+1)*keys_range_len-1;
		for(j=keys_range_min; j<=keys_range_max-(keys_range_len/2); j++) {
			bdb_op_add(dbp, envp, j, buf);
		}
	}

	return 0;

err:
	return -1;
}


int bdb_mix_fini(void *args)
{
	bdb_mix_state_t *global_state;
	int          ret;
	int          ret_t;
	int          i;
	int          num_env;
	int          num_db_per_env;

	global_state = (bdb_mix_state_t *) ubench_desc.global_state;

	/* Close our database handle, if it was opened. */
#ifdef USE_PER_THREAD_HASH_TABLE	
	num_env = num_threads;
	num_db_per_env = 1;
#else
	num_env = 1;
	num_db_per_env = 1;
#endif
	for (i=0; i<num_env; i++) {
		if (global_state->dbp[i] != NULL) {
			ret_t = global_state->dbp[i]->close(global_state->dbp[i], 0);
			if (ret_t != 0) {
				fprintf(stderr, "%s database close failed: %s\n",
					global_state->file_name, db_strerror(ret_t));
				ret = ret_t;
			}
		}

		//global_state->envp[i]->mutex_stat_print(global_state->envp[i], DB_STAT_ALL);

		/* Close our environment, if it was opened. */
		if (global_state->envp[i] != NULL) {
			ret_t = global_state->envp[i]->close(global_state->envp[i], 0);
			if (ret_t != 0) {
				fprintf(stderr, "environment close failed: %s\n", db_strerror(ret_t));
				ret = ret_t;
			}
		}
	}

	return 0;
}


int bdb_mix_thread_init(unsigned int tid)
{
	bdb_mix_thread_state_t    *thread_state;
	object_t                  *obj;
	object_t                  *iter;
	int                       i;
	int                       num_keys_per_thread;
	bdb_mix_state_t           *global_state = (bdb_mix_state_t *) ubench_desc.global_state;
	bdb_mix_stat_t            *stat;

	thread_state = (bdb_mix_thread_state_t*) malloc(sizeof(bdb_mix_thread_state_t));
	ubench_desc.thread_state[tid] = thread_state;

#ifdef USE_PER_THREAD_HASH_TABLE	
	thread_state->dbp = global_state->dbp[tid]; /* use per thread database */
#else	
	thread_state->dbp = global_state->dbp[0];   /* use common database */
#endif	
	thread_state->envp = thread_state->dbp->get_env(thread_state->dbp);

	thread_state->keys_range_len = num_keys_per_thread = (2*num_keys) / num_threads;
	thread_state->keys_range_min = tid*num_keys_per_thread;
	thread_state->keys_range_max = (tid+1)*num_keys_per_thread-1;

#ifdef USE_KEYSET
	assert(keyset_init(&thread_state->keyset_ins, num_keys_per_thread, tid)==0);
	assert(keyset_init(&thread_state->keyset_del, num_keys_per_thread, tid)==0);
	assert(keyset_fill(&thread_state->keyset_ins, thread_state->keys_range_min, thread_state->keys_range_max - (num_keys_per_thread/2))==0);
	assert(keyset_fill(&thread_state->keyset_del, thread_state->keys_range_max-(num_keys_per_thread/2)+1, thread_state->keys_range_max)==0);
#endif	
	stat = &(thread_state->stat);
	stat->work_transactions = 0;
	stat->num_ins = 0;
	stat->num_del = 0;
	stat->num_get = 0;
	stat->total_latency_cycles_ins = 0;
	stat->total_latency_cycles_del = 0;
	stat->total_latency_cycles_get = 0;

	return 0;
}

int bdb_mix_thread_fini(unsigned int tid)
{
	return 0;
}



void __bdb_mix_print_stats(FILE *fout, int include_latency) 
{
	int                     i;
	uint64_t                total_work_transactions = 0;
	uint64_t                total_runtime = 0;
	uint64_t                avg_runtime;
	uint64_t                avg_latency_ins;
	uint64_t                avg_latency_get;
	uint64_t                avg_latency_del;
	double                  throughput;
	double                  throughput_ins;
	double                  throughput_del;
	double                  throughput_get;
	char                    prefix[] = "bench.stat";
	char                    separator[] = "rj,40";

	bdb_mix_state_t         *global_state = (bdb_mix_state_t *) ubench_desc.global_state;
	bdb_mix_stat_t          *thread_stat;
	bdb_mix_stat_t          total_stat;

	total_stat.num_ins = 0;
	total_stat.num_del = 0;
	total_stat.num_get = 0;
	total_stat.total_latency_cycles_ins = 0;
	total_stat.total_latency_cycles_del = 0;
	total_stat.total_latency_cycles_get = 0;
	for (i=0; i<num_threads; i++) {
		thread_stat = &(((bdb_mix_thread_state_t *) ubench_desc.thread_state[i])->stat);
		total_work_transactions += thread_stat->work_transactions;
		total_runtime += ubench_desc.thread_runtime[i];
		total_stat.num_ins += thread_stat->num_ins;
		total_stat.num_del += thread_stat->num_del;
		total_stat.num_get += thread_stat->num_get;
		total_stat.total_latency_cycles_ins += thread_stat->total_latency_cycles_ins;
		total_stat.total_latency_cycles_del += thread_stat->total_latency_cycles_del;
		total_stat.total_latency_cycles_get += thread_stat->total_latency_cycles_get;
	}
	avg_runtime = total_runtime/num_threads;
	throughput = ((double) total_work_transactions) / ((double) avg_runtime);
	throughput_ins = ((double) total_stat.num_ins) / ((double) avg_runtime);
	throughput_del = ((double) total_stat.num_del) / ((double) avg_runtime);
	throughput_get = ((double) total_stat.num_get) / ((double) avg_runtime);
	avg_latency_ins = CYCLE2NS(mydiv(total_stat.total_latency_cycles_ins, total_stat.num_ins));
	avg_latency_del = CYCLE2NS(mydiv(total_stat.total_latency_cycles_del, total_stat.num_del));
	avg_latency_get = CYCLE2NS(mydiv(total_stat.total_latency_cycles_get, total_stat.num_get));

	fprintf(fout, "STATISTICS\n");
	fprintf(fout, "%s\n", EQUALSIGNS(60));
	PRINT_NAMEVAL_U64(prefix, "runtime", avg_runtime/1000, "ms", separator);
	PRINT_NAMEVAL_U64(prefix, "total_work_transactions", total_work_transactions, "ops", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput", throughput * 1000 * 1000, "ops/s", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput_ins", throughput_ins * 1000 * 1000, "ops/s", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput_del", throughput_del * 1000 * 1000, "ops/s", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput_get", throughput_get * 1000 * 1000, "ops/s", separator);
	if (include_latency) {
		PRINT_NAMEVAL_U64(prefix, "avg_latency_ins", avg_latency_ins, "ns", separator);
		PRINT_NAMEVAL_U64(prefix, "avg_latency_del", avg_latency_del, "ns", separator);
		PRINT_NAMEVAL_U64(prefix, "avg_latency_get", avg_latency_get, "ns", separator);
	}

}

void bdb_mix_latency_print_stats(FILE *fout) 
{
	__bdb_mix_print_stats(fout, 1);
}

void bdb_mix_throughput_print_stats(FILE *fout) 
{
	__bdb_mix_print_stats(fout, 0);
}

static int
bdb_op_add(DB *dbp, DB_ENV *envp, int id, char *val)
{
	int  ret;
	int  ret_t;
	DBT  key;
	DBT  value;
	char buf[16384];
	int  retry_count = 0;

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));
	memset(&value, 0, sizeof(DBT));
	key.data = &id;
	key.size = sizeof(int);
	value.data = buf;
	value.size = vsize;
	memcpy(buf, val, vsize); 


	/*
	 * Perform the database write. A txn handle is not provided, but the
	 * database support auto commit, so auto commit is used for the write.
	 */

	retry_count = 0;
retry:
	ret = dbp->put(dbp, NULL, &key, &value, 0);
	if (ret == DB_LOCK_DEADLOCK) {
		retry_count++;
		goto retry;
	}

	if (ret != 0) {
		envp->err(envp, ret, "Database put failed.");
#ifdef _DEBUG_THIS	
		envp->err(envp, ret, "Database put failed.");
#endif		
		goto err;
	}
	return 0;

err:
	return -1;
}


static int
bdb_op_del(DB *dbp, DB_ENV *envp, int id, char *buf)
{
	int  ret;
	DBT  key;
	int  retry_count = 0;

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof(int);

	retry_count = 0;
retry:
	ret = dbp->del(dbp, NULL, &key, 0);
	if (ret == DB_LOCK_DEADLOCK) {
		retry_count++;
		goto retry;
	}

	if (ret != 0) {
		envp->err(envp, ret, "Database delete failed.");
#ifdef _DEBUG_THIS	
		envp->err(envp, ret, "Database delete failed.");
#endif		
		goto err;
	}
	return 0;

err:
	return -1;
}


static int
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


static inline
void __bdb_mix_thread_main(unsigned int tid, int measure_latency)
{
 	unsigned int              iterations_per_chunk = ubench_desc.iterations_per_chunk;
	bdb_mix_thread_state_t*   thread_state = ubench_desc.thread_state[tid];
	unsigned int*             seedp = &thread_state->seed;
	int                       i;
	int                       op;
	DB                        *dbp;
	DB_ENV                    *envp;
	int                       id;
	int                       del_id;
	int                       ins_id;
	int                       found;
	int                       keys_range_min;
	int                       keys_range_max;
	int                       keys_range_len;
	int                       work_transactions = 0;
	uint64_t                  start;
	uint64_t                  stop; 
	bdb_mix_stat_t           *stat = &thread_state->stat;

	dbp = thread_state->dbp;
	envp = thread_state->envp;


	keys_range_min = thread_state->keys_range_min;
	keys_range_max = thread_state->keys_range_max;
	keys_range_len = thread_state->keys_range_len;

	for (i=0; i<iterations_per_chunk; i++) {
		op = random_operation(seedp, percent_write);
		id = keys_range_min + rand_r(seedp) % keys_range_len;
		switch(op) {
			case OP_HASH_WRITE:
#ifdef USE_KEYSET
				keyset_get_key_random(&thread_state->keyset_ins, &del_id);
				keyset_get_key_random(&thread_state->keyset_del, &ins_id);
				keyset_put_key(&thread_state->keyset_ins, ins_id);
				keyset_put_key(&thread_state->keyset_del, del_id);
				if (measure_latency) {
					start = gethrtime();
				}
				assert(bdb_op_del(dbp, envp, del_id, thread_state->buf) == 0);
				if (measure_latency) {
					asm_cpuid();
					stop = gethrtime();
					stat->total_latency_cycles_del += stop-start;
				}
				stat->num_del++;
				
				if (measure_latency) {
					start = gethrtime();
				}
				assert(bdb_op_add(dbp, envp, ins_id, thread_state->buf) == 0);
				if (measure_latency) {
					asm_cpuid();
					stop = gethrtime();
					stat->total_latency_cycles_ins += stop-start;
				}
				stat->num_ins++;
				work_transactions += 2;
#else
				found = (bdb_op_get(dbp, envp, id, thread_state->buf) == 0) ? 1 : 0;
				if (found) {
					work_transactions++;
					stat->num_del++;
					if (measure_latency) {
						start = gethrtime();
					}
					assert(bdb_op_del(dbp, envp, id, thread_state->buf) == 0);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_del += stop-start;
					}
				} else {
					stat->num_ins++;
					work_transactions++;
					if (measure_latency) {
						start = gethrtime();
					}
					assert(bdb_op_add(dbp, envp, id, thread_state->buf) == 0); 
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_ins += stop-start;
					}
				}
#endif
				break;
			case OP_HASH_READ:
				if (measure_latency) {
					start = gethrtime();
				}
				bdb_op_get(dbp, envp, id, thread_state->buf);
				if (measure_latency) {
					asm_cpuid();
					stop = gethrtime();
					stat->total_latency_cycles_get += stop-start;
				}
				stat->num_get++;
				work_transactions++;
				break;
		}
	}
	stat->work_transactions += work_transactions;
}


void bdb_mix_latency_thread_main(unsigned int tid)
{
	__bdb_mix_thread_main(tid, 1);
}

void bdb_mix_throughput_thread_main(unsigned int tid)
{
	__bdb_mix_thread_main(tid, 0);
}
