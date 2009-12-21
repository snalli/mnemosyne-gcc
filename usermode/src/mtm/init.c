#include "mtm_i.h"
#include <pthread.h>

static pthread_mutex_t global_init_lock = PTHREAD_MUTEX_INITIALIZER;
volatile uint32_t mtm_initialized = 0;

__thread mtm_thread_t *_mtm_thr;

int 
mtm_init_global(void)
{
	if (mtm_initialized) {
		return 0;
	}

	pthread_mutex_lock(&global_init_lock);
	if (!mtm_initialized) {
		defaultVtables = &perfVtables;
		mtm_initialized = 1;
	}	
	pthread_mutex_unlock(&global_init_lock);
	return 0;
}


mtm_thread_t *
mtm_init_thread(void)
{
	mtm_thread_t *thr = mtm_thr();

	if (thr) {
		return thr;
	}

	mtm_init_global();
	thr = (mtm_thread_t *) malloc(sizeof(mtm_thread_t));
	_mtm_thr = thr;
	thr->tx = NULL;
	thr->tmp_jb_ptr = &(thr->tmp_jb);
	thr->vtable = defaultVtables->mtm_wbetl;

	return thr;
}

void 
mtm_fini_global(void)
{
	//TODO
}


void 
mtm_fini_thread(void)
{
	//TODO
}
