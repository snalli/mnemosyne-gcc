/**
 * \file util.h
 *
 * \brief Several utilities/wrappers
 *
 */

#ifndef _M_MUTEX_H
#define _M_MUTEX_H

#include <stdlib.h>
#include <pthread.h>

typedef pthread_mutex_t m_mutex_t;

#define M_MUTEX_INIT     pthread_mutex_init
#define M_MUTEX_LOCK     pthread_mutex_lock
#define M_MUTEX_UNLOCK   pthread_mutex_unlock
#define M_MUTEX_TRYLOCK  pthread_mutex_trylock


#define MALLOC  malloc
#define CALLOC  calloc
#define REALLOC realloc 
#define FREE    free

#endif
