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
#include <mnemosyne.h>
#include <mtm.h>
#include <malloc.h>
#include <itm.h>
#include "ut_barrier.h"
#include "hashtable.h"
#include "elemset.h"
#include "mtm_mix.h"
#include "common.h"

//#define MNEMOSYNE_ATOMIC

typedef struct mtm_mix_stat_s {
	int      num_ins;
	int      num_del;
	int      num_get;
	uint64_t total_latency_cycles_ins;
	uint64_t total_latency_cycles_del;
	uint64_t total_latency_cycles_get;
	uint64_t work_transactions;
} mtm_mix_stat_t;


typedef struct mtm_mix_thread_state_s {
	unsigned int   seed;
	char           buf[16384];
	elemset_t      elemset_ins;        /* the set of the keys inserted in the table */
	elemset_t      elemset_del;        /* the set of the keys deleted from the table */
	int            keys_range_len;
	int            keys_range_min;
	int            keys_range_max;
	mtm_mix_stat_t stat;
} mtm_mix_thread_state_t;


typedef struct mtm_hashtable_s {
	struct hashtable *ht;
	char             tmp_buf[16384];
} mtm_hashtable_t;


typedef struct mtm_mix_state_s {
	mtm_hashtable_t *hashtable;
	mtm_mix_stat_t  stat;
} mtm_mix_state_t;


typedef struct object_s {
	unsigned int   key;
	char           value[0];
} object_t;

static int mtm_op_add(unsigned int tid, mtm_hashtable_t *hashtable, int id, int lock);


__attribute__((tm_callable))
void print_obj( void *obj )
{
	printf("[%u, ... ]\n", ((object_t *)obj)->key);
}

__attribute__((tm_callable))
static unsigned int hash_from_key_fn( void *obj )
{
	//printf("obj = %p", obj);
	//printf("\tkey = %u\n", ((object_t *)obj)->key);
	return ((object_t *)obj)->key;
}

__attribute__((tm_callable))
static int keys_equal_fn ( void *obj1, void *obj2 )
{
	//printf("equal1:obj1[%p, ... ]\n", ((object_t *)obj1));
	//printf("equal2:obj2[%p, ... ]\n", ((object_t *)obj2));
	//printf("equal1:[%u, ... ]\n", ((object_t *)obj1)->key);
	//printf("equal2:[%u, ... ]\n", ((object_t *)obj2)->key);
	if (((object_t *) obj1)->key == ((object_t *) obj2)->key) {
		return 1;
	}
	return 0;
}


int mtm_mix_init(void *args)
{
	mtm_mix_state_t *global_state = NULL;
	object_t        *obj;
	int             i;
	int             j;
	int             keys_range_min;
	int             keys_range_max;
	int             keys_range_len;

	global_state = (mtm_mix_state_t *) malloc(sizeof(mtm_mix_state_t));

	MNEMOSYNE_ATOMIC 
	{
		global_state->hashtable = (mtm_hashtable_t *) pmalloc(sizeof(mtm_hashtable_t));
	}
	//printf("hash_from_key_fn=%p\n",hash_from_key_fn);
	//printf("keys_equal_fn=%p\n",keys_equal_fn);
	global_state->hashtable->ht = create_hashtable((unsigned int)(2*(float) num_keys), hash_from_key_fn, keys_equal_fn);
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

#if 0
	keys_range_len = 2*num_keys / num_threads;
	for (i=0; i<num_threads; i++) {
		keys_range_min = i*keys_range_len;
		keys_range_max = (i+1)*keys_range_len-1;
		for(j=keys_range_min; j<=keys_range_max-(keys_range_len/2); j++) {
			if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
				INTERNAL_ERROR("Could not allocate memory.\n")
			}
			obj->key = j;
			memcpy(obj->value, global_state->hashtable->tmp_buf, vsize); 
			hashtable_insert(i, global_state->hashtable->ht, obj, obj);
		}
		for(j=keys_range_max-(keys_range_len/2); j<=keys_range_max; j++) {
			if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
				INTERNAL_ERROR("Could not allocate memory.\n")
			}
			obj->key = j;
			memcpy(obj->value, global_state->hashtable->tmp_buf, vsize); 
			hashtable_insert(i, global_state->hashtable->ht, obj, obj);
		}
		for(j=keys_range_max-(keys_range_len/2); j<=keys_range_max; j++) {
			object_t obj_key;
			obj_key.key = j;
			obj = hashtable_remove(i, global_state->hashtable->ht, &obj_key);
			pfree(obj);
		}

	}
#endif

	return 0;
}


int mtm_mix_fini(void *args)
{
	mtm_mix_state_t   *global_state = (mtm_mix_state_t *) ubench_desc.global_state;
	object_t          *iter;
	int               count;

	return 0;
}


int mtm_mix_thread_init(unsigned int tid)
{
	mtm_mix_thread_state_t    *thread_state;
	object_t                  *obj;
	object_t                  *iter;
	int                       i;
	int                       j;
	int                       num_keys_per_thread;
	mtm_mix_state_t           *global_state = (mtm_mix_state_t *) ubench_desc.global_state;
	mtm_mix_stat_t            *stat;
	int                       keys_range_min;
	int                       keys_range_max;
	int                       keys_range_len;


	thread_state = (mtm_mix_thread_state_t*) malloc(sizeof(mtm_mix_thread_state_t));
	ubench_desc.thread_state[tid] = thread_state;

	keys_range_len = thread_state->keys_range_len = num_keys_per_thread = (num_keys) / num_threads;
	keys_range_min = thread_state->keys_range_min = tid*num_keys_per_thread;
	keys_range_max = thread_state->keys_range_max = (tid+1)*num_keys_per_thread-1;

#ifdef USE_ELEMSET
	assert(elemset_init(&thread_state->elemset_ins, num_keys_per_thread, tid)==0);
	assert(elemset_init(&thread_state->elemset_del, num_keys_per_thread, tid)==0);

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

	for(j=keys_range_min; j<=keys_range_max-(keys_range_len/2); j++) {
		if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
			INTERNAL_ERROR("Could not allocate memory.\n")
		}
		obj->key = j;
		hashtable_insert(tid, global_state->hashtable->ht, obj, obj);
		elemset_put(&thread_state->elemset_ins, obj);
		//printf("put_ins: %p\n", obj);
		//printf("put_ins: key=%d\n", obj->key);
	}

	for(j=keys_range_max-(keys_range_len/2); j<=keys_range_max; j++) {
		if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
			INTERNAL_ERROR("Could not allocate memory.\n")
		}
		obj->key = j;
		elemset_put(&thread_state->elemset_del, obj);
		//printf("put_del: %p\n", obj);
		//printf("put_del: key=%d\n", obj->key);
	}

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


int mtm_mix_thread_fini(unsigned int tid)
{
	return 0;
}


void __mtm_mix_print_stats(FILE *fout, int include_latency) 
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
	uint64_t                hashtable_footprint;
	unsigned int            hashtable_num_entries;

	mtm_mix_state_t         *global_state = (mtm_mix_state_t *) ubench_desc.global_state;
	mtm_mix_stat_t          *thread_stat;
	mtm_mix_stat_t          total_stat;

	total_stat.num_ins = 0;
	total_stat.num_del = 0;
	total_stat.num_get = 0;
	total_stat.total_latency_cycles_ins = 0;
	total_stat.total_latency_cycles_del = 0;
	total_stat.total_latency_cycles_get = 0;
	for (i=0; i<num_threads; i++) {
		thread_stat = &(((mtm_mix_thread_state_t *) ubench_desc.thread_state[i])->stat);
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

	hashtable_num_entries= hashtable_count(global_state->hashtable->ht);
	hashtable_footprint  = hashtable_metadata_size(global_state->hashtable->ht);
	PRINT_NAMEVAL_U64(prefix, "hashtable_metadata_size", hashtable_footprint/1024, "KB", separator);
	hashtable_footprint += hashtable_num_entries * vsize;
	PRINT_NAMEVAL_U64(prefix, "hashtable_footprint", hashtable_footprint/1024, "KB", separator);
	PRINT_NAMEVAL_U64(prefix, "hashtable_count", hashtable_num_entries, "entries", separator);
}


void mtm_mix_latency_print_stats(FILE *fout) 
{
	__mtm_mix_print_stats(fout, 1);
}


void mtm_mix_throughput_print_stats(FILE *fout) 
{
	__mtm_mix_print_stats(fout, 0);
}


static inline 
int 
mtm_op_add(unsigned int tid, mtm_hashtable_t *hashtable, object_t *obj, int lock)
{
	unsigned int key = obj->key;

/*
	printf("add: %p\n", obj);
	printf("add: key=%d\n", obj->key);
	printf("add: obj->value=%p\n", obj->value);
	printf("add: vsize=%d\n", vsize);
*/	
	MNEMOSYNE_ATOMIC 
	{
		//print_obj(obj);
		//printf("mtm_op_add: key (via print_obj) = "); print_obj(obj);
		//printf("mtm_op_add: id                  = %u\n", id); 
		//printf("mtm_op_add: key                 = %u\n", obj->key); 
		//printf("mtm_op_add: key (via print_obj) = "); print_obj(obj);
		//printf("mtm_op_add: obj = %p\n", obj);
		memcpy(obj->value, hashtable->tmp_buf, vsize); 
		hashtable_insert(tid, hashtable->ht, obj, obj);
		//assert(hashtable_search(hashtable->ht, obj) != NULL);
	}
	assert(obj->key == key);

	return 0;
}


static inline 
int 
mtm_op_del(unsigned int tid, mtm_hashtable_t *hashtable, object_t *obj, int lock)
{
	object_t *obj_val;

/*
	printf("del: %p\n", obj);
	printf("del: key=%d\n", obj->key);
*/	
	MNEMOSYNE_ATOMIC 
	{
		obj_val = hashtable_remove(tid, hashtable->ht, obj);
	}	

	return 0;
}


static inline
int 
mtm_op_get(unsigned int tid, mtm_hashtable_t *hashtable, int id, int lock)
{
	object_t     obj;
	object_t     *obj_val;
	object_t     *obj_key = &obj;

	obj_key->key = id;
	obj_val = hashtable_search(tid, hashtable->ht, obj_key);
	if (obj_val) {
		//printf("mtm_op_get: %d: FOUND\n", id);
		return 1;
	}
		//printf("mtm_op_get: %d: NOT FOUND\n", id);
	return 0;
}


static inline
void __mtm_mix_thread_main(unsigned int tid, int measure_latency, int think)
{
 	unsigned int              iterations_per_chunk = ubench_desc.iterations_per_chunk;
	mtm_mix_thread_state_t*   thread_state = ubench_desc.thread_state[tid];
	unsigned int*             seedp = &thread_state->seed;
	int                       i;
	int                       j;
	int                       op;
	int                       id;
	object_t                  *del_obj;
	object_t                  *ins_obj;
	int                       found;
	int                       keys_range_min;
	int                       keys_range_max;
	int                       keys_range_len;
	int                       work_transactions = 0;
	uint64_t                  start;
	uint64_t                  stop; 
	struct timeval            work_start_time;
	struct timeval            work_stop_time;
	uint64_t                  work_time;   /* in usec */
	uint64_t                  think_time;  /* in usec */
	mtm_mix_stat_t           *stat = &thread_state->stat;
	mtm_mix_state_t          *global_state = (mtm_mix_state_t *) ubench_desc.global_state;
	int                      block = 16;


	keys_range_min = thread_state->keys_range_min;
	keys_range_max = thread_state->keys_range_max;
	keys_range_len = thread_state->keys_range_len;

	for (i=0; i<iterations_per_chunk; i+=block) {
		if (think) {
			gettimeofday(&work_start_time, NULL);
		}
		for (j=0; j<block; j++) {
			op = random_operation(seedp, percent_write);
			id = keys_range_min + rand_r(seedp) % keys_range_len;
			switch(op) {
				case OP_HASH_WRITE:
	#ifdef USE_ELEMSET
					elemset_get_random(&thread_state->elemset_ins, &del_obj);
					elemset_get_random(&thread_state->elemset_del, &ins_obj);
					elemset_put(&thread_state->elemset_ins, ins_obj);
					elemset_put(&thread_state->elemset_del, del_obj);
					if (measure_latency) {
						start = gethrtime();
					}
					assert(mtm_op_del(tid, global_state->hashtable, del_obj, 1) == 0);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_del += stop-start;
					}
					stat->num_del++;
					
					if (measure_latency) {
						start = gethrtime();
					}
					assert(mtm_op_add(tid, global_state->hashtable, ins_obj, 1) == 0);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_ins += stop-start;
					}
					stat->num_ins++;
					work_transactions += 2;
	#else
					/* Mnemosyne transactions don't support isolation so the two operations
					 * mtm_op_get and mtm_op_ins/mtm_op_del won't be executed atomically.
					 * Therefore we can't expect the ins/del to always succeed as some
					 * other thread might jump in and remove or insert the object.
					 */
					found = (mtm_op_get(tid, global_state->hashtable, id, 1) == 1) ? 1 : 0;
					stat->num_get++;
					if (found) {
						work_transactions++;
						stat->num_del++;
						if (measure_latency) {
							start = gethrtime();
						}
						assert(mtm_op_del(tid, global_state->hashtable, id, 1) == 0);
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
						assert(mtm_op_add(tid, global_state->hashtable, id, 1) == 0);
						if (measure_latency) {
							asm_cpuid();
							stop = gethrtime();
							stat->total_latency_cycles_ins += stop-start;
						}
					}
	#endif
					break;
				case OP_HASH_READ:
/*				
					if (measure_latency) {
						start = gethrtime();
					}
					mtm_op_get(tid, global_state->hashtable, id, 1);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_get += stop-start;
					}
					stat->num_get++;
					work_transactions++;
					break;
*/					
			}
		}	
		if (think) {
			gettimeofday(&work_stop_time, NULL);
		}
		work_time = 1000000 * (work_stop_time.tv_sec - work_start_time.tv_sec) +
	                              work_stop_time.tv_usec - work_start_time.tv_usec;
		think_time = ((100 - work_percent) * work_time) / work_percent;
		usleep(think_time);
	}
	stat->work_transactions += work_transactions;
}


void mtm_mix_latency_thread_main(unsigned int tid)
{
	__mtm_mix_thread_main(tid, 1, 0);
}

void mtm_mix_throughput_thread_main(unsigned int tid)
{
	__mtm_mix_thread_main(tid, 0, 0);
}

void mtm_mix_latency_think_thread_main(unsigned int tid)
{
	__mtm_mix_thread_main(tid, 1, 1);
}

void mtm_mix_throughput_think_thread_main(unsigned int tid)
{
	__mtm_mix_thread_main(tid, 0, 1);
}
