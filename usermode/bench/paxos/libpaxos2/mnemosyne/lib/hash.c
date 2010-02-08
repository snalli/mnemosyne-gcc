#include <pthread.h>
#include "util.h"

typedef struct util_hash_bucket_s util_hash_bucket_t;
typedef struct util_hash_bucket_list_s util_hash_bucket_list_t;

struct util_hash_bucket_s {
	struct util_hash_bucket_s *next;
	struct util_hash_bucket_s *prev;
	uint64_t key;
	void *value;
};

struct util_hash_bucket_list_s {
	util_hash_bucket_t *head;
	util_hash_bucket_t *free;
};

struct util_hash_s {
	uint64_t           tbl_size;
	util_hash_bucket_list_t *tbl;
	util_pool_t             *pool_bucket;
	util_bool_t             mtsafe;
};


util_result_t 
util_hash_create(util_hash_t** hp, uint64_t table_size, util_bool_t mtsafe)
{
	util_result_t result;
	int i;

	*hp = (util_hash_t *) MALLOC(sizeof(util_hash_t));
	if (*hp == NULL) 
	{
		return UTIL_R_NOMEMORY;
	}
	(*hp)->tbl = (util_hash_bucket_list_t*) CALLOC (table_size,
	                                               sizeof(util_hash_bucket_list_t));
	if ((*hp)->tbl == NULL) 
	{
		FREE(*hp);
		return UTIL_R_NOMEMORY;
	}
	if ((result = util_pool_create(&((*hp)->pool_bucket),
	                              sizeof(util_hash_bucket_t),
	                              table_size * HASH_FACTOR,
								  NULL)) 
	    != UTIL_R_SUCCESS) 
	{
	FREE((*hp)->tbl);
		FREE(*hp);
		return result;
	}

	for (i=0; i<table_size; i++) 
	{
		(*hp)->tbl[i].head = NULL;
		(*hp)->tbl[i].free = NULL;
	}
	(*hp)->tbl_size = table_size;
	(*hp)->mtsafe = mtsafe;

	return UTIL_R_SUCCESS;
}


util_result_t
util_hash_destroy(util_hash_t** hp)
{
	util_result_t result;

	if (*hp == NULL) 
	{
		return UTIL_R_SUCCESS;
	}
	if ((result = util_pool_destroy(&(*hp)->pool_bucket))
	    != UTIL_R_SUCCESS) 
	{
		return result;
	}
	FREE((*hp)->tbl);
	FREE(*hp);
	
	return UTIL_R_SUCCESS;
}


TM_CALLABLE
static void 
bucket_list_add(util_hash_bucket_t **head, util_hash_bucket_t *bucket)
{
	if (*head)
	{
		(*head)->prev = bucket;
	}
	bucket->next = *head;
	bucket->prev = NULL;
	(*head) = bucket;
}


TM_CALLABLE
static void 
bucket_list_remove(util_hash_bucket_t **head, util_hash_bucket_t *bucket)
{
	if (*head == bucket) 
	{
		*head = bucket->next;
		bucket->prev = NULL;
	} else {
		bucket->prev->next = bucket->next;
		if (bucket->next) 
		{
			bucket->next->prev = bucket->prev;
		}	
	}	
}


util_result_t
util_hash_add(util_hash_t* h, uint64_t key, void *value)
{
	util_result_t result;
	util_hash_bucket_t* bucket;
	util_hash_bucket_list_t* bucket_list;
	bucket_list = &h->tbl[key % h->tbl_size];

	for (bucket = bucket_list->head; 
	     bucket != NULL; 
	     bucket = bucket->next) 
	{
		if (bucket->key == key) 
		{
			result = UTIL_R_EXISTS;
			fprintf("EXISTS: %d\n", key);
			goto done;
		}
	}
	if (bucket_list->free) 
	{
		bucket = bucket_list->free;
		bucket_list_remove(&bucket_list->free, bucket);
	} else {
		if ((bucket = util_pool_object_alloc(h->pool_bucket))
	    	== NULL)
		{
			return result;
		}
	}	
	bucket->key = key;
	bucket->value = value;
	bucket_list_add(&bucket_list->head, bucket);
	result = UTIL_R_SUCCESS;

done:
	return result;
}


util_result_t
util_hash_lookup(util_hash_t* h, uint64_t key, void **value)
{
	util_result_t result;
	util_hash_bucket_t* bucket;
	util_hash_bucket_list_t* bucket_list;

	bucket_list = &h->tbl[key % h->tbl_size];

	for (bucket = bucket_list->head; 
	     bucket != NULL; 
	     bucket = bucket->next) 
	{
		if (bucket->key == key) 
		{
			if (value) 
			{
				*value = bucket->value;	
			}	
			result = UTIL_R_SUCCESS;
			goto done;
		}
	}
	result = UTIL_R_NOTEXISTS;

done:
	return result;

}


util_result_t
util_hash_remove(util_hash_t* h, uint64_t key, void **value)
{
	util_result_t result;
	util_hash_bucket_t* bucket;
	util_hash_bucket_list_t* bucket_list;

	bucket_list = &h->tbl[key % h->tbl_size];

	for (bucket = bucket_list->head; 
	     bucket != NULL; 
	     bucket = bucket->next) 
	{
		if (bucket->key == key) 
		{
			if (value) 
			{
				*value = bucket->value;	
			}	
			bucket_list_remove(&bucket_list->head, bucket);
			bucket_list_add(&bucket_list->free, bucket);
			result = UTIL_R_SUCCESS;
			goto done;
		}
	}
	result = UTIL_R_NOTEXISTS;
done:
	return result;
}


void 
util_hash_print(util_hash_t *h) {
	int i;
	util_hash_bucket_list_t *bucket_list;
	util_hash_bucket_t *bucket;

	fprintf(UTIL_DEBUG_OUT, "HASH TABLE: %p\n", h);
	for (i=0;i<h->tbl_size; i++)
	{
		bucket_list = &h->tbl[i];
		if (bucket = bucket_list->head) {
			fprintf(UTIL_DEBUG_OUT, "[%d]: head", i);
			for (; bucket != NULL; bucket=bucket->next)
			{
				fprintf(UTIL_DEBUG_OUT, " --> (%u, %p)", bucket->key, bucket->value);
			}
			fprintf(UTIL_DEBUG_OUT, "\n");
		}
		if (bucket = bucket_list->free) {
			fprintf(UTIL_DEBUG_OUT, "[%d]: free", i);
			for (; bucket != NULL; bucket=bucket->next)
			{
				fprintf(UTIL_DEBUG_OUT, " --> (%u, %p)", bucket->key, bucket->value);
			}
			fprintf(UTIL_DEBUG_OUT, "\n");
		}
	}
}


void 
util_hash_sanity_check(util_hash_t *h, int (*sanity_check_f)(uint64_t, void *)) {
	int i;
	util_hash_bucket_list_t *bucket_list;
	util_hash_bucket_t *bucket;

	for (i=0;i<h->tbl_size; i++)
	{
		bucket_list = &h->tbl[i];
		if (bucket = bucket_list->head) {
			for (; bucket != NULL; bucket=bucket->next)
			{
				if (sanity_check_f(bucket->key, bucket->value) != 0)
				{
					fprintf(UTIL_DEBUG_OUT, "Hash table sanity check failed: key = %llu\n", bucket->key);
					abort();	
				}
			}
		}
	}
}
