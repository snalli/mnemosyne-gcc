/**
 * \file pool.c
 *
 * \brief Pool allocator implementation.
 *
 */

#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include "util.h"


#define UTIL_DEBUG_POOL 0

struct util_pool_object_s {
	void   *buf;
	char   status;
	struct util_pool_object_s *next;
	struct util_pool_object_s *prev;
};

struct util_pool_s {
	void                          *full_buf;			
	util_pool_object_t             *obj_list;
	util_pool_object_t             *obj_free_head;
	util_pool_object_t             *obj_allocated_head;
	unsigned int                  obj_free_num;
	unsigned int                  obj_allocated_num;
	unsigned int                  obj_size;
	unsigned int                  obj_num;
	util_pool_object_constructor_t obj_constructor;
};


util_result_t
util_pool_create(util_pool_t **poolp, 
                unsigned int obj_size, 
                unsigned int obj_num, 
                util_pool_object_constructor_t obj_constructor) 
{
	util_pool_object_t *object_list;
	char *buf; 
	int i;

	*poolp = (util_pool_t *) MALLOC(sizeof(util_pool_t));
	if (*poolp == NULL) {
		return UTIL_R_NOMEMORY;
	}
	(*poolp)->obj_size = obj_size;
	(*poolp)->obj_num = obj_num;
	object_list = (util_pool_object_t *) CALLOC(obj_num, 
	                                           sizeof(util_pool_object_t));
	if (object_list == NULL) {
		FREE(*poolp);
		return UTIL_R_NOMEMORY;
	}
	(*poolp)->obj_list = object_list;
	if ((buf = CALLOC(obj_num, obj_size)) == NULL) {  
		FREE(object_list);
		FREE(*poolp);
		return UTIL_R_NOMEMORY;
	}
	(*poolp)->full_buf = buf; 
	for (i=0;i<obj_num;i++) {
		object_list[i].buf = &buf[i*obj_size];
		object_list[i].next = &object_list[(i+1)%obj_num];
		object_list[i].prev = &object_list[(i-1)%obj_num];
		object_list[i].status = UTIL_POOL_OBJECT_FREE;
		if (obj_constructor != NULL) {
			obj_constructor(object_list[i].buf);
		}
	}
	object_list[0].prev = object_list[obj_num-1].next = NULL;
	(*poolp)->obj_free_num = obj_num;
	(*poolp)->obj_free_head = &object_list[0];	
	(*poolp)->obj_allocated_num = 0;
	(*poolp)->obj_allocated_head = NULL;	
	return UTIL_R_SUCCESS;
}


util_result_t 
util_pool_destroy(util_pool_t **poolp) 
{
	UTIL_ASSERT(*poolp != NULL);
	FREE((*poolp)->full_buf);
	FREE((*poolp)->obj_list);	
	FREE(*poolp);
	*poolp = NULL;
	return UTIL_R_SUCCESS;
}


void *
util_pool_object_alloc(util_pool_t *pool) 
{
	util_result_t result;
	util_pool_object_t *pool_object;

	if (pool->obj_free_num == 0) {
		return NULL;
	}
	pool_object = pool->obj_free_head;
	pool->obj_free_head = pool_object->next;
	if (pool->obj_free_head) {
		pool->obj_free_head->prev = NULL;
	}
	pool_object->prev = NULL;
	pool_object->next = pool->obj_allocated_head;
	if (pool->obj_allocated_head) {
		pool->obj_allocated_head->prev = pool_object;
	}
	pool->obj_allocated_head = pool_object; 
	pool->obj_allocated_num++;
	pool->obj_free_num--;
	pool_object->status = UTIL_POOL_OBJECT_ALLOCATED;
	return pool_object->buf;
}


void
util_pool_object_free(util_pool_t *pool, void *objp) 
{
	util_pool_object_t *pool_object;
	unsigned int index;

	UTIL_ASSERT(pool);
	UTIL_ASSERT(objp);
	index = ((unsigned int) ( (char*) objp - (char*) pool->full_buf)) / pool->obj_size;
	pool_object = &pool->obj_list[index];
	UTIL_ASSERT(pool_object->status == UTIL_POOL_OBJECT_ALLOCATED);
	if (pool_object->prev) {
		pool_object->prev->next = pool_object->next;
	} else { 
		pool->obj_allocated_head = pool_object->next;
	}
	if (pool_object->next) {
		pool_object->next->prev = pool_object->prev;
	}
	pool_object->next = pool->obj_free_head;
	pool_object->prev = NULL;
	if (pool->obj_free_head) {
		pool->obj_free_head->prev = pool_object;
	} 
	pool->obj_allocated_num--;
	pool->obj_free_num++;
	pool_object->status = UTIL_POOL_OBJECT_FREE;
	pool->obj_free_head = pool_object; 
}


util_pool_object_t *
util_pool_object_first(util_pool_t *pool, int obj_status)
{
	switch (obj_status) {
		case UTIL_POOL_OBJECT_FREE:
			return pool->obj_free_head;
		case UTIL_POOL_OBJECT_ALLOCATED:
			return pool->obj_allocated_head;
		case UTIL_POOL_OBJECT_ALLOCATED & UTIL_POOL_OBJECT_FREE:
			return pool->obj_list;
		default:
			UTIL_INTERNALERROR("Unknown status of pool object");
	}
	return NULL;
}


util_pool_object_t *
util_pool_object_next(util_pool_object_t *obj)
{
	return obj->next;
}


void *
util_pool_object_of(util_pool_object_t *obj)
{
	return obj->buf;
}


/*
 **************************************************************************
 ***                     DEBUGGING SUPPORT ROUTINES                     ***
 **************************************************************************
 */


void 
util_pool_print(util_pool_t *pool, util_pool_object_print_t printer, int verbose) 
{
	util_pool_object_t *pool_object;
	unsigned int index;

	if (UTIL_DEBUG_POOL) {
		fprintf(UTIL_DEBUG_OUT, "FREE POOL\n");
		pool_object = pool->obj_free_head;
		while (pool_object) {
			index = (pool_object - pool->obj_list);
			if (verbose) {
				fprintf(UTIL_DEBUG_OUT, "Object Index = %u\t", index);
				if (pool_object->prev) {
					index = (pool_object->prev - pool->obj_list);
					fprintf(UTIL_DEBUG_OUT, "[prev = %u, ", index);
				} else {
					fprintf(UTIL_DEBUG_OUT, "[prev = NULL, ");
				} 
				if (pool_object->next) {
					index = (pool_object->next - pool->obj_list);
					fprintf(UTIL_DEBUG_OUT, "next = %u]\n", index);
				} else {
					fprintf(UTIL_DEBUG_OUT, "next = NULL]\n");
				}
			}
			if (printer) {
				printer(pool_object->buf);
			} 
			pool_object = pool_object->next;
		}
		fprintf(UTIL_DEBUG_OUT, "\nALLOCATED POOL\n");
		pool_object = pool->obj_allocated_head;
		while (pool_object) {
			index = (pool_object - pool->obj_list);
			if (verbose) {
				fprintf(UTIL_DEBUG_OUT, "Object Index = %u\t", index);
				if (pool_object->prev) {
					index = (pool_object->prev - pool->obj_list);
					fprintf(UTIL_DEBUG_OUT, "[prev = %u, ", index);
				} else {
					fprintf(UTIL_DEBUG_OUT, "[prev = NULL, ");
				} 
				if (pool_object->next) {
					index = (pool_object->next - pool->obj_list);
					fprintf(UTIL_DEBUG_OUT, "next = %u]\n", index);
				} else {
					fprintf(UTIL_DEBUG_OUT, "next = NULL]\n");
				} 
			}
			if (printer) {
				printer(pool_object->buf);
			} 
			pool_object = pool_object->next;
		}
	}
}
