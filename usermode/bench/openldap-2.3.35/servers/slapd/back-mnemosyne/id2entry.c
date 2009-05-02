/* id2entry.c - routines to deal with the id2entry index */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/id2entry.c,v 1.38.2.4 2007/01/02 21:44:02 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2007 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"
#include "back-ldbm.h"

#include "../mnemosyne.h"
#include "list.h"

/*
 * This routine adds (or updates) an entry on disk.
 * The cache should already be updated. 
 */

typedef struct pentry_list_s pentry_list_t;

struct pentry_list_s {
	struct berval bv;
	char *str;
	ID id;
	int len;
	struct list_head list;
};

MNEMOSYNE_PERSISTENT struct list_head pentry_list;
MNEMOSYNE_PERSISTENT int pentry_list_init = 0;


void
reincarnate_id2entry_cache(Cache *li_cache)
{
	pentry_list_t *le_iter;

	Entry		*edup;
	int i;
	char *str;

	if (pentry_list_init == 0) {
		INIT_LIST_HEAD(&pentry_list);
		pentry_list_init = 1;
		return;
	}

	list_for_each_entry(le_iter, &pentry_list, list) {
#if 1
		entry_decode(&le_iter->bv, &edup);
		edup->e_id = le_iter->id;
#else
		str = (char *) malloc(le->len+1);
		strcpy(str, le->str);
		edup = str2entry2( str, 0 );
#endif
		assert(cache_add_entry_rw( li_cache, edup, 0 ) == 0 );
		cache_entry_commit( edup );
		cache_return_entry_rw(li_cache, edup, 0);
	}
}


int
id2entry_add( Backend *be, Entry *e )
{
	DBCache	*db;
	Datum		key, data;
	int		len, rc, flags;
#ifndef WORDS_BIGENDIAN
	ID		id;
#endif
	Entry *edup;

	int i;
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;

	pentry_list_t *le;
	pentry_list_t *le_iter;
	char *str;
	char *str1;
	char *str2;

	printf("BEFORE_ADD:\n");
	le = (pentry_list_t *) pmalloc(sizeof(pentry_list_t));

# if 1
	entry_pencode(e, &le->bv);
	le->id = e->e_id;

# else
	str = entry2str( e, &len );
	le->str = (char *) pmalloc(len+1);
	le->id = e->e_id;
	le->len = len;
	memcpy(le->str, str, len+1);
	printf("str = %p, len = %d\n %s\n", le->str, len, le->str);
# endif

	list_add(&(le->list), &pentry_list);

	return 0;
}

int
id2entry_delete( Backend *be, Entry *e )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	DBCache	*db;
	Datum		key;
	int		rc;
#ifndef WORDS_BIGENDIAN
	ID		id;
#endif

	Debug(LDAP_DEBUG_TRACE, "=> id2entry_delete( %ld, \"%s\" )\n", e->e_id,
	    e->e_dn, 0 );

	assert(0);

#ifdef notdef
#ifdef LDAP_RDWR_DEBUG
	/* check for writer lock */
	assert(ldap_pvt_thread_rdwr_writers(&e->e_rdwr) == 1);
#endif
#endif

	ldbm_datum_init( key );

	if ( (db = ldbm_cache_open( be, "id2entry", LDBM_SUFFIX, LDBM_WRCREAT ))
		== NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open/create id2entry%s\n",
		    LDBM_SUFFIX, 0, 0 );

		return( -1 );
	}

	if ( cache_delete_entry( li->li_cache, e ) != 0 ) {
		Debug(LDAP_DEBUG_ANY, "could not delete %ld (%s) from cache\n",
		    e->e_id, e->e_dn, 0 );

	}

#ifdef WORDS_BIGENDIAN
	key.dptr = (char *) &e->e_id;
#else
	id = htonl(e->e_id);
	key.dptr = (char *) &id;
#endif
	key.dsize = sizeof(ID);

	rc = ldbm_cache_delete( db, key );

	ldbm_cache_close( be, db );

	Debug( LDAP_DEBUG_TRACE, "<= id2entry_delete %d\n", rc, 0, 0 );

	return( rc );
}

void 
print_entry(Entry	*e);


/* returns entry with reader/writer lock */
Entry *
id2entry_rw( Backend *be, ID id, int rw )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	DBCache	*db;
	Datum		key, data;
	Entry		*e;
#ifndef WORDS_BIGENDIAN
	ID		id2;
#endif
	Entry *edup;

	Debug( LDAP_DEBUG_TRACE, "=> id2entry_%s( %ld )\n",
		rw ? "w" : "r", id, 0 );

	if ( (e = cache_find_entry_id( li->li_cache, id, rw )) != NULL ) {
		Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) 0x%lx (cache)\n",
			rw ? "w" : "r", id, (unsigned long) e );

		return( e );
	}
	
	return NULL;
}
