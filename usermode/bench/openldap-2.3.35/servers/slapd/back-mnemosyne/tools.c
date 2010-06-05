/* tools.c - tools for slap tools */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/tools.c,v 1.43.2.5 2007/01/02 21:44:03 kurt Exp $ */
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

static MNEMOSYNEDBMCursor *cursorp = NULL;
static DBCache *id2entry = NULL;

int mnemosynedbm_tool_entry_open(
	BackendDB *be, int mode )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;
	int flags;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry == NULL );

	switch( mode ) {
	case 1:
		flags = MNEMOSYNEDBM_WRCREAT;
		break;
	case 2:
#ifdef TRUNCATE_MODE
		flags = MNEMOSYNEDBM_NEWDB;
#else
		flags = MNEMOSYNEDBM_WRCREAT;
#endif
		break;
	default:
		flags = MNEMOSYNEDBM_READER;
	}

	li->li_dbwritesync = 0;

	if ( (id2entry = mnemosynedbm_cache_open( be, "id2entry", MNEMOSYNEDBM_SUFFIX, flags ))
	    == NULL ) {
		Debug( LDAP_DEBUG_ANY, "Could not open/create id2entry" MNEMOSYNEDBM_SUFFIX "\n",
		    0, 0, 0 );

		return( -1 );
	}

	return 0;
}

int mnemosynedbm_tool_entry_close(
	BackendDB *be )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	mnemosynedbm_cache_close( be, id2entry );
	li->li_dbwritesync = 1;
	id2entry = NULL;

	return 0;
}

ID mnemosynedbm_tool_entry_first(
	BackendDB *be )
{
	Datum key;
	ID id;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	key = mnemosynedbm_firstkey( id2entry->dbc_db, &cursorp );

	if( key.dptr == NULL ) {
		return NOID;
	}

	AC_MEMCPY( &id, key.dptr, key.dsize );
#ifndef WORDS_BIGENDIAN
	id = ntohl( id );
#endif

	mnemosynedbm_datum_free( id2entry->dbc_db, key );

	return id;
}

ID mnemosynedbm_tool_entry_next(
	BackendDB *be )
{
	Datum key;
	ID id;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	/* allow for NEXTID */
	mnemosynedbm_datum_init( key );

	key = mnemosynedbm_nextkey( id2entry->dbc_db, key, cursorp );

	if( key.dptr == NULL ) {
		return NOID;
	}

	AC_MEMCPY( &id, key.dptr, key.dsize );
#ifndef WORDS_BIGENDIAN
	id = ntohl( id );
#endif

	mnemosynedbm_datum_free( id2entry->dbc_db, key );

	return id;
}

Entry* mnemosynedbm_tool_entry_get( BackendDB *be, ID id )
{
	Entry *e;
	Datum key, data;
#ifndef WORDS_BIGENDIAN
	ID id2;
#endif
	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	mnemosynedbm_datum_init( key );

#ifndef WORDS_BIGENDIAN
	id2 = htonl( id );
	key.dptr = (char *) &id2;
#else
	key.dptr = (char *) &id;
#endif
	key.dsize = sizeof(ID);

	data = mnemosynedbm_cache_fetch( id2entry, key );

	if ( data.dptr == NULL ) {
		return NULL;
	}

	e = str2entry2( data.dptr, 0 );
	mnemosynedbm_datum_free( id2entry->dbc_db, data );

	if( e != NULL ) {
		e->e_id = id;
	}

	return e;
}

ID mnemosynedbm_tool_entry_put(
	BackendDB *be,
	Entry *e,
	struct berval *text )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;
	Datum key, data;
	int rc, len;
	ID id;
	Operation op = {0};
	Opheader ohdr = {0};

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	assert( text != NULL );
	assert( text->bv_val != NULL );
	assert( text->bv_val[0] == '\0' );	/* overconservative? */

	if ( m_next_id_get( be, &id ) || id == NOID ) {
		strncpy( text->bv_val, "unable to get nextid", text->bv_len );
		return NOID;
	}

	e->e_id = li->li_nextid++;

	Debug( LDAP_DEBUG_TRACE, "=> mnemosynedbm_tool_entry_put( %ld, \"%s\" )\n",
		e->e_id, e->e_dn, 0 );

	if ( m_dn2id( be, &e->e_nname, &id ) ) {
		/* something bad happened to mnemosynedbm cache */
		strncpy( text->bv_val, "mnemosynedbm cache corrupted", text->bv_len );
		return NOID;
	}

	if( id != NOID ) {
		Debug( LDAP_DEBUG_TRACE,
			"<= mnemosynedbm_tool_entry_put: \"%s\" already exists (id=%ld)\n",
			e->e_ndn, id, 0 );
		strncpy( text->bv_val, "already exists", text->bv_len );
		return NOID;
	}

	op.o_hdr = &ohdr;
	op.o_bd = be;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;

	rc = m_index_entry_add( &op, e );
	if( rc != 0 ) {
		strncpy( text->bv_val, "index add failed", text->bv_len );
		return NOID;
	}

	rc = m_dn2id_add( be, &e->e_nname, e->e_id );
	if( rc != 0 ) {
		strncpy( text->bv_val, "m_dn2id add failed", text->bv_len );
		return NOID;
	}

	mnemosynedbm_datum_init( key );
	mnemosynedbm_datum_init( data );

#ifndef WORDS_BIGENDIAN
	id = htonl( e->e_id );
	key.dptr = (char *) &id;
#else
	key.dptr = (char *) &e->e_id;
#endif
	key.dsize = sizeof(ID);

	data.dptr = entry2str( e, &len );
	data.dsize = len + 1;

	/* store it */
	rc = mnemosynedbm_cache_store( id2entry, key, data, MNEMOSYNEDBM_REPLACE );

	if( rc != 0 ) {
		(void) m_dn2id_delete( be, &e->e_nname, e->e_id );
		strncpy( text->bv_val, "cache store failed", text->bv_len );
		return NOID;
	}

	return e->e_id;
}

int mnemosynedbm_tool_entry_reindex(
	BackendDB *be,
	ID id )
{
	int rc;
	Entry *e;
	Operation op = {0};
	Opheader ohdr = {0};

	Debug( LDAP_DEBUG_ARGS, "=> mnemosynedbm_tool_entry_reindex( %ld )\n",
		(long) id, 0, 0 );


	e = mnemosynedbm_tool_entry_get( be, id );

	if( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"mnemosynedbm_tool_entry_reindex:: could not locate id=%ld\n",
			(long) id, 0, 0 );

		return -1;
	}

	/*
	 * just (re)add them for now
	 * assume that some other routine (not yet implemented)
	 * will zap index databases
	 *
	 */

	Debug( LDAP_DEBUG_TRACE, "=> mnemosynedbm_tool_entry_reindex( %ld, \"%s\" )\n",
		id, e->e_dn, 0 );

	m_dn2id_add( be, &e->e_nname, e->e_id );

	op.o_hdr = &ohdr;
	op.o_bd = be;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;
	rc = m_index_entry_add( &op, e );

	entry_free( e );

	return rc;
}

int mnemosynedbm_tool_sync( BackendDB *be )
{
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;

	assert( slapMode & SLAP_TOOL_MODE );

	if ( li->li_nextid != NOID ) {
		if ( m_next_id_write( be, li->li_nextid ) ) {
			return( -1 );
		}
	}

	return 0;
}
