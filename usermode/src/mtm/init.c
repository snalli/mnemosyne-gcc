#include "mtm_i.h"
#include <pthread.h>

static uint32_t        proc_init = 0;
static pthread_mutex_t proc_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  proc_init_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t proc_fini_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  proc_fini_cond = PTHREAD_COND_INITIALIZER;
volatile uint32_t tm_initialized;


__thread mtm_thread_t *_mtm_thr;

void 
mtm_init_process(void)
{
	if (tm_initialized) {
		return;
	}

	pthread_mutex_lock(&proc_init_lock);
	while (proc_init) {
		pthread_cond_wait(&proc_init_cond, &proc_init_lock);
	}

	defaultVtables = &perfVtables;
	proc_init = 1;
	tm_initialized = 1;
	printf("Initialize\n");
	pthread_cond_signal(&proc_init_cond);
	pthread_mutex_unlock(&proc_init_lock);
}


mtm_thread_t *
mtm_init_thread(void)
{
	mtm_thread_t *thr = mtm_thr();

	if (thr) {
		printf("mtm_init_thread:1: thr = %p\n", thr);
		return thr;
	}

	mtm_init_process();
	thr = (mtm_thread_t *) malloc(sizeof(mtm_thread_t));
	_mtm_thr = thr;
	thr->tx = NULL;
	thr->tmp_jb_ptr = &(thr->tmp_jb);
	thr->vtable = defaultVtables->mtm_wbetl;
	printf("mtm_init_thread:2: thr = %p\n", thr);
	printf("mtm_init_thread:2: &thr->tmp_jb = %p\n", &thr->tmp_jb);
	return thr;
}
