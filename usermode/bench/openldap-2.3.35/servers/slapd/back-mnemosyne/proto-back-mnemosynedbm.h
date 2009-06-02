/* $OpenLDAP: pkg/ldap/servers/slapd/back-mnemosynedbm/proto-back-mnemosynedbm.h,v 1.79.2.4 2007/01/02 21:44:03 kurt Exp $ */
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

#ifndef _PROTO_BACK_MNEMOSYNEDBM
#define _PROTO_BACK_MNEMOSYNEDBM

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

/*
 * alias.c
 */
Entry *m_deref_internal_r LDAP_P((
	Backend *be,
	Entry *e,
	struct berval *dn,
	int *err,
	Entry **matched,
	const char **text ));

#define m_deref_entry_r( be, e, err, matched, text ) \
	m_deref_internal_r( be, e, NULL, err, matched, text )
#define m_deref_dn_r( be, dn, err, matched, text ) \
	m_deref_internal_r( be, NULL, dn, err, matched, text)

/*
 * attr.c
 */

void m_attr_mask LDAP_P(( struct mnemosynedbminfo *li,
	AttributeDescription *desc,
	slap_mask_t *indexmask ));

int m_attr_index_config LDAP_P(( struct mnemosynedbminfo *li,
	const char *fname, int lineno,
	int argc, char **argv ));
void m_attr_index_destroy LDAP_P(( Avlnode *tree ));

/*
 * cache.c
 */

int m_cache_add_entry_rw LDAP_P(( Cache *cache, Entry *e, int rw ));
int m_cache_update_entry LDAP_P(( Cache *cache, Entry *e ));
void m_cache_return_entry_rw LDAP_P(( Cache *cache, Entry *e, int rw ));
#define m_cache_return_entry_r(c, e) m_cache_return_entry_rw((c), (e), 0)
#define m_cache_return_entry_w(c, e) m_cache_return_entry_rw((c), (e), 1)
void m_cache_entry_commit LDAP_P(( Entry *e ));

ID m_cache_find_entry_ndn2id LDAP_P(( Backend *be, Cache *cache, struct berval *ndn ));
Entry * m_cache_find_entry_id LDAP_P(( Cache *cache, ID id, int rw ));
int m_cache_delete_entry LDAP_P(( Cache *cache, Entry *e ));
void m_cache_release_all LDAP_P(( Cache *cache ));

/*
 * dbcache.c
 */

DBCache * mnemosynedbm_cache_open LDAP_P(( Backend *be,
	const char *name, const char *suffix, int flags ));
void mnemosynedbm_cache_close LDAP_P(( Backend *be, DBCache *db ));
void mnemosynedbm_cache_really_close LDAP_P(( Backend *be, DBCache *db ));
void mnemosynedbm_cache_flush_all LDAP_P(( Backend *be ));
void mnemosynedbm_cache_sync LDAP_P(( Backend *be ));
#if 0 /* replaced by macro */
Datum mnemosynedbm_cache_fetch LDAP_P(( DBCache *db, Datum key ));
#else /* 1 */
#define mnemosynedbm_cache_fetch( db, key )	mnemosynedbm_fetch( (db)->dbc_db, (key) )
#endif /* 1 */
int mnemosynedbm_cache_store LDAP_P(( DBCache *db, Datum key, Datum data, int flags ));
int mnemosynedbm_cache_delete LDAP_P(( DBCache *db, Datum key ));
void *mnemosynedbm_cache_sync_daemon LDAP_P(( void *ctx, void *arg ));

/*
 * dn2id.c
 */

int m_dn2id_add LDAP_P(( Backend *be, struct berval *dn, ID id ));
int m_dn2id LDAP_P(( Backend *be, struct berval *dn, ID *idp ));
int m_dn2idl LDAP_P(( Backend *be, struct berval *dn, int prefix, ID_BLOCK **idlp ));
int m_dn2id_delete LDAP_P(( Backend *be, struct berval *dn, ID id ));

Entry * m_dn2entry_rw LDAP_P(( Backend *be, struct berval *dn, Entry **matched, int rw ));
#define m_dn2entry_r(be, dn, m) m_dn2entry_rw((be), (dn), (m), 0)
#define m_dn2entry_w(be, dn, m) m_dn2entry_rw((be), (dn), (m), 1)

/*
 * entry.c
 */
BI_entry_release_rw mnemosynedbm_back_entry_release_rw;
BI_entry_get_rw mnemosynedbm_back_entry_get;

/*
 * filterindex.c
 */

ID_BLOCK * m_filter_candidates LDAP_P(( Operation *op, Filter *f ));

/*
 * id2children.c
 */

int id2children_add LDAP_P(( Backend *be, Entry *p, Entry *e ));
int id2children_remove LDAP_P(( Backend *be, Entry *p, Entry *e ));
int m_has_children LDAP_P(( Backend *be, Entry *p ));

/*
 * id2entry.c
 */

int m_id2entry_add LDAP_P(( Backend *be, Entry *e ));
int m_id2entry_delete LDAP_P(( Backend *be, Entry *e ));

Entry * m_id2entry_rw LDAP_P(( Backend *be, ID id, int rw )); 
#define m_id2entry_r(be, id)	m_id2entry_rw((be), (id), 0)
#define m_id2entry_w(be, id)	m_id2entry_rw((be), (id), 1)

/*
 * idl.c
 */

ID_BLOCK * m_idl_alloc LDAP_P(( unsigned int nids ));
ID_BLOCK * m_idl_allids LDAP_P(( Backend *be ));
void m_idl_free LDAP_P(( ID_BLOCK *idl ));
ID_BLOCK * m_idl_fetch LDAP_P(( Backend *be, DBCache *db, Datum key ));
int m_idl_insert_key LDAP_P(( Backend *be, DBCache *db, Datum key, ID id ));
int m_idl_insert LDAP_P(( ID_BLOCK **idl, ID id, unsigned int maxids ));
int m_idl_delete_key LDAP_P(( Backend *be, DBCache *db, Datum key, ID id ));
ID_BLOCK * m_idl_intersection LDAP_P(( Backend *be, ID_BLOCK *a, ID_BLOCK *b ));
ID_BLOCK * m_idl_union LDAP_P(( Backend *be, ID_BLOCK *a, ID_BLOCK *b ));
ID_BLOCK * m_idl_notin LDAP_P(( Backend *be, ID_BLOCK *a, ID_BLOCK *b ));
ID m_idl_firstid LDAP_P(( ID_BLOCK *idl, ID *cursor ));
ID m_idl_nextid LDAP_P(( ID_BLOCK *idl, ID *cursor ));

/*
 * index.c
 */
extern int
m_index_is_indexed LDAP_P((
	Backend *be,
	AttributeDescription *desc ));

extern int
m_index_param LDAP_P((
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	char **dbname,
	slap_mask_t *mask,
	struct berval *prefix ));

extern int
m_index_values LDAP_P((
	Operation *op,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int opid ));

int m_index_entry LDAP_P(( Operation *op, int r, Entry *e ));
#define m_index_entry_add(be,e) m_index_entry((be),SLAP_INDEX_ADD_OP,(e))
#define m_index_entry_del(be,e) m_index_entry((be),SLAP_INDEX_DELETE_OP,(e))


/*
 * key.c
 */
extern int
m_key_change LDAP_P((
    Backend		*be,
    DBCache	*db,
    struct berval *k,
    ID			id,
    int			op ));
extern int
m_key_read LDAP_P((
    Backend	*be,
	DBCache *db,
    struct berval *k,
	ID_BLOCK **idout ));

/*
 * modify.c
 * These prototypes are placed here because they are used by modify and
 * modify rdn which are implemented in different files. 
 *
 * We need mnemosynedbm_internal_modify here because of LDAP modrdn & modify use 
 * it. If we do not add this, there would be a bunch of code replication 
 * here and there and of course the likelihood of bugs increases.
 * Juan C. Gomez (gomez@engr.sgi.com) 05/18/99
 * 
 */

/* returns LDAP error code indicating error OR SLAPD_ABANDON */
int mnemosynedbm_modify_internal LDAP_P(( Operation *op,
	Modifications *mods, Entry *e,
	const char **text, char *textbuf, size_t textlen ));

/*
 * nextid.c
 */

int m_next_id LDAP_P(( Backend *be, ID *idp ));
int m_next_id_get LDAP_P(( Backend *be, ID *idp ));
int m_next_id_write LDAP_P(( Backend *be, ID id ));

/*
 * former external.h
 */

extern BI_init			mnemosynedbm_back_initialize;

extern BI_open			mnemosynedbm_back_open;
extern BI_close			mnemosynedbm_back_close;
extern BI_destroy		mnemosynedbm_back_destroy;

extern BI_db_init		mnemosynedbm_back_db_init;
extern BI_db_open		mnemosynedbm_back_db_open;
extern BI_db_close		mnemosynedbm_back_db_close;
extern BI_db_destroy		mnemosynedbm_back_db_destroy;
extern BI_db_config		mnemosynedbm_back_db_config;

extern BI_op_extended		mnemosynedbm_back_extended;
extern BI_op_bind		mnemosynedbm_back_bind;
extern BI_op_search		mnemosynedbm_back_search;
extern BI_op_compare		mnemosynedbm_back_compare;
extern BI_op_modify		mnemosynedbm_back_modify;
extern BI_op_modrdn		mnemosynedbm_back_modrdn;
extern BI_op_add		mnemosynedbm_back_add;
extern BI_op_delete		mnemosynedbm_back_delete;

extern BI_operational		mnemosynedbm_back_operational;
extern BI_has_subordinates	mnemosynedbm_back_hasSubordinates;

/* hooks for slap tools */
extern BI_tool_entry_open	mnemosynedbm_tool_entry_open;
extern BI_tool_entry_close	mnemosynedbm_tool_entry_close;
extern BI_tool_entry_first	mnemosynedbm_tool_entry_first;
extern BI_tool_entry_next	mnemosynedbm_tool_entry_next;
extern BI_tool_entry_get	mnemosynedbm_tool_entry_get;
extern BI_tool_entry_put	mnemosynedbm_tool_entry_put;

extern BI_tool_entry_reindex	mnemosynedbm_tool_entry_reindex;
extern BI_tool_sync		mnemosynedbm_tool_sync;

extern BI_chk_referrals		mnemosynedbm_back_referrals;

LDAP_END_DECL

#endif /* _PROTO_BACK_MNEMOSYNEDBM */
