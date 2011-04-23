#include <pthread.h>
#include <stdlib.h>
#include "reincarnation_callback.h"
#include "segment.h"
#include "log/log_i.h"
#include "thrdesc.h"
#include "debug.h"
#include "config.h"


static pthread_mutex_t global_init_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  global_init_cond = PTHREAD_COND_INITIALIZER;
//static pthread_mutex_t global_fini_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  global_fini_cond = PTHREAD_COND_INITIALIZER;
volatile uint32_t      mnemosyne_initialized = 0;


__thread mnemosyne_thrdesc_t *_mnemosyne_thr;

static void do_global_init(void) __attribute__(( constructor ));
static void do_global_fini(void) __attribute__(( destructor ));

void
do_global_init(void)
{
	pcm_storeset_t* pcm_storeset;
#ifdef _M_STATS_BUILD
	struct timeval     start_time;
	struct timeval     stop_time;
	unsigned long long op_time;
#endif

	if (mnemosyne_initialized) {
		return;
	}

	pcm_storeset = pcm_storeset_get ();

	pthread_mutex_lock(&global_init_lock);
	if (!mnemosyne_initialized) {
		mcore_config_init();
#ifdef _M_STATS_BUILD
		gettimeofday(&start_time, NULL);
#endif
		m_segmentmgr_init();
		mnemosyne_initialized = 1;
		mnemosyne_reincarnation_callback_execute_all();
#ifdef _M_STATS_BUILD
		gettimeofday(&stop_time, NULL);
		op_time = 1000000 * (stop_time.tv_sec - start_time.tv_sec) +
		                     stop_time.tv_usec - start_time.tv_usec;
		fprintf(stderr, "reincarnation_latency = %llu (us)\n", op_time);
#endif
		m_logmgr_init(pcm_storeset);
		M_WARNING("Initialize\n");
	}	
	pthread_mutex_unlock(&global_init_lock);
}


void
do_global_fini(void)
{
	if (!mnemosyne_initialized) {
		return;
	}

	pthread_mutex_lock(&global_init_lock);
	if (mnemosyne_initialized) {
		m_logmgr_fini();
		m_segmentmgr_fini();
		mtm_fini_global();
		mnemosyne_initialized = 0;

		M_WARNING("Shutdown\n");
	}	
	pthread_mutex_unlock(&global_init_lock);
}


void 
mnemosyne_init_global(void)
{
	do_global_init();
}


void 
mnemosyne_fini_global(void)
{
	do_global_fini();
}


mnemosyne_thrdesc_t *
mnemosyne_init_thread(void)
{
	mnemosyne_thrdesc_t *thr = NULL; //FIXME: get thread descriptor value

	if (thr) {
		M_WARNING("mnemosyne_init_thread:1: thr = %p\n", thr);
		return thr;
	}

	mnemosyne_init_global();
	thr = (mnemosyne_thrdesc_t *) malloc(sizeof(mnemosyne_thrdesc_t));
	//TODO: initialize values
	//thr->tx = NULL;
	//thr->vtable = defaultVtables->PTM;
	M_WARNING("mnemosyne_init_thread:2: thr = %p\n", thr);
	return thr;
}


void
mnemosyne_fini_thread(void)
{
	M_WARNING("mnemosyne_fini_thread:2\n");
}
