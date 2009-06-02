/* id2entry.c - routines to deal with the id2entry index */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/id2entry.c,v 1.38.2.4 2007/01/02 21:44:02 kurt Exp $ */
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
#include "back-mnemosynedbm.h"

/*
 * This routine adds (or updates) an entry on disk.
 * The cache should already be updated. 
 */

int
m_id2entry_add( Backend *be, Entry *e )
{
	DBCache	*db;
	Datum		key, data;
	int		len, rc, flags;
#ifndef WORDS_BIGENDIAN
	ID		id;
#endif

	mnemosynedbm_datum_init( key );
	mnemosynedbm_datum_init( data );

	Debug( LDAP_DEBUG_TRACE, "=> id2entry_add( %ld, \"%s\" )\n", e->e_id,
	    e->e_dn, 0 );


	if ( (db = mnemosynedbm_cache_open( be, "id2entry", MNEMOSYNEDBM_SUFFIX, MNEMOSYNEDBM_WRCREAT ))
	    == NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open/create id2entry%s\n",
		    MNEMOSYNEDBM_SUFFIX, 0, 0 );

		return( -1 );
	}

#ifdef WORDS_BIGENDIAN
	key.dptr = (char *) &e->e_id;
#else
	id = htonl(e->e_id);
	key.dptr = (char *) &id;
#endif
	key.dsize = sizeof(ID);

	ldap_pvt_thread_mutex_lock( &entry2str_mutex );
	data.dptr = entry2str( e, &len );
	data.dsize = len + 1;

	/* store it */
	flags = MNEMOSYNEDBM_REPLACE;
	rc = mnemosynedbm_cache_store( db, key, data, flags );

	ldap_pvt_thread_mutex_unlock( &entry2str_mutex );

	mnemosynedbm_cache_close( be, db );

	Debug( LDAP_DEBUG_TRACE, "<= id2entry_add %d\n", rc, 0, 0 );


	return( rc );
}

int
m_id2entry_delete( Backend *be, Entry *e )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;
	DBCache	*db;
	Datum		key;
	int		rc;
#ifndef WORDS_BIGENDIAN
	ID		id;
#endif

	Debug(LDAP_DEBUG_TRACE, "=> id2entry_delete( %ld, \"%s\" )\n", e->e_id,
	    e->e_dn, 0 );


#ifdef notdef
#ifdef LDAP_RDWR_DEBUG
	/* check for writer lock */
	assert(ldap_pvt_thread_rdwr_writers(&e->e_rdwr) == 1);
#endif
#endif

	mnemosynedbm_datum_init( key );

	if ( (db = mnemosynedbm_cache_open( be, "id2entry", MNEMOSYNEDBM_SUFFIX, MNEMOSYNEDBM_WRCREAT ))
		== NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open/create id2entry%s\n",
		    MNEMOSYNEDBM_SUFFIX, 0, 0 );

		return( -1 );
	}

	if ( m_cache_delete_entry( &li->li_cache, e ) != 0 ) {
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

	rc = mnemosynedbm_cache_delete( db, key );

	mnemosynedbm_cache_close( be, db );

	Debug( LDAP_DEBUG_TRACE, "<= id2entry_delete %d\n", rc, 0, 0 );

	return( rc );
}

/* returns entry with reader/writer lock */
Entry *
m_id2entry_rw( Backend *be, ID id, int rw )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;
	DBCache	*db;
	Datum		key, data;
	Entry		*e;
#ifndef WORDS_BIGENDIAN
	ID		id2;
#endif

	mnemosynedbm_datum_init( key );
	mnemosynedbm_datum_init( data );

	Debug( LDAP_DEBUG_TRACE, "=> id2entry_%s( %ld )\n",
		rw ? "w" : "r", id, 0 );


	if ( (e = m_cache_find_entry_id( &li->li_cache, id, rw )) != NULL ) {
		Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) 0x%lx (cache)\n",
			rw ? "w" : "r", id, (unsigned long) e );

		return( e );
	}

	if ( (db = mnemosynedbm_cache_open( be, "id2entry", MNEMOSYNEDBM_SUFFIX, MNEMOSYNEDBM_WRCREAT ))
		== NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open id2entry%s\n",
		    MNEMOSYNEDBM_SUFFIX, 0, 0 );

		return( NULL );
	}

#ifdef WORDS_BIGENDIAN
	key.dptr = (char *) &id;
#else
	id2 = htonl(id);
	key.dptr = (char *) &id2;
#endif
	key.dsize = sizeof(ID);

	data = mnemosynedbm_cache_fetch( db, key );

	if ( data.dptr == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) not found\n",
			rw ? "w" : "r", id, 0 );

		mnemosynedbm_cache_close( be, db );
		return( NULL );
	}

	e = str2entry2( data.dptr, 0 );
	mnemosynedbm_datum_free( db->dbc_db, data );
	mnemosynedbm_cache_close( be, db );

	if ( e == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) (failed)\n",
			rw ? "w" : "r", id, 0 );

		return( NULL );
	}

	e->e_id = id;

	if ( slapMode == SLAP_SERVER_MODE
			&& m_cache_add_entry_rw( &li->li_cache, e, rw ) != 0 )
	{
		entry_free( e );

		/* XXX this is a kludge.
		 * maybe the entry got added underneath us
		 * There are many underlying race condtions in the cache/disk code.
		 */
		if ( (e = m_cache_find_entry_id( &li->li_cache, id, rw )) != NULL ) {
			Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) 0x%lx (cache)\n",
				rw ? "w" : "r", id, (unsigned long) e );

			return( e );
		}

		Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) (cache add failed)\n",
			rw ? "w" : "r", id, 0 );

		return NULL;
	}

	Debug( LDAP_DEBUG_TRACE, "<= id2entry_%s( %ld ) 0x%lx (disk)\n",
		rw ? "w" : "r", id, (unsigned long) e );

	if ( slapMode == SLAP_SERVER_MODE ) {
		/* marks the entry as committed, so it will get added to the cache
		 * when the lock is released */
		m_cache_entry_commit( e );
	}

	return( e );
}
