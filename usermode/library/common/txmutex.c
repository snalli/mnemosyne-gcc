#include <pthread.h>
#include "txmutex.h"


void _ITM_CALL_CONVENTION *m_txmutex_unlock_commit_action(void *arg)
{
	m_txmutex_t *txmutex = (m_txmutex_t *) arg;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	pthread_mutex_unlock(mutex);

	return NULL;
}
