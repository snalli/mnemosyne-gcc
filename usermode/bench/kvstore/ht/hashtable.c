/* Copyright (C) 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */

#include "hashtable.h"
#include "hashtable_private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include "sloppycnt.h"

#include <mnemosyne.h>
#include <mtm.h>
#include <malloc.h>
#include "txlock.h"

//#define MNEMOSYNE_ATOMIC __transaction [[relaxed]]
//#define MNEMOSYNE_ATOMIC
//void m_logmgr_fini() {}


//#define _DEBUG_THIS

#define INTERNAL_ERROR(msg)                                             \
do {                                                                    \
  fprintf(stderr, "ERROR [%s:%d] %s.\n", __FILE__, __LINE__, msg);      \
  exit(-1);                                                             \
} while(0);

#define PAGE_SIZE                 4096
#define PRIVATE_REGION_SIZE       (64*1024*PAGE_SIZE)

/*
Credit for primes table: Aaron Krowne
 http://br.endernet.org/~akrowne/
 http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
*/
static const unsigned int primes[] = {
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
const float max_load_factor = 0.65;


inline unsigned int
unsigned_int_fetch_and_add_full (volatile unsigned int *p, unsigned int incr)
{
  unsigned int result;

  __asm__ __volatile__ ("lock; xaddl %0, %1" :
                        "=r" (result), "=m" (*p) : "0" (incr), "m" (*p)
                        : "memory");
  return result;
}



/*****************************************************************************/
struct hashtable *
create_hashtable(unsigned int minsize,
                 unsigned int (*hashf) (void*),
                 int (*eqf) (void*,void*))
{
	int i;
    struct hashtable *h;
    unsigned int pindex, size = primes[0];
    /* Check requested hashtable isn't too large */
    if (minsize > (1u << 30)) return NULL;
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }
    h = (struct hashtable *)pmalloc(sizeof(struct hashtable));
    if (NULL == h) return NULL; /*oom*/
    h->table = (struct entry **)pmalloc(sizeof(struct entry*) * size);
    if (NULL == h->table) { pfree(h); return NULL; } /*oom*/
    h->table_mutex = (m_txmutex_t *)pmalloc(sizeof(m_txmutex_t) * size);
    if (NULL == h->table_mutex) { 
		pfree(h->table); 
		pfree(h); 
		return NULL; 
	} /*oom*/
    memset(h->table, 0, size * sizeof(struct entry *));
	for (i=0; i<size; i++) {
		m_txmutex_init(&h->table_mutex[i]);
	}
    h->tablelength  = size;
    h->primeindex   = pindex;
	sloppycnt_uint32_init(&h->entrycount);
    h->hashfn       = hashf;
    h->eqfn         = eqf;
	//printf("h=%p\n",h);
	//printf("&h->entrycoun=%pt\n", &h->entrycount); fflush(stdout);
	//printf("&(h->eqfn)=%p\n",&(h->eqfn));
	//printf("h->hashfn=%p\n",h->hashfn);
	//printf("h->eqfn=%p\n",h->eqfn);
    h->loadlimit    = (unsigned int) ceil(size * max_load_factor);
    return h;
}

/*****************************************************************************/
unsigned int
hash(struct hashtable *h, void *k)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    unsigned int i = h->hashfn(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}

/*****************************************************************************/
__attribute__((tm_callable))
static int
hashtable_expand(struct hashtable *h)
{
	/* TODO: Currently we don't support expanding a multithreaded hash table */
	assert(0);
    /* Double the size of the table to accomodate more entries */
    struct entry **newtable;
    struct entry *e;
    struct entry **pE;
    unsigned int newsize, i, index;
    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (struct entry **)pmalloc(sizeof(struct entry*) * newsize);
    if (NULL != newtable)
    {
        memset(newtable, 0, newsize * sizeof(struct entry *));
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->tablelength; i++) {
            while (NULL != (e = h->table[i])) {
                h->table[i] = e->next;
                index = indexFor(newsize,e->h);
                e->next = newtable[index];
                newtable[index] = e;
            }
        }
        pfree(h->table);
        h->table = newtable;
    }
    /* Plan B: realloc instead */
    else 
    {
        newtable = (struct entry **)
                   realloc(h->table, newsize * sizeof(struct entry *));
        if (NULL == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->tablelength], 0, newsize - h->tablelength);
        for (i = 0; i < h->tablelength; i++) {
            for (pE = &(newtable[i]), e = *pE; e != NULL; e = *pE) {
                index = indexFor(newsize,e->h);
                if (index == i)
                {
                    pE = &(e->next);
                }
                else
                {
                    *pE = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }
    h->tablelength = newsize;
    h->loadlimit   = (unsigned int) ceil(newsize * max_load_factor);
    return -1;
}

/*****************************************************************************/
/* Not thread safe */
unsigned int
hashtable_count(struct hashtable *h)
{
	return h->entrycount.global.cnt;
}

/*****************************************************************************/
__attribute__((tm_callable))
static inline
int
hashtable_insert_internal(int tid, struct hashtable *h, void *k, void *v, int thread_safe)
{
    /* This method allows duplicate keys - but they shouldn't be used */
    unsigned int index;
    struct entry *e;

	//printf("[%d] hashtable_insert: \n", tid); fflush(stdout);
	//printf("[%d] hashtable_insert: &h->entrycount=%p\n", tid, &h->entrycount); fflush(stdout);
	//printf("[%d] hashtable_insert: h->eqfn=%p\n", tid, h->eqfn); fflush(stdout);
	//printf("[%d] hashtable_insert: h->hashfn=%p\n", tid, h->hashfn); fflush(stdout);
	sloppycnt_uint32_add(tid, &h->entrycount, 1, thread_safe);
    if (h->entrycount.global.cnt > h->loadlimit)
	///if (unsigned_int_fetch_and_add_full (&h->entrycount, 1) > h->loadlimit)
    {
        /* Ignore the return value. If expand fails, we should
         * still try cramming just this value into the existing table
         * -- we may not have memory for a larger table, but one more
         * element may be ok. Next time we insert, we'll try expanding again.*/
        hashtable_expand(h);
    }
    e = (struct entry *)pmalloc(sizeof(struct entry));
    if (NULL == e) { 
		//--(h->entrycount); 
		///unsigned_int_fetch_and_add_full (&h->entrycount, (unsigned int) -1);
		return 0; 
	} /*oom*/
    e->h = hash(h,k);
    index = indexFor(h->tablelength,e->h);
	//printf("hashtable_insert: key = "); print_obj(k);
	//printf("hashtable_insert: k = %p\n", k);
	//printf("hashtable_insert: e->h = %u\n", e->h);
	//printf("hashtable_insert: index = %u\n", index);
	//printf("hashtable_insert: tablelength = %u\n", h->tablelength);
    e->k = k;
    e->v = v;
	if (thread_safe) {
		m_txmutex_lock(&h->table_mutex[index]);
	}	
	//printf("[%d] hashtable_insert: index=%d\n", tid, index); fflush(stdout);
	//printf("[%d] hashtable_insert: e=%p\n", tid, e); fflush(stdout);
	//printf("[%d] hashtable_insert: e->next=%p\n", tid, e->next); fflush(stdout);
    e->next = h->table[index];
    h->table[index] = e;
	if (thread_safe) {
		m_txmutex_unlock(&h->table_mutex[index]);
	}	
	//printf("[%d] hashtable_insert: DONE\n", tid); fflush(stdout);
    return -1;
}

int
hashtable_insert(int tid, struct hashtable *h, void *k, void *v)
{
	return hashtable_insert_internal(tid, h, k, v, 1);
}


int
hashtable_insert_nolock(int tid, struct hashtable *h, void *k, void *v)
{
	return hashtable_insert_internal(tid, h, k, v, 0);
}


/*****************************************************************************/
__attribute__((tm_callable))
static inline
void * /* returns value associated with key */
hashtable_search_internal(int tid, struct hashtable *h, void *k, int thread_safe)
{
	unsigned int n;
    struct entry *e;
    struct entry *first_e;
    unsigned int hashvalue, index;
    hashvalue = hash(h,k);
	
	//printf("[%d] hashtable_search: \n", tid);
	//printf("hashtable_search: hashvalue = %u\n", hashvalue);
	//printf("hashtable_search: tablelength = %u\n", h->tablelength);

    index = indexFor(h->tablelength, hashvalue);
    e = h->table[index];
	first_e = e;
	//printf("hashtable_search: index = %u\n", index);
	if (thread_safe) {
		m_txmutex_lock(&h->table_mutex[index]);
	}	
	n = 0;
    while (NULL != e)
    {	
		n++;
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k))) { 
			if (thread_safe) {
				m_txmutex_unlock(&h->table_mutex[index]);
			}
			return e->v;
		}	
        e = e->next;
		if (e==first_e) {
			printf("loop\n");
			abort();
		}
    }
	if (thread_safe) {
		m_txmutex_unlock(&h->table_mutex[index]);
	}	
    return NULL;
}


void * /* returns value associated with key */
hashtable_search(int tid, struct hashtable *h, void *k)
{
	return hashtable_search_internal(tid, h, k, 1);
}


void * /* returns value associated with key */
hashtable_search_nolock(int tid, struct hashtable *h, void *k)
{
	return hashtable_search_internal(tid, h, k, 0);
}


/*****************************************************************************/
__attribute__((tm_callable))
static inline
void * /* returns value associated with key */
hashtable_remove_internal(int tid, struct hashtable *h, void *k, int thread_safe)
{
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */

    struct entry *e;
    struct entry **pE;
    void *v;
    unsigned int hashvalue, index;

	//printf("[%d] hashtable_remove: \n", tid); fflush(stdout);
    hashvalue = hash(h,k);
	/* FIXME: This assumes that tablelength does not change, which is true 
	 * only when the table is not expanding. THis is okay for now as we don't
	 * support table expansion.
	 */
    index = indexFor(h->tablelength,hash(h,k));
	if (thread_safe) {
		m_txmutex_lock(&h->table_mutex[index]);
	}	
	//printf("[%d] hashtable_remove: got lock: index=%d\n", tid, index); fflush(stdout);
    pE = &(h->table[index]);
    e = *pE;
    while (NULL != e)
    {
		//printf("[%d] hashtable_remove: NEXT: e=%p\n", tid, e); fflush(stdout);
		//printf("[%d] hashtable_remove: NEXT: e->k=%p\n", tid, e->k); fflush(stdout);
		//printf("[%d] hashtable_remove: NEXT: k=%p\n", tid, k); fflush(stdout);
		//print_obj(k);
		//printf("[%d] hashtable_remove: NEXT: eqfn=%p\n", tid, h->eqfn); fflush(stdout);
		//printf("[%d] hashtable_remove: NEXT: eqfn=%p\n", tid, h->eqfn(k, e->k)); fflush(stdout);
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k)))
        {
			//printf("[%d] hashtable_remove: FOUND\n", tid); fflush(stdout);
            *pE = e->next;
			if (thread_safe) {
				m_txmutex_unlock(&h->table_mutex[index]);
			}	
			sloppycnt_uint32_sub(tid, &h->entrycount, 1, thread_safe);
            //h->entrycount--;
			///unsigned_int_fetch_and_add_full (&h->entrycount, (unsigned int) -1);
            v = e->v;
            //freekey(e->k); // What the heck? This is not a clean API: the programmer 
			                 // allocated it, therefore the programmer should be 
							 // responsible deallocating it.
			//FIXME				 
            //pfree(e);
			//printf("[%d] hashtable_remove: FOUND...DONE\n", tid); fflush(stdout);
            return v;
        }
		//printf("[%d] hashtable_remove: NEXT..\n", tid); fflush(stdout);
        pE = &(e->next);
        e = e->next;
		//printf("[%d] hashtable_remove: NEXT...\n", tid); fflush(stdout);
    }
	if (thread_safe) {
		m_txmutex_unlock(&h->table_mutex[index]);
	}	
	//printf("[%d] hashtable_remove: NOTFOUND...DONE\n", tid); fflush(stdout);
    return NULL;
}


void * /* returns value associated with key */
hashtable_remove(int tid, struct hashtable *h, void *k)
{
	return hashtable_remove_internal(tid, h, k, 1);
}


void * /* returns value associated with key */
hashtable_remove_nolock(int tid, struct hashtable *h, void *k)
{
	return hashtable_remove_internal(tid, h, k, 0);
}


/*****************************************************************************/
/* destroy */
void
hashtable_destroy(struct hashtable *h, int free_values)
{
    unsigned int i;
    struct entry *e, *f;
    struct entry **table = h->table;
    if (free_values)
    {
        for (i = 0; i < h->tablelength; i++)
        {
            e = table[i];
            while (NULL != e)
            { f = e; e = e->next; freekey(f->k); pfree(f->v); pfree(f); }
        }
    }
    else
    {
        for (i = 0; i < h->tablelength; i++)
        {
            e = table[i];
            while (NULL != e)
            { f = e; e = e->next; freekey(f->k); pfree(f); }
        }
    }
    pfree(h->table);
    pfree(h);
}


/*****************************************************************************/
uint64_t hashtable_metadata_size(struct hashtable *h)
{
	unsigned int size = h->tablelength;
	uint64_t     footprint;
	unsigned int i=0;
	unsigned int num_entries=0;

	footprint  = 0; 
	footprint += sizeof(struct hashtable);
	footprint += sizeof(m_txmutex_t) * size;   /* mutex table */
	footprint += sizeof(struct entry*) * size; /* table */
	num_entries = hashtable_count(h);
	footprint += num_entries * sizeof(struct entry);
}
/*
 * Copyright (c) 2002, Christopher Clark
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
