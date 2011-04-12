/**
 * \file rwlock.h
 * \brief Reader-writer lock definition
 *
 */

#ifndef MTM_RWLOCK_H_AGH190
#define MTM_RWLOCK_H_AGH190

typedef struct {
	//TODO: fields
} mtm_rwlock_t;

extern int mtm_rwlock_init (mtm_rwlock_t *);
extern int mtm_rwlock_rdlock (mtm_rwlock_t *);
extern int mtm_rwlock_wrlock (mtm_rwlock_t *);
extern int mtm_rwlock_trywrlock (mtm_rwlock_t *);
extern int mtm_rwlock_unlock (mtm_rwlock_t *);

#endif /* MTM_RWLOCK_H_AGH190 */
