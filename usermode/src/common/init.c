#include <common/mnemosyne_i.h>
#include <common/segment.h>
#include <pthread.h>


static uint32_t        global_init = 0;
static pthread_mutex_t global_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  global_init_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t global_fini_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  global_fini_cond = PTHREAD_COND_INITIALIZER;
volatile uint32_t      mnemosyne_initialized;


__thread mnemosyne_thrdesc_t *_mnemosyne_thr;

static void do_global_init(void) __attribute__((constructor));

void
do_global_init(void)
{
	if (mnemosyne_initialized) {
		return;
	}

	pthread_mutex_lock(&global_init_lock);
	while (global_init) {
		pthread_cond_wait(&global_init_cond, &global_init_lock);
	}

	//defaultVtables = &perfVtables;
	mnemosyne_segment_reincarnate();
	global_init = 1;
	mnemosyne_initialized = 1;
	MNEMOSYNE_WARNING("Initialize\n");
	pthread_cond_signal(&global_init_cond);
	pthread_mutex_unlock(&global_init_lock);
}

void 
mnemosyne_init_global(void)
{
	do_global_init();
}


mnemosyne_thrdesc_t *
mnemosyne_init_thread(void)
{
	mnemosyne_thrdesc_t *thr = NULL; //FIXME: get thread descriptor value

	if (thr) {
		MNEMOSYNE_WARNING("mnemosyne_init_thread:1: thr = %p\n", thr);
		return thr;
	}

	mnemosyne_init_global();
	thr = (mnemosyne_thrdesc_t *) malloc(sizeof(mnemosyne_thrdesc_t));
	//TODO: initialize values
	//thr->tx = NULL;
	//thr->vtable = defaultVtables->PTM;
	MNEMOSYNE_WARNING("mnemosyne_init_thread:2: thr = %p\n", thr);
	return thr;
}	
