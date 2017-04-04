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

/*!
 * \file
 * Routines concerning the checkpointing and restoration of persistent segments.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */

#ifndef _MNEMOSYNE_SEGMENT_H
#define _MNEMOSYNE_SEGMENT_H

/* System header files */
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
/* Mnemosyne common header files */
#include <list.h>
#include <result.h>


#ifndef MAP_SCM
#define MAP_SCM 0x80000
#endif


#ifdef MCORE_KERNEL_PMAP
# define MAP_PERSISTENT MAP_SCM
#else
# define MAP_PERSISTENT 0
#endif

/** Persistent segment table entry flags */
#define SGTB_TYPE_PMAP                0x1    /* a segment allocated via pmap family of functions */
#define SGTB_TYPE_SECTION             0x2    /* a segment of a .persistent section */
#define SGTB_VALID_ENTRY              0x4
#define SGTB_VALID_DATA               0x8

typedef struct m_segtbl_entry_s m_segtbl_entry_t;
typedef struct m_segidx_entry_s m_segidx_entry_t;
typedef struct m_segidx_s       m_segidx_t;
typedef struct m_segtbl_s       m_segtbl_t;

/** Persistent segment table index entry. */
struct m_segidx_entry_s {
	m_segtbl_entry_t *segtbl_entry; /**< the segment table entry */
	uint32_t         index;         /**< the index of the entry in the segment table */ 
	uint64_t         module_id;     /**< valid for .persistent sections only. Identifies the module the .persistent section belongs to */
	struct list_head list;
};


/* Persistent segment table index. */
struct m_segidx_s {
	pthread_mutex_t  mutex;          /**< synchronizes access to the index */
	m_segidx_entry_t *all_entries;   /**< all the segment index entries */
	m_segidx_entry_t mapped_entries; /**< the head of the mapped segments list; we keep this list ordered by start address; no overlaps allowed */
	m_segidx_entry_t free_entries;   /**< the head of the free segments list */
};


/** Persistent segment table entry. */
struct m_segtbl_entry_s {
	uint32_t  flags;
	uintptr_t start;
	unsigned long long  size;
};


/** Persistent segment table. */
struct m_segtbl_s {
	m_segtbl_entry_t *entries;  /**< pointer to the persistent segment table entries */
	m_segidx_t       *idx;      /**< the fast index providing access to the persistent segment table. */
};


extern m_segtbl_t m_segtbl;


m_result_t m_segmentmgr_init();
m_result_t m_segmentmgr_fini();

void *m_pmap2(void *start, unsigned long long length, int prot, int flags);
m_result_t m_segment_find_using_addr(void *addr, m_segidx_entry_t **entryp);

#endif /* _MNEMOSYNE_SEGMENT_H */
