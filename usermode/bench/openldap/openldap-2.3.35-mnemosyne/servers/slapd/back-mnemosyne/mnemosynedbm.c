/* mnemosynedbm.c - ldap dbm compatibility routines */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/mnemosynedbm.c,v 1.4.2.4 2007/01/02 21:44:03 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2007 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 1998-2001 Net Boolean Incorporated.
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
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).  Additional significant contributors
 * include:
 *   Gary Williams
 *   Howard Chu
 *   Juan Gomez
 *   Kurt D. Zeilenga
 *   Kurt Spanier
 *   Mark Whitehouse
 *   Randy Kundee
 */


#include "portable.h"

#ifdef SLAPD_MNEMOSYNE

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/errno.h>

#include <mnemosyne.h>
#include <itm.h>
#include "mnemosynedbm.h"
#include "ldap_pvt_thread.h"
#include "hash.h"
#include "hash-bits.h"
#include "list.h"

#include "stat.h"

void
mnemosynedbm_datum_free( MNEMOSYNEDBM mnemosynedbm, Datum data )
{
	if ( data.dptr ) {
		free( data.dptr );
		memset( &data, '\0', sizeof( Datum ));
		data.dptr = NULL;
	}
}

Datum
mnemosynedbm_datum_dup( MNEMOSYNEDBM mnemosynedbm, Datum data )
{
	Datum	dup;

	mnemosynedbm_datum_init( dup );

	if ( data.dsize == 0 ) {
		dup.dsize = 0;
		dup.dptr = NULL;

		return( dup );
	}
	dup.dsize = data.dsize;

	if ( (dup.dptr = (char *) pmalloc( data.dsize )) != NULL ) {
		AC_MEMCPY( dup.dptr, data.dptr, data.dsize );
	}

	return( dup );
}

static int mnemosynedbm_initialized = 0;

static ldap_pvt_thread_rdwr_t mnemosynedbm_big_rdwr;
#define MNEMOSYNEDBM_RWLOCK_INIT (ldap_pvt_thread_rdwr_init( &mnemosynedbm_big_rdwr ))
#define MNEMOSYNEDBM_RWLOCK_DESTROY (ldap_pvt_thread_rdwr_destroy( &mnemosynedbm_big_rdwr ))
#define MNEMOSYNEDBM_WLOCK		(ldap_pvt_thread_rdwr_wlock(&mnemosynedbm_big_rdwr))
#define MNEMOSYNEDBM_WUNLOCK	(ldap_pvt_thread_rdwr_wunlock(&mnemosynedbm_big_rdwr))
#define MNEMOSYNEDBM_RLOCK		(ldap_pvt_thread_rdwr_rlock(&mnemosynedbm_big_rdwr))
#define MNEMOSYNEDBM_RUNLOCK	(ldap_pvt_thread_rdwr_runlock(&mnemosynedbm_big_rdwr))


int mnemosynedbm_initialize( const char * home )
{
	if(mnemosynedbm_initialized++) return 1;

	MNEMOSYNEDBM_RWLOCK_INIT;

	return 0;
}


int mnemosynedbm_shutdown( void )
{
	if( !mnemosynedbm_initialized ) return 1;

	MNEMOSYNEDBM_RWLOCK_DESTROY;

	return 0;
}


DB_ENV *mnemosynedbm_initialize_env(const char *home, int dbcachesize, int *envdirok)
{
	return NULL;
}


void mnemosynedbm_shutdown_env(DB_ENV *env)
{
}

MNEMOSYNE_PERSISTENT struct list_head hashtable_list;
MNEMOSYNE_PERSISTENT int              hashtable_list_init = 0;


MNEMOSYNEDBM
mnemosynedbm_open( DB_ENV *env, char *name, int rw, int mode, int dbcachesize )
{
	hashtable_t  *ht_iter;
	MNEMOSYNEDBM ht = NULL;

	MNEMOSYNEDBM_WLOCK;

	if (hashtable_list_init == 0) {
		__tm_atomic {
			INIT_LIST_HEAD(&hashtable_list);
			hashtable_list_init = 1;
		}	
	}

	/* Check if hashtable already exists */
	list_for_each_entry(ht_iter, &hashtable_list, list) {
		if (strcmp(ht_iter->name, name)==0) {
			ht = ht_iter;
			goto unlock;
		}
	}

	/* Hashtable does not exist; create it */
	//__tm_atomic 
	{
		ht = (hashtable_t *) pmalloc(sizeof(hashtable_t));
		hashtable_init(ht, name);
		list_add(&(ht->list), &hashtable_list);
	}	

unlock:
	MNEMOSYNEDBM_WUNLOCK;
 
	return ht;
}

void
mnemosynedbm_close( MNEMOSYNEDBM mnemosynedbm )
{
}

void
mnemosynedbm_sync( MNEMOSYNEDBM mnemosynedbm )
{
}

Datum
mnemosynedbm_fetch( MNEMOSYNEDBM mnemosynedbm, Datum key )
{
	datum_t _value;
	datum_t _key;

	int	rc;

	MNEMOSYNEDBM_RLOCK;

	_key.data = key.data;
	_key.size = key.size;
	rc = hashtable_get(mnemosynedbm, &_key, &_value);

	if (rc != 0) {
		_value.data = NULL;
		_value.size = 0;
	}	

	MNEMOSYNEDBM_RUNLOCK;

	return( _value );
}

volatile int a;

int
mnemosynedbm_store( MNEMOSYNEDBM mnemosynedbm, Datum key, Datum value, int flags )
{
	int                rc;
	datum_t            _key;
	datum_t            _value;
	struct timeval     start_time;
	struct timeval     lock_time;
	struct timeval     stop_time;
	unsigned long long store_time;
	unsigned long long lockwait_time;

	gettimeofday(&start_time, NULL);

	MNEMOSYNEDBM_WLOCK;

	gettimeofday(&lock_time, NULL);

	_key.data = key.data;
	_key.size = key.size;
	_value.data = value.data;
	_value.size = value.size;
	rc = hashtable_put(mnemosynedbm, &_key, &_value, flags);
	gettimeofday(&stop_time, NULL);

	MNEMOSYNEDBM_WUNLOCK;

	lockwait_time = 1000000 * (lock_time.tv_sec - start_time.tv_sec) +
                  lock_time.tv_usec - start_time.tv_usec;
	store_time = 1000000 * (stop_time.tv_sec - lock_time.tv_sec) +
                  stop_time.tv_usec - lock_time.tv_usec;
	slapd_stats_mdbm_store(store_time, stop_time);
	slapd_stats_mdbm_store_lockwait(lockwait_time, stop_time);

	return( rc );
}


int
mnemosynedbm_delete( MNEMOSYNEDBM mnemosynedbm, Datum key )
{
	int	    rc;
	datum_t _key;
	datum_t _value;

	MNEMOSYNEDBM_WLOCK;

	_key.data = key.data;
	_key.size = key.size;
	rc = hashtable_del(mnemosynedbm, &_key, 0);

	MNEMOSYNEDBM_WUNLOCK;

	return( rc );
}

Datum
mnemosynedbm_firstkey( MNEMOSYNEDBM mnemosynedbm, MNEMOSYNEDBMCursor **dbch )
{
	assert(0);
}

Datum
mnemosynedbm_nextkey( MNEMOSYNEDBM mnemosynedbm, Datum key, MNEMOSYNEDBMCursor *dbcp )
{
	assert(0);
}

int
mnemosynedbm_errno( MNEMOSYNEDBM mnemosynedbm )
{
	return( errno );
}

#endif /* SLAPD_MNEMOSYNE */
