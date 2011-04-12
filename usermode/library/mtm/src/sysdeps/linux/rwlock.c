/**
 * \file rwlock.c
 * \brief Reader-writer lock implementation 
 *
 * \todo Implement rw-lock if need to support serial mode for irrevocable 
 * actions
 *
 * Note: Linux implementation could use pthreads rwlock
 *
 */

#include "rwlock.h"

int
mtm_rwlock_init (mtm_rwlock_t *lock)
{
	// TODO: implement me
}


int
mtm_rwlock_rdlock (mtm_rwlock_t *lock)
{
	// TODO: implement me
}


int
mtm_rwlock_wrlock (mtm_rwlock_t *lock)
{
	// TODO: implement me
}


int
mtm_rwlock_trywrlock (mtm_rwlock_t *lock)
{
	// TODO: implement me
}


int
mtm_rwlock_unlock (mtm_rwlock_t *lock)
{
	// TODO: implement me
}
