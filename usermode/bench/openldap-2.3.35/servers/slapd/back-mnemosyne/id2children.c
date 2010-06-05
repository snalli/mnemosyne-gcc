/* id2children.c - routines to deal with the id2children index */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/id2children.c,v 1.31.2.3 2007/01/02 21:44:02 kurt Exp $ */
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

#include "slap.h"
#include "back-mnemosynedbm.h"

int
m_has_children(
    Backend	*be,
    Entry	*p
)
{
	DBCache	*db;
	Datum		key;
	int		rc = 0;
	ID_BLOCK		*idl;

	mnemosynedbm_datum_init( key );

	Debug( LDAP_DEBUG_TRACE, "=> has_children( %ld )\n", p->e_id , 0, 0 );


	if ( (db = mnemosynedbm_cache_open( be, "dn2id", MNEMOSYNEDBM_SUFFIX,
	    MNEMOSYNEDBM_WRCREAT )) == NULL ) {
		Debug( LDAP_DEBUG_ANY,
		    "<= has_children -1 could not open \"dn2id%s\"\n",
		    MNEMOSYNEDBM_SUFFIX, 0, 0 );

		return( 0 );
	}

	key.dsize = strlen( p->e_ndn ) + 2;
	key.dptr = ch_malloc( key.dsize );
	sprintf( key.dptr, "%c%s", DN_ONE_PREFIX, p->e_ndn );

	idl = m_idl_fetch( be, db, key );

	free( key.dptr );

	mnemosynedbm_cache_close( be, db );

	if( idl != NULL ) {
		m_idl_free( idl );
		rc = 1;
	}

	Debug( LDAP_DEBUG_TRACE, "<= has_children( %ld ): %s\n",
		p->e_id, rc ? "yes" : "no", 0 );

	return( rc );
}
