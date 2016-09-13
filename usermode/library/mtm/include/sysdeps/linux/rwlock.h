/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

/**
 * \file rwlock.h
 * \brief Reader-writer lock definition
 *
 */

#ifndef MTM_RWLOCK_H_AGH190
#define MTM_RWLOCK_H_AGH190

typedef struct {
	//TODO: fields
} mtm_rwlock_t;

extern int mtm_rwlock_init (mtm_rwlock_t *);
extern int mtm_rwlock_rdlock (mtm_rwlock_t *);
extern int mtm_rwlock_wrlock (mtm_rwlock_t *);
extern int mtm_rwlock_trywrlock (mtm_rwlock_t *);
extern int mtm_rwlock_unlock (mtm_rwlock_t *);

#endif /* MTM_RWLOCK_H_AGH190 */
