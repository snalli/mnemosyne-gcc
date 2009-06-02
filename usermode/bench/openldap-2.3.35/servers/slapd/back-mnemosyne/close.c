/* close.c - close mnemosynedbm backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/close.c,v 1.19.2.6 2007/01/02 21:44:02 kurt Exp $ */
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

int
mnemosynedbm_back_db_close( Backend *be )
{
	struct mnemosynedbminfo *li = be->be_private;

	Debug( LDAP_DEBUG_TRACE, "mnemosynedbm backend syncing\n", 0, 0, 0 );

	mnemosynedbm_cache_flush_all( be );
	Debug( LDAP_DEBUG_TRACE, "mnemosynedbm backend done syncing\n", 0, 0, 0 );

	m_cache_release_all( &li->li_cache );
	if ( alock_close( &li->li_alock_info )) {
		Debug( LDAP_DEBUG_ANY,
			"mnemosynedbm_back_db_close: alock_close failed\n", 0, 0, 0 );
	}

	return 0;
}
