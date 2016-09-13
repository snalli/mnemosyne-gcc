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

#ifndef _M_TXLOCK_H_GHA983
#define _M_TXLOCK_H_GHA983

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


//FIXME: These locks are not deadlock safe -- need to 
//drop locks if can't get a lock to avoid deadlock
//See Volos Transact 08

#include <itm.h>
#include <pthread.h>

#define DEBUG_PRINTF(format, ...)
//#define DEBUG_PRINTF(format, a, b) printf(format, a, b)

/* MUTEX TXSAFE LOCKS */

typedef pthread_mutex_t m_txmutex_t;

void _ITM_CALL_CONVENTION m_txmutex_unlock_commit_action(void *arg);


__attribute__((transaction_pure))
static inline 
int m_txmutex_init(m_txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;
	pthread_mutexattr_t attr;
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	return pthread_mutex_init(mutex, &attr);
}


__attribute__((transaction_pure))
static inline 
int m_txmutex_destroy(m_txmutex_t *txmutex)
{
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	return pthread_mutex_destroy(mutex);
}

__attribute__((noinline))
static
void breakme() { }

__attribute__((transaction_pure))
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


void _ITM_CALL_CONVENTION m_txmutex_unlock_commit_action_local(void *arg)
{
	m_txmutex_t *txmutex = (m_txmutex_t *) arg;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	//printf("[%d] COMMIT: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
	pthread_mutex_unlock(mutex);

	return;
}



__attribute__((transaction_pure))
static inline 
int m_txmutex_unlock(m_txmutex_t *txmutex)
{
	int             ret;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	_ITM_transaction * td;
	td = _ITM_getTransaction();

	if (_ITM_inTransaction() > 0) {
		DEBUG_PRINTF("[%d] DEFER: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
		_ITM_addUserCommitAction (m_txmutex_unlock_commit_action_local, 2, txmutex);
		//_ITM_addUserCommitAction (td, m_txmutex_unlock_commit_action_local, 2, txmutex);
		ret = 0;
	} else {
		DEBUG_PRINTF("[%d] NOW  : MUTEX  UNLOCK %p\n", pthread_self(), mutex);
		ret = pthread_mutex_unlock (mutex);
	}

	return ret;
}

/* READER/WRITER TXSAFE LOCKS */

typedef pthread_rwlock_t m_txrwlock_t;

void _ITM_CALL_CONVENTION m_txrwlock_unlock_commit_action(void *arg);


__attribute__((transaction_pure))
static inline 
int m_txrwlock_init(m_txrwlock_t *txrwlock)
{
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	return pthread_rwlock_init(rwlock, NULL);
}


__attribute__((transaction_pure))
static inline 
int m_txrwlock_destroy(m_txrwlock_t *txrwlock)
{
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	return pthread_rwlock_destroy(rwlock);
}


__attribute__((transaction_pure))
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


__attribute__((transaction_pure))
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


__attribute__((transaction_pure))
static inline 
int m_txrwlock_unlock(m_txrwlock_t *txrwlock)
{
	int             ret;
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	_ITM_transaction * td;
	td = _ITM_getTransaction();

	if (_ITM_inTransaction() > 0) {
		DEBUG_PRINTF("[%d] DEFER: RWLOCK UNLOCK %p\n", pthread_self(), rwlock);
		_ITM_addUserCommitAction (m_txrwlock_unlock_commit_action, 2, txrwlock);
		ret = 0;
	} else {
		DEBUG_PRINTF("[%d] NOW  : RWLOCK UNLOCK %p\n", pthread_self(), rwlock);
		ret = pthread_rwlock_unlock (rwlock);
	}

	return ret;
}


#endif
