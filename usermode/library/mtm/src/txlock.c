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

#include <pthread.h>
#include "mtm_i.h"

typedef pthread_mutex_t m_txmutex_t;

void _ITM_CALL_CONVENTION m_txmutex_unlock_commit_action(void *arg)
{
	m_txmutex_t *txmutex = (m_txmutex_t *) arg;
	pthread_mutex_t *mutex = (pthread_mutex_t *) txmutex;

	//printf("[%d] COMMIT: MUTEX  UNLOCK %p\n", pthread_self(), mutex);
	pthread_mutex_unlock(mutex);

	return;
}

typedef pthread_rwlock_t m_txrwlock_t;

void _ITM_CALL_CONVENTION m_txrwlock_unlock_commit_action(void *arg)
{
	m_txrwlock_t *txrwlock = (m_txrwlock_t *) arg;
	pthread_rwlock_t *rwlock = (pthread_rwlock_t *) txrwlock;

	//printf("[%d] COMMIT: RWLOCK UNLOCK %p\n", pthread_self(), rwlock);
	pthread_rwlock_unlock(rwlock);

	return;
}
