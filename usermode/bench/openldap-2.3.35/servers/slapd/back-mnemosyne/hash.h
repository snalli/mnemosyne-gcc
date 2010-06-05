#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include <stdlib.h>
#include <mnemosyne.h>
#include <malloc.h>
#include "list.h"
#include "uthash.h"

#define HASH_REPLACE 1

typedef struct datum_s {
	char *data;
	int  size;
} datum_t;

typedef struct object_s {
	UT_hash_handle hh;
	char *value_data;
	int  value_size;
	int  key_size;
	char key_data[];
} object_t;

typedef struct hashtable_s {
	object_t         *ht;
	char             name[256];
	struct list_head list;
} hashtable_t;

__attribute__((tm_callable))
static int tx_memcmp(const void *m1, const void *m2, size_t n)
{
	unsigned char *s1 = (unsigned char *) m1;
	unsigned char *s2 = (unsigned char *) m2;

	while (n--) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		s1++;
		s2++;
	}
	return 0;
}

#if 1 
void * memcpy_nolog(void *, const void *, size_t);
void * memcpy_nolog_txn(void *, const void *, size_t);

__attribute__((tm_wrapping(memcpy_nolog))) void *memcpy_nolog_txn(void *, const void *, size_t);


void * memcpy_nolog(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

void *memcpy_nolog_txn(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}
#else 
# define memcpy_nolog memcpy
#endif 

static void print_data(char *ptr, int size)
{
	int i;

	if (size == 8) {
		printf("%llu\n", *((unsigned long long *) ptr));
		return;
	} 
	for (i=0; i<size; i++) {
		printf("%c", ptr[i]);
	}
	printf("\n");
}

static datum_t value_dup(object_t *obj)
{
	datum_t	dup;

	if ( obj->value_size == 0 ) {
		dup.size = 0;
		dup.data = NULL;

		return( dup );
	}
	dup.size = obj->value_size;

	if ( (dup.data = (char *) malloc( obj->value_size )) != NULL ) {
		memcpy(dup.data, obj->value_data, obj->value_size );
	}

	return( dup );
}

__attribute__ ((tm_callable))
static int hashtable_init(hashtable_t *hashtable, char *name) 
{
	object_t *r;

	hashtable->ht = NULL;
	strcpy(hashtable->name, name);
	/* HACK: Insert a dummy element to force allocation of the hash table */
	r = (object_t *) pmalloc(sizeof(object_t)+1);
	HASH_ADD(hh, hashtable->ht, key_data, 1, r);
	return 0;
}

static int hashtable_put(hashtable_t *hashtable, datum_t *k, datum_t *v, int flags)
{
	object_t *r;
	object_t *p;

	//printf("hashtable_put: k->size: %llu\n", (unsigned long long) k->size);
	//printf("hashtable_put: v->size: %llu\n", (unsigned long long) v->size);

	/* check for duplicates */
	HASH_FIND(hh, hashtable->ht, k->data, k->size, p);
	__tm_atomic 
	{
		if (p) {
			if (flags & HASH_REPLACE) {
				pfree(p->value_data);
				p->value_data = (char *) pmalloc(v->size);
				p->value_size = v->size;
				memcpy_nolog(p->value_data, v->data, v->size);
				return 0;
			} else {
				return -1;
			}	
		}

		r = (object_t *) pmalloc(sizeof(object_t)+k->size);
		r->key_size = k->size;
		memcpy(r->key_data, k->data, k->size);
		r->value_data = (char *) pmalloc(v->size);
		r->value_size = v->size;
		memcpy_nolog(r->value_data, v->data, v->size);
		HASH_ADD(hh, hashtable->ht, key_data, r->key_size, r);
	}

	return 0;
}


static int hashtable_get(hashtable_t *hashtable, datum_t *k, datum_t *v)
{
	object_t *r;
	datum_t  dupv;

	HASH_FIND(hh, hashtable->ht, k->data, k->size, r);

	if (!r) {
		return -1;
	}

	dupv = value_dup(r); /* this allocates new memory for the stored data */
	v->size = dupv.size;
	v->data = dupv.data;

	return 0;
}


static int hashtable_del(hashtable_t *hashtable, datum_t *k, int flags)
{
	object_t *p;

	HASH_FIND(hh, hashtable->ht, k->data, k->size, p);
	if (p) {
		//__tm_atomic 
		{
			HASH_DEL(hashtable->ht, p);
			pfree(p->value_data);
			pfree(p);
		}	
		return 0;
	}

	return -1;
}


#endif /* _HASH_TABLE_H */
