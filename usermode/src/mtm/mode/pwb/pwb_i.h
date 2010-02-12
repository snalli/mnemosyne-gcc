/**
 * \file pwb_i.h
 *
 * \brief Private header file for write-back with encounter-time locking.
 *
 */

#ifndef _WBETL_INTERNAL_H
#define _WBETL_INTERNAL_H

#include "mtm_i.h"
#include "mode/pwb/locks.h"
#include "hal/pcm.h"
#include "recovery.h"

#undef  DESIGN
#define DESIGN WRITE_BACK_ETL

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
  mtm_pwb_r_entry_t *entries;         /* Array of entries */
  int                 nb_entries;       /* Number of entries */
  int                 size;             /* Size of array */
};


/* Volatile write set entry */
struct mtm_pwb_w_entry_s {
  union {                                 /* For padding... */
    struct {
      volatile mtm_word_t         *addr;   /* Address written */
      mtm_word_t                  value;   /* New (write-back) or old (write-through) value */
      mtm_word_t                  mask;    /* Write mask */
      mtm_word_t                  version;       /* Version overwritten */
      nonvolatile_write_set_entry_t* w_entry_nv; /* The matching nonvolatile write-set entry, containing the same data. */
      volatile mtm_word_t         *lock;         /* Pointer to lock (for fast access) */
#if defined(CONFLICT_TRACKING)
      struct mtm_tx_s             *tx;           /* Transaction owning the write set */
#endif /* defined(CONFLICT_TRACKING) */
      struct mtm_pwb_w_entry_s *next;           /* Next address covered by same lock (if any) */
    };
#if CM == CM_PRIORITY
    mtm_word_t padding[12];                /* Padding (must be a multiple of 32 bytes) */
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

	pcm_storeset_t  *pcm_storeset;

	mtm_pwb_r_set_t r_set;
	mtm_pwb_w_set_t w_set;
	
	/*! The portion of the write set residing in nonvolatile memory. */
	nonvolatile_write_set_t* w_set_nv;
};


#endif
