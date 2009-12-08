#include <common/mnemosyne_i.h>
#include <common/segment.h>
#include <pthread.h>


static pthread_mutex_t global_init_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  global_init_cond = PTHREAD_COND_INITIALIZER;
//static pthread_mutex_t global_fini_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  global_fini_cond = PTHREAD_COND_INITIALIZER;
volatile uint32_t      mnemosyne_initialized = 0;


__thread mnemosyne_thrdesc_t *_mnemosyne_thr;

static void do_global_init(void) __attribute__((constructor));
static void do_global_fini(void) __attribute__((destructor));

void
do_global_init(void)
{
	if (mnemosyne_initialized) {
		return;
	}

	pthread_mutex_lock(&global_init_lock);
	if (!mnemosyne_initialized) {
		//defaultVtables = &perfVtables;
		mnemosyne_segment_reincarnate_address_space();
		mnemosyne_initialized = 1;
		MNEMOSYNE_WARNING("Initialize\n");
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
		mnemosyne_segment_checkpoint_address_space();
		mnemosyne_initialized = 1;
		MNEMOSYNE_WARNING("Shutdown\n");
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


void
mnemosyne_fini_thread(void)
{
	MNEMOSYNE_WARNING("mnemosyne_fini_thread:2\n");
}
