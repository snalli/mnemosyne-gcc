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

typedef pthread_mutex_t m_txmutex_t;

void _ITM_CALL_CONVENTION *m_txmutex_unlock_commit_action(void *arg);


__attribute__((tm_pure))
static inline 
int m_txmutex_init(m_txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;
	pthread_mutexattr_t attr;
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	return pthread_mutex_init(mutex, &attr);
}


__attribute__((tm_pure))
static inline 
int m_txmutex_destroy(m_txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	return pthread_mutex_destroy(mutex);
}

__attribute__((noinline))
static
void breakme() { }

__attribute__((tm_pure))
static inline 
int m_txmutex_lock(m_txmutex_t *txmutex)
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
int m_txmutex_unlock(m_txmutex_t *txmutex)
{
	int             ret;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	_ITM_transaction * td;
	td = _ITM_getTransaction();

	if (_ITM_inTransaction(td) > 0) {
		DEBUG_PRINTF("[%d] DEFER: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
		_ITM_addUserCommitAction (td, m_txmutex_unlock_commit_action, 2, txmutex);
		ret = 0;
	} else {
		DEBUG_PRINTF("[%d] NOW  : MUTEX  UNLOCK %p\n", pthread_self(), mutex);
		ret = pthread_mutex_unlock (mutex);
	}

	return ret;
}

/* READER/WRITER TXSAFE LOCKS */

typedef pthread_rwlock_t m_txrwlock_t;

void _ITM_CALL_CONVENTION *m_txrwlock_unlock_commit_action(void *arg);


__attribute__((tm_pure))
static inline 
int m_txrwlock_init(m_txrwlock_t *txrwlock)
{
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	return pthread_rwlock_init(rwlock, NULL);
}


__attribute__((tm_pure))
static inline 
int m_txrwlock_destroy(m_txrwlock_t *txrwlock)
{
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	return pthread_rwlock_destroy(rwlock);
}


__attribute__((tm_pure))
static inline 
int m_txrwlock_rdlock(m_txrwlock_t *txrwlock)
{
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	/* 
	 * TODO: Support a thread re-locking a lock it owns as 
	 * locks are not released till commit
	 */
	DEBUG_PRINTF("[%d] NOW  : RWLOCK RDLOCK %p\n", pthread_self(), rwlock);
	return pthread_rwlock_rdlock(rwlock);
}


__attribute__((tm_pure))
static inline 
int m_txrwlock_wrlock(m_txrwlock_t *txrwlock)
{
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	/* 
	 * TODO: Support a thread re-locking a lock it owns as 
	 * locks are not released till commit
	 */
	DEBUG_PRINTF("[%d] NOW  : RWLOCK WRLOCK %p\n", pthread_self(), rwlock);
	return pthread_rwlock_wrlock(rwlock);
}


__attribute__((tm_pure))
static inline 
int m_txrwlock_unlock(m_txrwlock_t *txrwlock)
{
	int             ret;
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	_ITM_transaction * td;
	td = _ITM_getTransaction();

	if (_ITM_inTransaction(td) > 0) {
		DEBUG_PRINTF("[%d] DEFER: RWLOCK UNLOCK %p\n", pthread_self(), rwlock);
		_ITM_addUserCommitAction (td, m_txrwlock_unlock_commit_action, 2, txrwlock);
		ret = 0;
	} else {
		DEBUG_PRINTF("[%d] NOW  : RWLOCK UNLOCK %p\n", pthread_self(), rwlock);
		ret = pthread_rwlock_unlock (rwlock);
	}

	return ret;
}


#endif
