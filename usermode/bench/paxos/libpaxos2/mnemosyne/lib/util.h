#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tm.h"


/* GENERIC */
#define UTIL_R_SUCCESS     0
#define UTIL_R_NOMEMORY    1
#define UTIL_R_NOTEXISTS   2
#define UTIL_R_EXISTS      3
#define UTIL_R_NOTINITLOCK 4

#define MALLOC malloc
#define CALLOC calloc
#define FREE free
#define UTIL_DEBUG_OUT  stdout
#define UTIL_BOOL_FALSE 0
#define UTIL_BOOL_TRUE  1

#define UTIL_ASSERT(expr) 
#define UTIL_INTERNALERROR(expr) 


typedef int util_result_t;
typedef int util_bool_t;


#ifdef DEBUG
# error
#else
# define PRINT_DEBUG
#endif


/* HASH TABLE */

#define HASH_FACTOR 4

/* Opaque structure used to represent hashtable. */
typedef struct util_hash_s util_hash_t;

util_result_t util_hash_create(util_hash_t**, uint64_t, util_bool_t);
util_result_t util_hash_destroy(util_hash_t**);
TM_CALLABLE util_result_t util_hash_add(util_hash_t*, uint64_t, void*);
TM_CALLABLE util_result_t util_hash_lookup(util_hash_t*, uint64_t, void**);
TM_CALLABLE util_result_t util_hash_remove(util_hash_t*, uint64_t, void**);
void util_hash_print(util_hash_t *);



/* POOL ALLOCATOR */

#define UTIL_POOL_OBJECT_FREE         0x1	
#define UTIL_POOL_OBJECT_ALLOCATED    0x2	



/**
 * The pool works as a slab allocator. That is when allocating an objects 
 * you can assume that its state is the same as when the object was 
 * returned back to the pool.
 *
 * Objects are allocated from the head 
 */

typedef struct util_pool_object_s util_pool_object_t;
typedef struct util_pool_s util_pool_t;
typedef void (*util_pool_object_constructor_t)(void *obj); 
typedef void (*util_pool_object_print_t)(void *obj); 

util_result_t util_pool_create(util_pool_t **poolp, 
                             unsigned int obj_size,
                             unsigned int obj_num, 
                             util_pool_object_constructor_t obj_constructor);
util_result_t util_pool_destroy(util_pool_t **poolp);
TM_CALLABLE void *util_pool_object_alloc(util_pool_t *pool);
TM_CALLABLE void util_pool_object_free(util_pool_t *pool, void *objp);
TM_CALLABLE util_pool_object_t *util_pool_object_first(util_pool_t *pool, int obj_status);
TM_CALLABLE util_pool_object_t *util_pool_object_next(util_pool_object_t *obj);
TM_CALLABLE void *util_pool_object_of(util_pool_object_t *obj);
void util_pool_print(util_pool_t *pool, util_pool_object_print_t printer, int verbose);
TM_CALLABLE void util_hash_sanity_check(util_hash_t *h, int (*)(uint64_t, void *));
#endif
