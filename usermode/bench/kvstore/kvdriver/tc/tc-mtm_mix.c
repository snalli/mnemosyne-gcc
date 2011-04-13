#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <sched.h>
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
#include <itm.h>
#include "tcutil.h"
#include "tcbdb.h"
#include "ut_barrier.h"
#include "mix-bits.h"
#include "hashtable.h"
#include "elemset.h"
#include "tc_mix.h"
#include "common.h"
#include "tc_mix-bits.h"


pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

int tc_mtm_mix_init(void *args)
{
	mix_global_state_t     *mix_global_state = NULL;
	tc_mix_global_state_t  *global_state = NULL;

	global_state = (tc_mix_global_state_t *) malloc(sizeof(tc_mix_global_state_t));
	mix_init(&mix_global_state, tc_mtm_op_ins, tc_mtm_op_del, tc_mtm_op_get, (void *) global_state);

	return tc_mix_init_internal(global_state, 1);
}


int tc_mtm_mix_init_etl(void *args)
{
	mix_global_state_t     *mix_global_state = NULL;
	tc_mix_global_state_t  *global_state = NULL;

	global_state = (tc_mix_global_state_t *) malloc(sizeof(tc_mix_global_state_t));
	mix_init(&mix_global_state, tc_mtm_op_ins, tc_mtm_op_del, tc_mtm_op_get, (void *) global_state);

	return tc_mix_init_internal(global_state, 0);
}



int 
tc_mtm_op_ins(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	tc_mix_global_state_t  *global_state = (tc_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	TCBDB                  *bdb=global_state->bdb;
	object_t               *obj = (object_t *) elem;
	bool                   result;
	int                    key = obj->key;

	MNEMOSYNE_ATOMIC {
		//printf("[%d] INSERT %d\n", thread_state->tid, obj->key);
		if (tcbdbput(bdb, &(obj->key), sizeof(int), &(obj->value), vsize) == false) {
			return 0;
			return -1;
		}
	}

	return 0;
}


int 
tc_mtm_op_del(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	tc_mix_global_state_t  *global_state = (tc_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	TCBDB                  *bdb=global_state->bdb;
	object_t               *obj = (object_t *) elem;

	MNEMOSYNE_ATOMIC {
		//printf("[%d] REMOVE %p\n", thread_state->tid, obj);
		//printf("[%d] REMOVE %d\n", thread_state->tid, obj->key);
		if (tcbdbout(bdb, &(obj->key), sizeof(int)) == false) {
			return 0;
			return -1;
		}
	}

	return 0;
}


int 
tc_mtm_op_get(void *t, void *elem, int lock)
{
	mix_global_state_t     *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	tc_mix_global_state_t  *global_state = (tc_mix_global_state_t *) mix_global_state->data;
	mix_thread_state_t     *thread_state = (mix_thread_state_t *) t;
	TCBDB                  *bdb=global_state->bdb;
	object_t               *obj = (object_t *) elem;
	int                    vs;

	MNEMOSYNE_ATOMIC {
		if (tcbdbget(bdb, &(obj->key), sizeof(int), &vs) == false) {
			return -1;
		}
	}
	return 0;
}


void tc_mix_latency_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 0, 1);
}


void tc_mix_throughput_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 0, 1);
}

void tc_mix_latency_think_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 1, 1);
}


void tc_mix_throughput_think_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 1, 1);
}


void tc_mix_latency_etl_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 1, 0, 0);
}


void tc_mix_throughput_etl_thread_main(unsigned int tid)
{
	mix_thread_main(tid, 0, 0, 0);
}
