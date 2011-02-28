#ifndef _TM_H_AAA121
#define _TM_H_AAA121

#define TM_ATOMIC __tm_atomic

#if 0

#define pthread_mutex_lock(lock)     \
do {     \
  printf("%x: MUTEX_LOCK %s: WAIT\n", pthread_self(), #lock); \
  pthread_mutex_lock(lock); \
  printf("%x: MUTEX_LOCK %s: GOT IT\n", pthread_self(), #lock); \
} while (0); 

#define pthread_mutex_unlock(lock)     \
do {     \
  printf("%x: MUTEX_UNLOCK %s \n", pthread_self(), #lock); \
  pthread_mutex_unlock(lock); \
} while (0); 

#endif

#endif
