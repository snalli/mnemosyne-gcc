#ifndef __SCOPED_LOCK__
#define __SCOPED_LOCK__

#include <pthread.h>
#include <assert.h>
struct ScopedLock {
	private:
		pthread_mutex_t *m_;
	public:
		ScopedLock(pthread_mutex_t *m): m_(m) {
			assert(pthread_mutex_lock(m_)==0);
		}
		~ScopedLock() {
			assert(pthread_mutex_unlock(m_)==0);
		}
};
#endif  /*__SCOPED_LOCK__*/
