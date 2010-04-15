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
//#define MAP_PERSISTENT MAP_SCM
#define MAP_PERSISTENT 0


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
	uint32_t  size;
};


/** Persistent segment table. */
struct m_segtbl_s {
	m_segtbl_entry_t *entries;  /**< pointer to the persistent segment table entries */
	m_segidx_t       *idx;      /**< the fast index providing access to the persistent segment table. */
};


extern m_segtbl_t m_segtbl;


m_result_t m_segmentmgr_init();
m_result_t m_segmentmgr_fini();

void *m_pmap2(void *start, size_t length, int prot, int flags);
m_result_t m_segment_find_using_addr(void *addr, m_segidx_entry_t **entryp);

#endif /* _MNEMOSYNE_SEGMENT_H */
