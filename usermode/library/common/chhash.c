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

/**
 * \file chhash.c
 *
 * \brief Simple chained bucket hash table implementation.
 */

#include "debug.h"
#include "result.h"
#include "util.h"
#include "chhash.h"

typedef struct m_chhash_bucket_list_s m_chhash_bucket_list_t;

struct m_chhash_bucket_s {
	struct m_chhash_bucket_s *next;
	struct m_chhash_bucket_s *prev;
	m_chhash_key_t           key;
	m_chhash_value_t         value;
};

struct m_chhash_bucket_list_s {
	m_mutex_t         mutex;
	m_chhash_bucket_t *head;
	m_chhash_bucket_t *free;
};

struct m_chhash_s {
	unsigned int           tbl_size;
	m_chhash_bucket_list_t *tbl;
	bool                   mtsafe;
};


m_result_t 
m_chhash_create(m_chhash_t** hp, unsigned int table_size, bool mtsafe)
{
	int i;

	*hp = (m_chhash_t *) MALLOC(sizeof(m_chhash_t));
	if (*hp == NULL) 
	{
		return M_R_NOMEMORY;
	}
	(*hp)->tbl = (m_chhash_bucket_list_t*) CALLOC (table_size,
	                                               sizeof(m_chhash_bucket_list_t));
	if ((*hp)->tbl == NULL) 
	{
		FREE(*hp);
		return M_R_NOMEMORY;
	}

	for (i=0; i<table_size; i++) 
	{
		M_MUTEX_INIT(&(*hp)->tbl[i].mutex, NULL);
		(*hp)->tbl[i].head = NULL;
		(*hp)->tbl[i].free = NULL;
	}
	(*hp)->tbl_size = table_size;
	(*hp)->mtsafe = mtsafe;

	return M_R_SUCCESS;
}


m_result_t
m_chhash_destroy(m_chhash_t** hp)
{
	m_chhash_bucket_t *bucket;
	m_chhash_bucket_t *next_bucket;
	int               i;

	if (*hp == NULL) 
	{
		return M_R_SUCCESS;
	}
	for (i=0; i<(*hp)->tbl_size; i++) {
		for (bucket = (*hp)->tbl[i].head; bucket; bucket = next_bucket) {
			next_bucket = bucket->next;
			FREE(bucket);
		}
		for (bucket = (*hp)->tbl[i].free; bucket; bucket = next_bucket) {
			next_bucket = bucket->next;
			FREE(bucket);
		}
	}

	FREE((*hp)->tbl);
	FREE(*hp);
	
	return M_R_SUCCESS;
}


static void 
bucket_list_add(m_chhash_bucket_t **head, m_chhash_bucket_t *bucket)
{
	if (*head)
	{
		(*head)->prev = bucket;
	}
	bucket->next = *head;
	bucket->prev = NULL;
	(*head) = bucket;
}


static void 
bucket_list_remove(m_chhash_bucket_t **head, m_chhash_bucket_t *bucket)
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


m_result_t
m_chhash_add(m_chhash_t* h, 
             m_chhash_key_t key, 
             m_chhash_value_t value)
{
	m_result_t             result;
	m_chhash_bucket_t      *bucket;
	m_chhash_bucket_list_t *bucket_list = &h->tbl[key % h->tbl_size];

	if (h->mtsafe == true) 
	{
		M_MUTEX_LOCK(&(bucket_list->mutex));
	}

	for (bucket = bucket_list->head; 
	     bucket != NULL; 
	     bucket = bucket->next) 
	{
		if (bucket->key == key) 
		{
			result = M_R_EXISTS;
			goto done;
		}
	}
	if (bucket_list->free) 
	{
		bucket = bucket_list->free;
		bucket_list_remove(&bucket_list->free, bucket);
	} else {
		bucket = (m_chhash_bucket_t *) MALLOC(sizeof(m_chhash_bucket_t));
	}	
	bucket->key = key;
	bucket->value = value;
	bucket_list_add(&bucket_list->head, bucket);
	result = M_R_SUCCESS;

done:
	if (h->mtsafe == true) 
	{
		M_MUTEX_UNLOCK(&(bucket_list->mutex));
	}
	return result;
}


m_result_t
m_chhash_lookup(m_chhash_t* h,
                m_chhash_key_t key, 
                m_chhash_value_t *value)
{
	m_result_t             result;
	m_chhash_bucket_t      *bucket;
	m_chhash_bucket_list_t *bucket_list;

	bucket_list = &h->tbl[key % h->tbl_size];

	if (h->mtsafe == true) 
	{
		M_MUTEX_LOCK(&(bucket_list->mutex));
	}

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
			result = M_R_SUCCESS;
			goto done;
		}
	}
	result = M_R_NOTEXISTS;

done:
	if (h->mtsafe == true) 
	{
		M_MUTEX_UNLOCK(&(bucket_list->mutex));
	}
	return result;

}


m_result_t
m_chhash_remove(m_chhash_t* h,
                m_chhash_key_t key, 
                m_chhash_value_t *value)
{
	m_result_t             result;
	m_chhash_bucket_t      *bucket;
	m_chhash_bucket_list_t *bucket_list;

	bucket_list = &h->tbl[key % h->tbl_size];

	if (h->mtsafe == true) 
	{
		M_MUTEX_LOCK(&(bucket_list->mutex));
	}

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
			result = M_R_SUCCESS;
			goto done;
		}
	}
	result = M_R_NOTEXISTS;
done:
	if (h->mtsafe == true) 
	{
		M_MUTEX_UNLOCK(&(bucket_list->mutex));
	}
	return result;
}

/* Iterator is not multithreaded safe */

void
m_chhash_iter_init(m_chhash_t *chhash, m_chhash_iter_t *iter)
{
	unsigned int            i;
	m_chhash_bucket_t *bucket;

	iter->chhash = chhash;
	iter->index = 0;
	iter->bucket = NULL;

	for (i=0; i<chhash->tbl_size; i++) {
		if (bucket = chhash->tbl[i].head) {
			iter->bucket = bucket;
			iter->index = i;
			break;
		}
	}
}


m_result_t
m_chhash_iter_next(m_chhash_iter_t *iter, 
                   m_chhash_key_t *key, 
                   m_chhash_value_t *value)
{
	unsigned int            i;
	m_chhash_bucket_t *bucket;
	m_chhash_t        *chhash = iter->chhash;

	if (iter->bucket == NULL) {
		*key = 0;
		*value = NULL;
		return M_R_NULLITER;
	} 

	*key = iter->bucket->key;
	*value = iter->bucket->value;

	if (iter->bucket->next) {
		iter->bucket = iter->bucket->next;
	} else {	
		iter->bucket = NULL;
		for (i=iter->index+1; i<chhash->tbl_size; i++) {
			if (bucket = chhash->tbl[i].head) {
				iter->bucket = bucket;
				iter->index = i;
				break;
			}
		}
	}
	return M_R_SUCCESS;

}


void 
m_chhash_print(m_chhash_t *h) {
	int i;
	m_chhash_bucket_list_t *bucket_list;
	m_chhash_bucket_t      *bucket;

	fprintf(M_DEBUG_OUT, "HASH TABLE: %p\n", h);
	for (i=0;i<h->tbl_size; i++)
	{
		bucket_list = &h->tbl[i];
		if (bucket = bucket_list->head) {
			fprintf(M_DEBUG_OUT, "[%d]: head", i);
			for (; bucket != NULL; bucket=bucket->next)
			{
				fprintf(M_DEBUG_OUT, " --> (%u, %p)", bucket->key, bucket->value);
			}
			fprintf(M_DEBUG_OUT, "\n");
		}
		if (bucket = bucket_list->free) {
			fprintf(M_DEBUG_OUT, "[%d]: free", i);
			for (; bucket != NULL; bucket=bucket->next)
			{
				fprintf(M_DEBUG_OUT, " --> (%u, %p)", bucket->key, bucket->value);
			}
			fprintf(M_DEBUG_OUT, "\n");
		}
	}
}
