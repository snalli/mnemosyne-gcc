/**
 * \file chhash.h
 *
 * \brief Simple bucket hash table interface.
 */

#ifndef _M_HASH_H
#define _M_HASH_H

#include "mtypes.h"
#include "result.h"

#define HASH_FACTOR 4

/* Opaque structure used to represent hash table. */
typedef struct m_chhash_s m_chhash_t;

/* Opaque structure used to represent hash table iterator. */
typedef struct m_chhash_iter_s m_chhash_iter_t;

typedef unsigned int m_chhash_key_t;
typedef void *m_chhash_value_t;
typedef struct m_chhash_bucket_s m_chhash_bucket_t;

struct m_chhash_iter_s {
	m_chhash_t        *chhash;
	unsigned int      index;
	m_chhash_bucket_t *bucket;
};


m_result_t m_chhash_create(m_chhash_t**, unsigned int, bool);
m_result_t m_chhash_destroy(m_chhash_t**);
m_result_t m_chhash_add(m_chhash_t*, m_chhash_key_t, m_chhash_value_t);
m_result_t m_chhash_lookup(m_chhash_t*, m_chhash_key_t, m_chhash_value_t *);
m_result_t m_chhash_remove(m_chhash_t*, m_chhash_key_t, m_chhash_value_t *);
void m_chhash_iter_init(m_chhash_t *chhash, m_chhash_iter_t *iter);
m_result_t m_chhash_iter_next(m_chhash_iter_t *iter, m_chhash_key_t *key, m_chhash_value_t *value);
void m_chhash_print(m_chhash_t *);
#endif
