#ifndef _M_TXMUTEX_H_GHA983
#define _M_TXMUTEX_H_GHA983

#include <itm.h>
#include <pthread.h>

typedef pthread_mutex_t m_txmutex_t;

void _ITM_CALL_CONVENTION *m_txmutex_unlock_commit_action(void *arg);

__attribute__((tm_pure))
static inline 
int m_txmutex_init(m_txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	return pthread_mutex_init(mutex, NULL);
}

__attribute__((tm_pure))
static inline 
int m_txmutex_lock(m_txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	return pthread_mutex_lock(mutex);
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
		_ITM_addUserCommitAction (td, m_txmutex_unlock_commit_action, 2, txmutex);
		ret = 0;
	} else {
		ret = pthread_mutex_unlock (mutex);
	}

	return ret;
}

#endif
