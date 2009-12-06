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

#endif /* _HASH_TABLE_H */
