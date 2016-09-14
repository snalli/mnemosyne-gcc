/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/


/*
 * Source code is partially derived from TinySTM (license is attached)
 *
 *
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * Copyright (c) 2007-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <execinfo.h>
#include <signal.h>
#include <pthread.h>
#include "mtm_i.h"
#include "config.h"
#include "locks.h"
#include "mode/pwb-common/tmlog.h"
#include "sysdeps/x86/target.h"
#include "stats.h"

static pthread_mutex_t global_init_lock = PTHREAD_MUTEX_INITIALIZER;
volatile uint32_t mtm_initialized = 0;
static int global_num=0;

m_statsmgr_t *mtm_statsmgr;


/*
 * Catch signal (to emulate non-faulting load).
 */
static 
void 
signal_catcher(int sig)
{
	mtm_tx_t *tx = mtm_get_tx();

	/* A fault might only occur upon a load concurrent with a free (read-after-free) */
	PRINT_DEBUG("Caught signal: %d\n", sig);

	if (tx == NULL || tx->status == TX_IDLE) {
		/* There is not much we can do: execution will restart at faulty load */
		fprintf(stderr, "Error: invalid memory accessed and no longjmp destination\n");
		exit(1);
	}

#ifdef INTERNAL_STATS
	tx->aborts_invalid_memory++;
#endif /* INTERNAL_STATS */
	/* Will cause a longjmp */
	//FIXME: call restart_transaction
	//mtm_rollback(tx);
	assert(0);
}




static inline
void 
init_global()
{
#if CM == CM_PRIORITY
	char             *s;
#endif /* CM == CM_PRIORITY */
	struct sigaction act;
	pcm_storeset_t   *pcm_storeset;

	PRINT_DEBUG("==> mtm_init()\n");
	PRINT_DEBUG("\tsizeof(word)=%d\n", (int)sizeof(mtm_word_t));

	COMPILE_TIME_ASSERT(sizeof(mtm_word_t) == sizeof(void *));
	COMPILE_TIME_ASSERT(sizeof(mtm_word_t) == sizeof(atomic_t));

	mtm_config_init();

#ifdef EPOCH_GC
	gc_init(mtm_get_clock);
#endif /* EPOCH_GC */

	// Clear the lock bits (and also the write-set bits) for all addresses
	// in memory. Because the write-sets are also referenced by this array,
	// we must do this even in the case of disabled isolation.
   	PM_MEMSET((void *)locks, 0, LOCK_ARRAY_SIZE * sizeof(mtm_word_t));

#if CM == CM_PRIORITY
	s = getenv(VR_THRESHOLD);
	if (s != NULL) {
		vr_threshold = (int)strtol(s, NULL, 10);
	} else {
		vr_threshold = VR_THRESHOLD;
	}	
	PRINT_DEBUG("\tVR_THRESHOLD=%d\n", vr_threshold);
	s = getenv(CM_THRESHOLD);
	if (s != NULL) {
		cm_threshold = (int)strtol(s, NULL, 10);
	} else {
		cm_threshold = CM_THRESHOLD;
	}	
	PRINT_DEBUG("\tCM_THRESHOLD=%d\n", cm_threshold);
#endif /* CM == CM_PRIORITY */

	CLOCK = 0;
#ifdef ROLLOVER_CLOCK
	if (pthread_mutex_init(&tx_count_mutex, NULL) != 0) {
		fprintf(stderr, "Error creating mutex\n");
		exit(1);
	}
	if (pthread_cond_init(&tx_reset, NULL) != 0) {
		fprintf(stderr, "Error creating condition variable\n");
		exit(1);
	}

	tx_count = 0;
	tx_overflow = 0;
#endif /* ROLLOVER_CLOCK */

#ifndef TLS
	if (pthread_key_create(&_mtm_thread_tx, NULL) != 0) {
		fprintf(stderr, "Error creating thread local\n");
		exit(1);
	}
#endif /* ! TLS */

	pcm_storeset = pcm_storeset_get ();

	/* Register persistent logical log type with the log manager */
	m_logmgr_register_logtype(pcm_storeset, M_TMLOG_LF_TYPE, &M_TMLOG_OPS);
	/* Ask log manager to perform recovery on the new log type */
	m_logmgr_do_recovery(pcm_storeset);

#ifdef _M_STATS_BUILD	
	/* Create a statistics manager if need to dynamically profile */
	if (1) {
		m_statsmgr_create(&mtm_statsmgr, mtm_runtime_settings.stats_file);
	}	
#endif	

	/* Catch signals for non-faulting load 
	act.sa_handler = signal_catcher;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGBUS, &act, NULL) < 0 || sigaction(SIGSEGV, &act, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	*/
}



int 
mtm_init_global(void)
{
	if (mtm_initialized) {
		return 0;
	}

	pthread_mutex_lock(&global_init_lock);
	if (!mtm_initialized) {
		init_global();
		default_dtable_group = &normal_dtable_group;
		mtm_initialized = 1;
	}	
	pthread_mutex_unlock(&global_init_lock);
	return 0;
}


void 
fini_global()
{
	PRINT_DEBUG("==> mtm_exit()\n");

#ifndef TLS
	pthread_key_delete(_mtm_thread_tx);
#endif /* ! TLS */
#ifdef ROLLOVER_CLOCK
	//pthread_cond_destroy(&tx_reset);
	pthread_mutex_destroy(&tx_count_mutex);
#endif /* ROLLOVER_CLOCK */

#ifdef EPOCH_GC
	gc_exit();
#endif /* EPOCH_GC */
#ifdef _M_STATS_BUILD	
	m_stats_print(mtm_statsmgr);
#endif  
}



void 
mtm_fini_global(void)
{
	if (!mtm_initialized) {
		return;
	}

	pthread_mutex_lock(&global_init_lock);
	if (mtm_initialized) {
		fini_global();
		mtm_initialized = 0;
	}	
	pthread_mutex_unlock(&global_init_lock);
	return;
}


/*
 * Called by the CURRENT thread to initialize thread-local STM data.
 */
TXTYPE 
mtm_init_thread(void)
{
	mtm_tx_t       *tx = mtm_get_tx();
	mtm_mode_t     txmode;
	pthread_attr_t attr;


	if (tx) {
		TX_RETURN;
	}

	mtm_init_global();

	PRINT_DEBUG("==> mtm_init_thread()\n");

#ifdef EPOCH_GC
	gc_init_thread();
#endif /* EPOCH_GC */


	/* freud : Allocate descriptor */
#if ALIGNMENT == 1 /* no alignment requirement */
	if ((tx = (mtm_tx_t *)malloc(sizeof(mtm_tx_t))) == NULL) {
		perror("malloc");
		exit(1);
	}
#else
	if (posix_memalign((void **)&tx, ALIGNMENT, sizeof(mtm_tx_t)) != 0) {
		fprintf(stderr, "Error: cannot allocate aligned memory\n");
		exit(1);
	}
#endif

	/* Get current thread's PCM emulation bookkeeping structure */
	tx->pcm_storeset = pcm_storeset_get();

	/* Set status (no need for CAS or atomic op) */
	tx->status = TX_IDLE;

	/* Setup execution mode */
#undef ACTION
#define ACTION(mode) \
	mtm_##mode##_create(tx, &(tx->modedata[MTM_MODE_##mode]));
	FOREACH_MODE(ACTION)
#undef ACTION  

	txmode = mtm_str2mode(mtm_runtime_settings.force_mode);

	switch (txmode) {
		case MTM_MODE_pwbnl:
			// assert(0); /* freud : unsupported transaction mode */
			// hack break;
		case MTM_MODE_pwbetl:
			/* freud : get rid of this */
			tx->mode = MTM_MODE_pwbetl;
			/* We do not use the dispatch table though we construct it */
			// tx->dtable = default_dtable_group->mtm_pwbetl;
			break;
		default:
			assert(0); /* unknown transaction mode */
	}


	/* Nesting level */
	tx->nesting = 0;

	///* Transaction-specific data */
	//memset(tx->data, 0, MAX_SPECIFIC * sizeof(void *));
#ifdef READ_LOCKED_DATA
	/* Instance number */
	tx->id = 0;
#endif /* READ_LOCKED_DATA */
#ifdef CONFLICT_TRACKING
	/* Thread identifier */
	tx->thread_id = pthread_self();
#endif /* CONFLICT_TRACKING */
#if CM == CM_DELAY || CM == CM_PRIORITY
	/* Contented lock */
	tx->c_lock = NULL;
#endif /* CM == CM_DELAY || CM == CM_PRIORITY */
#if CM == CM_BACKOFF
	/* Backoff */
	tx->backoff = MIN_BACKOFF;
	tx->seed = 123456789UL;
#endif /* CM == CM_BACKOFF */
#if CM == CM_PRIORITY
	/* Priority */
	tx->priority = 0;
	tx->visible_reads = 0;
#endif /* CM == CM_PRIORITY */
#if CM == CM_PRIORITY || defined(INTERNAL_STATS)
	tx->retries = 0;
#endif /* CM == CM_PRIORITY || defined(INTERNAL_STATS) */
#ifdef INTERNAL_STATS
	/* Statistics */
	tx->aborts = 0;
	tx->aborts_ro = 0;
	tx->aborts_locked_read = 0;
	tx->aborts_locked_write = 0;
	tx->aborts_validate_read = 0;
	tx->aborts_validate_write = 0;
	tx->aborts_validate_commit = 0;
	tx->aborts_invalid_memory = 0;
	tx->aborts_reallocate = 0;
# ifdef ROLLOVER_CLOCK
	tx->aborts_rollover = 0;
# endif /* ROLLOVER_CLOCK */
# ifdef READ_LOCKED_DATA
	tx->locked_reads_ok = 0;
	tx->locked_reads_failed = 0;
# endif /* READ_LOCKED_DATA */
	tx->max_retries = 0;
#endif /* INTERNAL_STATS */
	/* Store as thread-local data */
#ifdef TLS
	_mtm_thread_tx = tx;
#else /* ! TLS */
	pthread_setspecific(_mtm_thread_tx, tx);
#endif /* ! TLS */
#ifdef ROLLOVER_CLOCK
	mtm_rollover_enter(tx);
#endif /* ROLLOVER_CLOCK */

	tx->tmp_jb_ptr = &(tx->tmp_jb);

	tx->stack_base = get_stack_base();
	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr, &tx->stack_size);

	mtm_local_init(tx);

	/* Allocate private write-back table; entries are set to zero by calloc. */
	if ((tx->wb_table = (mtm_word_t *)calloc(PRIVATE_LOCK_ARRAY_SIZE, sizeof(mtm_word_t))) == NULL) {
		perror("malloc");
		exit(1);
	}	

	mtm_useraction_list_alloc(&tx->commit_action_list);
	mtm_useraction_list_alloc(&tx->undo_action_list);

	tx->thread_num = __sync_add_and_fetch (&global_num, 1);
#ifdef _M_STATS_BUILD	
	m_stats_threadstat_create(mtm_statsmgr, tx->thread_num, &tx->threadstat);
#endif

	TX_RETURN;
}


/*
 * Called by the CURRENT thread to cleanup thread-local STM data.
 */
void 
mtm_fini_thread(void)
{
#ifdef EPOCH_GC
	mtm_word_t t;
#endif /* EPOCH_GC */
	mtm_tx_t *tx = mtm_get_tx();

	PRINT_DEBUG("==> mtm_exit_thread(%p)\n", tx);

#if 0
	/* Callbacks */
	if (nb_exit_cb != 0) {
		int cb;
		for (cb = 0; cb < nb_exit_cb; cb++) {
			exit_cb[cb].f(TXARGS exit_cb[cb].arg);
		}	
	}
#endif	

#ifdef ROLLOVER_CLOCK
	mtm_rollover_exit(tx);
#endif /* ROLLOVER_CLOCK */

	/* Create mode specific descriptors */
#undef ACTION
#define ACTION(mode) \
	mtm_##mode##_destroy((mtm_mode_data_t *) tx->modedata[MTM_MODE_##mode]);
	FOREACH_MODE(ACTION)
#undef ACTION  

	pcm_storeset_put();
#ifdef EPOCH_GC
	t = GET_CLOCK;
	gc_free(tx, t);
	gc_exit_thread();
#else /* ! EPOCH_GC */
	free(tx);
#endif /* ! EPOCH_GC */
}
