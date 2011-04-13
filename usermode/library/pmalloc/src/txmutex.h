/* 
 * This is a subset of the txlock.[h|c] defined by the MTM library.
 * We don't use the txlock.[h|c] to avoid depending on the MTM
 * library. 
 *
 * Note: We already depend on MTM as we use transactions along
 * the memory allocators generic slow path but we would like to
 * remove this dependency in the future.
 */

#ifndef _M_TXMUTEX_H_GHA983
#define _M_TXMUTEX_H_GHA983

#undef pthread_mutex_t
#undef pthread_mutex_init
#undef pthread_mutex_destroy
#undef pthread_mutex_lock
#undef pthread_mutex_unlock

#undef pthread_rwlock_t
#undef pthread_rwlock_init
#undef pthread_rwlock_destroy
#undef pthread_rwlock_rdlock
#undef pthread_rwlock_wrlock
#undef pthread_rwlock_unlock


#include <itm.h>
#include <pthread.h>

#define DEBUG_PRINTF(format, ...)
//#define DEBUG_PRINTF(format, a, b) printf(format, a, b)

/* MUTEX TXSAFE LOCKS */

typedef pthread_mutex_t txmutex_t;

static
void _ITM_CALL_CONVENTION txmutex_unlock_commit_action(void *arg)
{
	txmutex_t *txmutex = (txmutex_t *) arg;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	//printf("[%d] COMMIT: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
	pthread_mutex_unlock(mutex);

	return;
}

__attribute__((tm_pure))
static inline 
int txmutex_init(txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;
	pthread_mutexattr_t attr;
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	return pthread_mutex_init(mutex, &attr);
}


__attribute__((tm_pure))
static inline 
int txmutex_destroy(txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	return pthread_mutex_destroy(mutex);
}


__attribute__((tm_pure))
static inline 
int txmutex_lock(txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;
	int ret;

	DEBUG_PRINTF("[%d] NOW  : MUTEX  LOCK   %p\n", pthread_self(), mutex);
	ret = pthread_mutex_lock(mutex);
	DEBUG_PRINTF("[%d] NOW  : MUTEX  LOCK   %p DONE\n", pthread_self(), mutex);

	return ret;
}


__attribute__((tm_pure))
static inline 
int txmutex_unlock(txmutex_t *txmutex)
{
	int             ret;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	_ITM_transaction * td;
	td = _ITM_getTransaction();

	if (_ITM_inTransaction(td) > 0) {
		DEBUG_PRINTF("[%d] DEFER: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
		_ITM_addUserCommitAction (td, txmutex_unlock_commit_action, 2, txmutex);
		ret = 0;
	} else {
		DEBUG_PRINTF("[%d] NOW  : MUTEX  UNLOCK %p\n", pthread_self(), mutex);
		ret = pthread_mutex_unlock (mutex);
	}

	return ret;
}


#endif
