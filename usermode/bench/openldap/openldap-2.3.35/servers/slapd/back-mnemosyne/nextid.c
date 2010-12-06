/* nextid.c - keep track of the next id to be given out */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/nextid.c,v 1.36.2.4 2007/01/02 21:44:03 kurt Exp $ */
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

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/param.h>

#include "slap.h"
#include "back-mnemosynedbm.h"

static int
next_id_read( Backend *be, ID *idp )
{
	Datum key, data;
	DBCache *db;

	*idp = NOID;

	if ( (db = mnemosynedbm_cache_open( be, "nextid", MNEMOSYNEDBM_SUFFIX, MNEMOSYNEDBM_WRCREAT ))
	    == NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open/create nextid" MNEMOSYNEDBM_SUFFIX "\n",
			0, 0, 0 );

		return( -1 );
	}

	mnemosynedbm_datum_init( key );
	key.dptr = (char *) idp;
	key.dsize = sizeof(ID);

	data = mnemosynedbm_cache_fetch( db, key );

	if( data.dptr != NULL ) {
		AC_MEMCPY( idp, data.dptr, sizeof( ID ) );
		mnemosynedbm_datum_free( db->dbc_db, data );

	} else {
		*idp = 1;
	}

	mnemosynedbm_cache_close( be, db );
	return( 0 );
}

int
m_next_id_write( Backend *be, ID id )
{
	Datum key, data;
	DBCache *db;
	ID noid = NOID;
	int flags, rc = 0;

	if ( (db = mnemosynedbm_cache_open( be, "nextid", MNEMOSYNEDBM_SUFFIX, MNEMOSYNEDBM_WRCREAT ))
	    == NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open/create nextid" MNEMOSYNEDBM_SUFFIX "\n",
		    0, 0, 0 );

		return( -1 );
	}

	mnemosynedbm_datum_init( key );
	mnemosynedbm_datum_init( data );

	key.dptr = (char *) &noid;
	key.dsize = sizeof(ID);

	data.dptr = (char *) &id;
	data.dsize = sizeof(ID);

	flags = MNEMOSYNEDBM_REPLACE;
	if ( mnemosynedbm_cache_store( db, key, data, flags ) != 0 ) {
		rc = -1;
	}

	mnemosynedbm_cache_close( be, db );
	return( rc );
}

int
m_next_id_get( Backend *be, ID *idp )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;
	int rc = 0;

	*idp = NOID;

	if ( li->li_nextid == NOID ) {
		if ( ( rc = next_id_read( be, idp ) ) ) {
			return( rc );
		}
		li->li_nextid = *idp;
	}

	*idp = li->li_nextid;

	return( rc );
}

int
m_next_id( Backend *be, ID *idp )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;
	int rc = 0;

	if ( li->li_nextid == NOID ) {
		if ( ( rc = next_id_read( be, idp ) ) ) {
			return( rc );
		}
		li->li_nextid = *idp;
	}

	*idp = li->li_nextid++;
	if ( m_next_id_write( be, li->li_nextid ) ) {
		rc = -1;
	}

	return( rc );
}
