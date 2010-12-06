/* mnemosynedbm.h - ldap dbm compatibility routine header file */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/mnemosynedbm.h,v 1.2.2.3 2007/01/02 21:44:03 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2007 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _MNEMOSYNEDBM_H_
#define _MNEMOSYNEDBM_H_

#include <ldap_cdefs.h>
#include <ac/string.h>


#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include "hash.h"


#define R_NOOVERWRITE DB_NOOVERWRITE
#define DB_ENV        void


LDAP_BEGIN_DECL

typedef datum_t	Datum;
#define dsize	size
#define dptr	data

typedef hashtable_t *MNEMOSYNEDBM;


/* for mnemosynedbm_open */
typedef void MNEMOSYNEDBMCursor;
#define MNEMOSYNEDBM_READER	    0
#define MNEMOSYNEDBM_WRITER	    0
#define MNEMOSYNEDBM_WRCREAT    0
#define MNEMOSYNEDBM_NEWDB      0

LDAP_END_DECL

/* for mnemosynedbm_open */
#define MNEMOSYNEDBM_NOSYNC	    0
#define MNEMOSYNEDBM_SYNC       0
#define MNEMOSYNEDBM_LOCKING    0
#define MNEMOSYNEDBM_NOLOCKING  0

/* for mnemosynedbm_insert */
#define MNEMOSYNEDBM_INSERT	    0
#define MNEMOSYNEDBM_REPLACE    HASH_REPLACE

#	define MNEMOSYNEDBM_SUFFIX	".dbb"

LDAP_BEGIN_DECL

LDAP_MNEMOSYNEDBM_F (int) mnemosynedbm_initialize( const char * );
LDAP_MNEMOSYNEDBM_F (int) mnemosynedbm_shutdown( void );

LDAP_MNEMOSYNEDBM_F (DB_ENV*) mnemosynedbm_initialize_env(const char *, int dbcachesize, int *envdirok);
LDAP_MNEMOSYNEDBM_F (void) mnemosynedbm_shutdown_env(DB_ENV *);

LDAP_MNEMOSYNEDBM_F (int) mnemosynedbm_errno( MNEMOSYNEDBM mnemosynedbm );
LDAP_MNEMOSYNEDBM_F (MNEMOSYNEDBM) mnemosynedbm_open( DB_ENV *env, char *name, int rw, int mode, int dbcachesize );
LDAP_MNEMOSYNEDBM_F (void) mnemosynedbm_close( MNEMOSYNEDBM mnemosynedbm );
LDAP_MNEMOSYNEDBM_F (void) mnemosynedbm_sync( MNEMOSYNEDBM mnemosynedbm );
LDAP_MNEMOSYNEDBM_F (void) mnemosynedbm_datum_free( MNEMOSYNEDBM mnemosynedbm, Datum data );
LDAP_MNEMOSYNEDBM_F (Datum) mnemosynedbm_datum_dup( MNEMOSYNEDBM mnemosynedbm, Datum data );
LDAP_MNEMOSYNEDBM_F (Datum) mnemosynedbm_fetch( MNEMOSYNEDBM mnemosynedbm, Datum key );
LDAP_MNEMOSYNEDBM_F (int) mnemosynedbm_store( MNEMOSYNEDBM mnemosynedbm, Datum key, Datum data, int flags );
LDAP_MNEMOSYNEDBM_F (int) mnemosynedbm_delete( MNEMOSYNEDBM mnemosynedbm, Datum key );

LDAP_MNEMOSYNEDBM_F (Datum) mnemosynedbm_firstkey( MNEMOSYNEDBM mnemosynedbm, MNEMOSYNEDBMCursor **cursor );
LDAP_MNEMOSYNEDBM_F (Datum) mnemosynedbm_nextkey( MNEMOSYNEDBM mnemosynedbm, Datum key, MNEMOSYNEDBMCursor *cursor );


/* initialization of Datum structures */
#define mnemosynedbm_datum_init(d) ((void)memset(&(d), '\0', sizeof(Datum)))

LDAP_END_DECL

#endif /* _mnemosynedbm_h_ */
