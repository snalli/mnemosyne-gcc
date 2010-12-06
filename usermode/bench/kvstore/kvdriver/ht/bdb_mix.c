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
#include "mix-bits.h"
#include "elemset.h"
#include "bdb_mix.h"
#include "common.h"

char   *db_home_dir_prefix = "/mnt/pcmfs";


typedef struct bdb_mix_global_state_s {
	DB          *dbp;
	DB_ENV      *envp;
	char        file_name[128];
} bdb_mix_global_state_t;


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
	DB                     *dbp = NULL;
	DB_ENV                 *envp = NULL;
	mix_global_state_t     *mix_global_state = NULL;
	bdb_mix_global_state_t *global_state = NULL;
	int                    ret;
	int                    ret_t;
	u_int32_t              env_flags;
	char                   buf[16384];
	int                    j;
	const char             *file_name_prefix = "mydb.db";  /* Database file name */
	char                   file_name[128];
	char                   num_env;
	char                   num_db_per_env;
	char                   db_home_dir[128];

	global_state = (bdb_mix_global_state_t *) malloc(sizeof(bdb_mix_global_state_t));
	mix_init(&mix_global_state, bdb_op_ins, bdb_op_del, bdb_op_get, (void *) global_state);

	env_flags =
	  DB_CREATE     |  /* Create the environment if it does not exist */ 
	  DB_RECOVER    |  /* Run normal recovery. */
	  DB_INIT_LOG   |  /* Initialize the logging subsystem */
	  DB_INIT_TXN   |  /* Initialize the transactional subsystem. This
	                    * also turns on logging. */
	  DB_INIT_MPOOL |  /* Initialize the memory pool (in-memory cache) */
	  DB_THREAD     ;  /* Cause the environment to be free-threaded */

	env_flags |=  DB_INIT_LOCK;    /* Initialize the locking subsystem */

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

	ret = envp->set_lk_max_locks(envp, 10000);
	if (ret != 0) {
		goto err;
	}
	ret = envp->set_lk_max_objects(envp, 10000);
	if (ret != 0) {
		goto err;
	}

	ret = envp->set_cachesize(envp, 0, 1024*1024*1024, 0);
	if (ret != 0) {
		goto err;
	}

	/* Now actually open the environment */
	sprintf(db_home_dir, "%s/db", db_home_dir_prefix);
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
	sprintf(file_name, "%s", file_name_prefix);
	ret = open_db(&dbp, prog_name, file_name, envp, 0);
	if (ret != 0) {
		fprintf(stderr, "Error opening database: %s\n", db_strerror(ret));
		goto err;
	}

	global_state->dbp = dbp;
	global_state->envp = envp;

	strcpy(global_state->file_name, file_name);

	return 0;

err:
	return -1;
}


int bdb_mix_fini(void *args)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	bdb_mix_global_state_t *global_state = (bdb_mix_global_state_t *) mix_global_state->data;
	int                    ret;
	int                    ret_t;

	/* Close our database handle, if it was opened. */

	ret_t = global_state->dbp->close(global_state->dbp, 0);
	if (ret_t != 0) {
		fprintf(stderr, "%s database close failed: %s\n",
			global_state->file_name, db_strerror(ret_t));
		ret = ret_t;
	}

	/* Close our environment, if it was opened. */
	ret_t = global_state->envp->close(global_state->envp, 0);
	if (ret_t != 0) {
		fprintf(stderr, "environment close failed: %s\n", db_strerror(ret_t));
		ret = ret_t;
	}

	return 0;
}


int 
bdb_mix_create_elem(int id, void **elemp)
{
	object_t *obj;

	if ((obj = (object_t*) malloc(sizeof(object_t) + vsize)) == NULL) {
		INTERNAL_ERROR("Could not allocate memory.\n")
	}
	obj->key = id;
	*elemp = (void *) obj;

	return 0;
}



int
bdb_mix_thread_init(unsigned int tid)
{
	mix_global_state_t     *mix_global_state;
	bdb_mix_global_state_t *global_state;
	mix_thread_state_t     *thread_state;
	void                   *elem;
	int                    i;
	int                    j;
	int                    num_keys_per_thread;
	mix_stat_t             *stat;

	assert(mix_thread_init(tid, NULL, bdb_mix_create_elem, &thread_state)==0);

	mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	global_state = (bdb_mix_global_state_t *) mix_global_state->data;

	/* Populate the hash table */

	for (i=0; i<thread_state->elemset_ins.count; i++) {
		elem = thread_state->elemset_ins.array[i];
		bdb_op_ins(thread_state, elem, 0);
	}

	return 0;
}


int bdb_mix_thread_fini(unsigned int tid)
{
	return 0;
}


static inline
void 
__bdb_mix_print_stats(FILE *fout, int report_latency) 
{
	char                    prefix[] = "bench.stat";
	char                    separator[] = "rj,40";
	uint64_t                hashtable_footprint;
	unsigned int            hashtable_num_entries;

	mix_print_stats(fout, report_latency);
}


void bdb_mix_latency_print_stats(FILE *fout) 
{
	__bdb_mix_print_stats(fout, 1);
}


void bdb_mix_throughput_print_stats(FILE *fout) 
{
	__bdb_mix_print_stats(fout, 0);
}


int
bdb_op_ins(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	bdb_mix_global_state_t *global_state = (bdb_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	DB_ENV                 *envp = global_state->envp;
	DB                     *dbp = global_state->dbp;
	object_t               *obj = (object_t *) elem;
	int                    ret;
	int                    ret_t;
	DBT                    key;
	DBT                    value;
	char                   buf[16384];
	int                    retry_count = 0;
	unsigned int           id = obj->key; 

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));
	memset(&value, 0, sizeof(DBT));
	key.data = &id;
	key.size = sizeof(unsigned int);
	value.data = buf;
	value.size = vsize;


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
		goto err;
	}
	return 0;

err:
	return -1;
}


int
bdb_op_del(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	bdb_mix_global_state_t *global_state = (bdb_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	DB_ENV                 *envp = global_state->envp;
	DB                     *dbp = global_state->dbp;
	object_t               *obj = (object_t *) elem;
	int                    ret;
	DBT                    key;
	int                    retry_count = 0;
	unsigned int           id = obj->key; 

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof(unsigned int);

	retry_count = 0;
retry:
	ret = dbp->del(dbp, NULL, &key, 0);
	if (ret == DB_LOCK_DEADLOCK) {
		retry_count++;
		goto retry;
	}

	if (ret != 0) {
		envp->err(envp, ret, "Database delete failed.");
		goto err;
	}
	return 0;

err:
	return -1;
}


int
bdb_op_get(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	bdb_mix_global_state_t *global_state = (bdb_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	DB_ENV                 *envp = global_state->envp;
	DB                     *dbp = global_state->dbp;
	object_t               *obj = (object_t *) elem;
	int                    ret;
	DBT                    key;
	DBT                    value;
	int                    retry_count = 0;
	unsigned int           id = obj->key; 
	char                   buf[16384];

	/* Prepare the DBTs */
	memset(&key, 0, sizeof(DBT));

	key.data = &id;
	key.size = sizeof(unsigned int);

	value.data = buf;
	value.ulen = vsize;
	value.flags = DB_DBT_USERMEM; 

retry:
	ret = dbp->get(dbp, NULL, &key, &value, 0);
	if (ret == DB_LOCK_DEADLOCK) {
		retry_count++;
		goto retry;
	}

	if (ret != 0) {
		envp->err(envp, ret, "Database get failed.");
		goto err;
	}
	return 0;

err:
	return -1;
}


void bdb_mix_latency_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 0, 0);
}

void bdb_mix_throughput_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 0, 0);
}
