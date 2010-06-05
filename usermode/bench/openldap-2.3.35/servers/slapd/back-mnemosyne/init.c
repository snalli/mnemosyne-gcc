/* init.c - initialize mnemosynedbm backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/init.c,v 1.96.2.8 2007/01/02 21:44:02 kurt Exp $ */
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
#include <ldap_rq.h>

int
mnemosyne_back_initialize(
    BackendInfo	*bi
)
{
	static char *controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_X_PERMISSIVE_MODIFY,
		NULL
	};

	bi->bi_controls = controls;

	bi->bi_flags |= 
		SLAP_BFLAG_INCREMENT |
#ifdef MNEMOSYNEDBM_SUBENTRIES
		SLAP_BFLAG_SUBENTRIES |
#endif
		SLAP_BFLAG_ALIASES |
		SLAP_BFLAG_REFERRALS;

	bi->bi_open = mnemosynedbm_back_open;
	bi->bi_config = NULL;
	bi->bi_close = mnemosynedbm_back_close;
	bi->bi_destroy = mnemosynedbm_back_destroy;

	bi->bi_db_init = mnemosynedbm_back_db_init;
	bi->bi_db_config = mnemosynedbm_back_db_config;
	bi->bi_db_open = mnemosynedbm_back_db_open;
	bi->bi_db_close = mnemosynedbm_back_db_close;
	bi->bi_db_destroy = mnemosynedbm_back_db_destroy;

	bi->bi_op_bind = mnemosynedbm_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = mnemosynedbm_back_search;
	bi->bi_op_compare = mnemosynedbm_back_compare;
	bi->bi_op_modify = mnemosynedbm_back_modify;
	bi->bi_op_modrdn = mnemosynedbm_back_modrdn;
	bi->bi_op_add = mnemosynedbm_back_add;
	bi->bi_op_delete = mnemosynedbm_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = mnemosynedbm_back_extended;

	bi->bi_entry_release_rw = mnemosynedbm_back_entry_release_rw;
	bi->bi_entry_get_rw = mnemosynedbm_back_entry_get;
	bi->bi_chk_referrals = mnemosynedbm_back_referrals;
	bi->bi_operational = mnemosynedbm_back_operational;
	bi->bi_has_subordinates = mnemosynedbm_back_hasSubordinates;

	/*
	 * hooks for slap tools
	 */
	bi->bi_tool_entry_open = mnemosynedbm_tool_entry_open;
	bi->bi_tool_entry_close = mnemosynedbm_tool_entry_close;
	bi->bi_tool_entry_first = mnemosynedbm_tool_entry_first;
	bi->bi_tool_entry_next = mnemosynedbm_tool_entry_next;
	bi->bi_tool_entry_get = mnemosynedbm_tool_entry_get;
	bi->bi_tool_entry_put = mnemosynedbm_tool_entry_put;
	bi->bi_tool_entry_reindex = mnemosynedbm_tool_entry_reindex;
	bi->bi_tool_sync = mnemosynedbm_tool_sync;

	bi->bi_tool_dn2id_get = 0;
	bi->bi_tool_id2entry_get = 0;
	bi->bi_tool_entry_modify = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return 0;
}

int
mnemosynedbm_back_destroy(
    BackendInfo	*bi
)
{
	return 0;
}

int
mnemosynedbm_back_open(
    BackendInfo	*bi
)
{
	int rc;

	/* initialize the underlying database system */
	rc = mnemosynedbm_initialize( NULL );
	return rc;
}

int
mnemosynedbm_back_close(
    BackendInfo	*bi
)
{
	/* terminate the underlying database system */
	mnemosynedbm_shutdown();
	return 0;
}

int
mnemosynedbm_back_db_init(
    Backend	*be
)
{
	struct mnemosynedbminfo	*li;

	/* allocate backend-database-specific stuff */
	li = (struct mnemosynedbminfo *) ch_calloc( 1, sizeof(struct mnemosynedbminfo) );

	/* arrange to read nextid later (on first request for it) */
	li->li_nextid = NOID;

	/* default cache size */
	li->li_cache.c_maxsize = DEFAULT_CACHE_SIZE;

	/* default database cache size */
	li->li_dbcachesize = DEFAULT_DBCACHE_SIZE;

	/* default db mode is with locking */ 
	li->li_dblocking = 1;

	/* default db mode is with write synchronization */ 
	li->li_dbwritesync = 1;

	/* default file creation mode */
	li->li_mode = SLAPD_DEFAULT_DB_MODE;

	/* default database directory */
	li->li_directory = ch_strdup( SLAPD_DEFAULT_DB_DIR );

	/* DB_ENV environment pointer for DB3 */
	li->li_dbenv = 0;

	/* envdirok is turned on by mnemosynedbm_initialize_env if DB3 */
	li->li_envdirok = 0;

	/* syncfreq is 0 if disabled, or # seconds */
	li->li_dbsyncfreq = 0;

	/* wait up to dbsyncwaitn times if server is busy */
	li->li_dbsyncwaitn = 12;

	/* delay interval */
	li->li_dbsyncwaitinterval = 5;

	/* current wait counter */
	li->li_dbsyncwaitcount = 0;

	/* initialize various mutex locks & condition variables */
	ldap_pvt_thread_rdwr_init( &li->li_giant_rwlock );
	ldap_pvt_thread_mutex_init( &li->li_cache.c_mutex );
	ldap_pvt_thread_mutex_init( &li->li_dbcache_mutex );
	ldap_pvt_thread_cond_init( &li->li_dbcache_cv );

	be->be_private = li;

	return 0;
}

int
mnemosynedbm_back_db_open(
    BackendDB	*be
)
{
	struct mnemosynedbminfo *li = (struct mnemosynedbminfo *) be->be_private;
	int rc;

	rc = alock_open( &li->li_alock_info, "slapd",
		li->li_directory, ALOCK_UNIQUE );
	if ( rc == ALOCK_BUSY ) {
		Debug( LDAP_DEBUG_ANY,
			"mnemosynedbm_back_db_open: database already in use\n",
			0, 0, 0 );
		return -1;
	} else if ( rc == ALOCK_RECOVER ) {
		Debug( LDAP_DEBUG_ANY,
			"mnemosynedbm_back_db_open: unclean shutdown detected;"
			" database may be inconsistent!\n",
			0, 0, 0 );
		rc = alock_recover( &li->li_alock_info );
	}
	if ( rc != ALOCK_CLEAN ) {
		Debug( LDAP_DEBUG_ANY,
			"mnemosynedbm_back_db_open: alock package is unstable;"
			" database may be inconsistent!\n",
			0, 0, 0 );
	}
	li->li_dbenv = mnemosynedbm_initialize_env( li->li_directory,
		li->li_dbcachesize, &li->li_envdirok );

	/* If we're in server mode and a sync frequency was set,
	 * submit a task to perform periodic db syncs.
	 */
	if (( slapMode & SLAP_SERVER_MODE ) && li->li_dbsyncfreq > 0 )
	{
		ldap_pvt_thread_mutex_lock( &slapd_rq.rq_mutex );
		ldap_pvt_runqueue_insert( &slapd_rq, li->li_dbsyncfreq,
			mnemosynedbm_cache_sync_daemon, be,
			"mnemosynedbm_cache_sync", be->be_suffix[0].bv_val );
		ldap_pvt_thread_mutex_unlock( &slapd_rq.rq_mutex );
	}

	return 0;
}

int
mnemosynedbm_back_db_destroy(
    BackendDB	*be
)
{
	/* should free/destroy every in be_private */
	struct mnemosynedbminfo	*li = (struct mnemosynedbminfo *) be->be_private;

	if (li->li_dbenv)
	    mnemosynedbm_shutdown_env(li->li_dbenv);

	free( li->li_directory );
	m_attr_index_destroy( li->li_attrs );

	ldap_pvt_thread_rdwr_destroy( &li->li_giant_rwlock );
	ldap_pvt_thread_mutex_destroy( &li->li_cache.c_mutex );
	ldap_pvt_thread_mutex_destroy( &li->li_dbcache_mutex );
	ldap_pvt_thread_cond_destroy( &li->li_dbcache_cv );

	free( be->be_private );
	be->be_private = NULL;

	return 0;
}

#if SLAPD_MNEMOSYNE == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( mnemosynedbm )

#endif /* SLAPD_MNEMOSYNE == SLAPD_MOD_DYNAMIC */


