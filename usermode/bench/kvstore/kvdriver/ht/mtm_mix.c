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
#include "mix-bits.h"
#include "hashtable.h"
#include "elemset.h"
#include "mtm_mix.h"
#include "common.h"

//#define MNEMOSYNE_ATOMIC

typedef struct mtm_hashtable_s {
	struct hashtable *ht;
	char             tmp_buf[16384];
} mtm_hashtable_t;


typedef struct mtm_mix_global_state_s {
	mtm_hashtable_t *hashtable;
} mtm_mix_global_state_t;



typedef struct object_s {
	unsigned int   key;
	char           value[0];
} object_t;


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
	mix_global_state_t     *mix_global_state = NULL;
	mtm_mix_global_state_t *global_state = NULL;
	object_t               *obj;
	int                    i;
	int                    j;
	mtm_hashtable_t        *hashtable;
	char                   *ubench_name;

	/* 
	 * We need to enable transactional isolation if the experiment 
	 * does not use locks. 
	 * Since MTM has not been initialized yet, it is okay to do it
	 * via the environment variable. 
	 */
	ubench_name = ubench_index2str(ubench_to_run);
	if (strcmp(ubench_name, "mtm_mix_latency_etl") == 0 ||
	    strcmp(ubench_name, "mtm_mix_throughput_etl") == 0)
	{
		setenv("MTM_FORCE_MODE", "pwbetl", 1);
	}

	global_state = (mtm_mix_global_state_t *) malloc(sizeof(mtm_mix_global_state_t));
	mix_init(&mix_global_state, mtm_op_ins, mtm_op_del, mtm_op_get, (void *) global_state);

	MNEMOSYNE_ATOMIC 
	{
		global_state->hashtable = (mtm_hashtable_t *) pmalloc(sizeof(mtm_hashtable_t));
	}
	global_state->hashtable->ht = create_hashtable((unsigned int)(2*(float) num_keys), hash_from_key_fn, keys_equal_fn);

	return 0;
}


int mtm_mix_fini(void *args)
{
	return 0;
}


int 
mtm_mix_create_elem(int id, void **elemp)
{
	object_t *obj;

	if ((obj = (object_t*) pmalloc(sizeof(object_t) + vsize)) == NULL) {
		INTERNAL_ERROR("Could not allocate memory.\n")
	}
	obj->key = id;
	*elemp = (void *) obj;

	return 0;
}


int
mtm_mix_thread_init(unsigned int tid)
{
	mix_global_state_t     *mix_global_state;
	mtm_mix_global_state_t *global_state;
	mix_thread_state_t     *thread_state;
	object_t               *obj;
	object_t               *iter;
	int                    i;
	int                    j;
	int                    num_keys_per_thread;
	mix_stat_t             *stat;

	assert(mix_thread_init(tid, NULL, mtm_mix_create_elem, &thread_state)==0);

	mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	global_state = (mtm_mix_global_state_t *) mix_global_state->data;

	/* Populate the hash table */

	for (i=0; i<thread_state->elemset_ins.count; i++) {
		obj = (object_t *) thread_state->elemset_ins.array[i];
		hashtable_insert(tid, global_state->hashtable->ht, obj, obj);
	}

	return 0;
}


int mtm_mix_thread_fini(unsigned int tid)
{
	return 0;
}


static inline
void 
__mtm_mix_print_stats(FILE *fout, int report_latency) 
{
	char                    prefix[] = "bench.stat";
	char                    separator[] = "rj,40";
	uint64_t                hashtable_footprint;
	unsigned int            hashtable_num_entries;

	mix_print_stats(fout, report_latency);

	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	mtm_mix_global_state_t *global_state = (mtm_mix_global_state_t *) mix_global_state->data;

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


int 
mtm_op_ins(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	mtm_mix_global_state_t *global_state = (mtm_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	mtm_hashtable_t        *hashtable = global_state->hashtable;
	object_t               *obj = (object_t *) elem;
	unsigned int           tid = thread_state->tid;
	unsigned int           key = obj->key;

/*
	printf("add: %p\n", obj);
	printf("add: key=%d\n", obj->key);
	printf("add: obj->value=%p\n", obj->value);
	printf("add: vsize=%d\n", vsize);
*/	
	if (lock) {
		MNEMOSYNE_ATOMIC 
		{
			//print_obj(obj);
			//printf("mtm_op_ins: key (via print_obj) = "); print_obj(obj);
			//printf("mtm_op_ins: id                  = %u\n", id); 
			//printf("mtm_op_ins: key                 = %u\n", obj->key); 
			//printf("mtm_op_ins: key (via print_obj) = "); print_obj(obj);
			//printf("mtm_op_ins: obj = %p\n", obj);
			memcpy(obj->value, hashtable->tmp_buf, vsize); 
			hashtable_insert(tid, hashtable->ht, obj, obj);
			//assert(hashtable_search(hashtable->ht, obj) != NULL);
		}
	} else {
		MNEMOSYNE_ATOMIC 
		{
			memcpy(obj->value, hashtable->tmp_buf, vsize); 
			hashtable_insert_nolock(tid, hashtable->ht, obj, obj);
		}
	}
	assert(obj->key == key);

	return 0;
}


int 
mtm_op_del(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	mtm_mix_global_state_t *global_state = (mtm_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	mtm_hashtable_t        *hashtable = global_state->hashtable;
	unsigned int           tid = thread_state->tid;
	object_t               *obj = (object_t *) elem;
	object_t               *obj_val;

/*
	printf("del: %p\n", obj);
	printf("del: key=%d\n", obj->key);
*/	
	if (lock) {
		MNEMOSYNE_ATOMIC 
		{
			obj_val = hashtable_remove(tid, hashtable->ht, obj);
		}	
	} else {
		MNEMOSYNE_ATOMIC 
		{
			obj_val = hashtable_remove_nolock(tid, hashtable->ht, obj);
		}	
	}

	return 0;
}


int 
mtm_op_get(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	mtm_mix_global_state_t *global_state = (mtm_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	mtm_hashtable_t        *hashtable = global_state->hashtable;
	unsigned int           tid = thread_state->tid;
	object_t               *obj = (object_t *) elem;
	object_t               *obj_val;

	if (lock) {
		obj_val = hashtable_search(tid, hashtable->ht, obj);
	} else {	
		MNEMOSYNE_ATOMIC {
			obj_val = hashtable_search_nolock(tid, hashtable->ht, obj);
		}
	}	
	if (obj_val) {
		return 0;
	}
	return -1;
}


void mtm_mix_latency_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 0, 1);
}

void mtm_mix_throughput_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 0, 1);
}

void mtm_mix_latency_think_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 1, 1);
}

void mtm_mix_throughput_think_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 1, 1);
}

/* etl versions rely on memory transactions for isolation */
void mtm_mix_latency_thread_main_etl(unsigned int tid)
{
	mix_thread_main(tid, 1, 0, 0);
}

void mtm_mix_throughput_thread_main_etl(unsigned int tid)
{
	mix_thread_main(tid, 0, 0, 0);
}
