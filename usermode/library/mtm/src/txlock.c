#include <pthread.h>
#include "mtm_i.h"

typedef pthread_mutex_t m_txmutex_t;

void _ITM_CALL_CONVENTION *m_txmutex_unlock_commit_action(void *arg)
{
	m_txmutex_t *txmutex = (m_txmutex_t *) arg;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	//printf("[%d] COMMIT: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
	pthread_mutex_unlock(mutex);

	return NULL;
}

typedef pthread_rwlock_t m_txrwlock_t;

void _ITM_CALL_CONVENTION *m_txrwlock_unlock_commit_action(void *arg)
{
	m_txrwlock_t *txrwlock = (m_txrwlock_t *) arg;
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	//printf("[%d] COMMIT: RWLOCK UNLOCK %p\n", pthread_self(), rwlock);
	pthread_rwlock_unlock(rwlock);

	return NULL;
}
