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
 * \file pwb_i.h
 *
 * \brief Private header file for write-back common header file.
 *
 */

#ifndef _PWB_COMMON_INTERNAL_IOK811_H
#define _PWB_COMMON_INTERNAL_IOK811_H

#include <pcm.h>
#include <log.h>

#include "mtm_i.h"
#include "dtable.h"
#include "local.h"
#include "locks.h"
#include "tmlog.h"


//#undef MTM_DEBUG_PRINT
//# define MTM_DEBUG_PRINT(...)               printf(__VA_ARGS__); fflush(NULL)

void ITM_NORETURN mtm_pwb_restart_transaction (mtm_tx_t *tx, mtm_restart_reason r);


typedef struct mtm_pwb_r_entry_s      mtm_pwb_r_entry_t;
typedef struct mtm_pwb_r_set_s        mtm_pwb_r_set_t;
typedef struct mtm_pwb_w_entry_s      mtm_pwb_w_entry_t;
typedef struct mtm_pwb_w_set_s        mtm_pwb_w_set_t;
typedef struct mtm_pwb_mode_data_s    mtm_pwb_mode_data_t;
typedef struct mtm_pwb_r_entry_s      r_entry_t;
typedef struct mtm_pwb_r_set_s        r_set_t;
typedef struct mtm_pwb_w_entry_s      w_entry_t;
typedef struct mtm_pwb_w_set_s        w_set_t;
typedef struct mtm_pwb_mode_data_s    mode_data_t;



/* Read set entry */
struct mtm_pwb_r_entry_s {
  mtm_word_t          version;          /* Version read */
  volatile mtm_word_t *lock;            /* Pointer to lock (for fast access) */
};


/* Read set */
struct mtm_pwb_r_set_s {                  
  mtm_pwb_r_entry_t   *entries;         /* Array of entries */
  int                 nb_entries;       /* Number of entries */
  int                 size;             /* Size of array */
};


/* Volatile write set entry */
struct mtm_pwb_w_entry_s {
	union {                                                  /* For padding... */
		struct {
			volatile mtm_word_t         *addr;               /* Address written */
			mtm_word_t                  value;               /* New (write-back) or old (write-through) value */
			mtm_word_t                  mask;                /* Write mask */
			mtm_word_t                  version;             /* Version overwritten */
			int                         is_nonvolatile;      /* Write access is to non-volatile memory */
			volatile mtm_word_t         *lock;               /* Pointer to lock (for fast access) */
#if defined(CONFLICT_TRACKING)
			struct mtm_tx_s             *tx;                 /* Transaction owning the write set */
#endif /* defined(CONFLICT_TRACKING) */
			struct mtm_pwb_w_entry_s    *next;               /* Next address covered by same lock (if any) */
			struct mtm_pwb_w_entry_s*   next_cache_neighbor; /* Next address covered by same lock and falls within the same cacheline. These entries can be written together with a single cache-line flush. */
		};
#if CM == CM_PRIORITY
		mtm_word_t padding[12];                              /* Padding (must be a multiple of 32 bytes) */
#endif /* CM == CM_PRIORITY */
	};
};


/* Write set */
struct mtm_pwb_w_set_s {             
	mtm_pwb_w_entry_t *entries;           /* Array of entries */
	int               nb_entries;         /* Number of entries */
	int               size;               /* Size of array */
	int               reallocate;         /* Reallocate on next start */
};


/*!
 * A descriptor associated with each transaction, holding that transaction's read/write
 * set and other statistics about the transaction specific to this mode.
 */
struct mtm_pwb_mode_data_s
{
	mtm_word_t      start;
	mtm_word_t      end;

	mtm_pwb_r_set_t r_set;
	mtm_pwb_w_set_t w_set;
	
	m_log_dsc_t     *ptmlog_dsc; /**< The persistent tm log descriptor */
	M_TMLOG_T       *ptmlog;     /**< The persistent tm log; this is to avoid dereferencing ptmlog_dsc in the fast path */
};

#endif /* _PWB_COMMON_INTERNAL_IOK811_H */
