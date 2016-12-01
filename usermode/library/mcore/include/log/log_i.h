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
 * \file
 * 
 * \brief The interface to the log manager.
 *
 */
#ifndef _LOG_INTERNAL_H
#define _LOG_INTERNAL_H

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <result.h>
#include <list.h>
#include "hrtime.h"
#include "../hal/pcm_i.h"

#ifdef __cplusplus
extern "C" {
#endif


/* 
 * Physical log size is power of 2 to implement arithmetic efficiently 
 * e.g. modulo using bitwise operations: x % 2^n == x & (2^n - 1) 
 */
#define PHYSICAL_LOG_NUM_ENTRIES_LOG2 20
#define PHYSICAL_LOG_NUM_ENTRIES      (1 << PHYSICAL_LOG_NUM_ENTRIES_LOG2)
#define PHYSICAL_LOG_SIZE             (PHYSICAL_LOG_NUM_ENTRIES * sizeof(pcm_word_t)) /* in bytes */


/* Masks for the 64-bit non-volatile generic_flags field. */

#define LF_FLAGS_MASK     0xFFFFFFFFFFFF0000LLU      /* generic flags mask */
#define LF_TYPE_MASK      0x000000000000FFFFLLU      /* log type mask */

/* Generic non-volatile flags */
#define LF_DIRTY          0x8000000000000000

/* Log truncation and recovery */
#define INV_LOG_ORDER     0xFFFFFFFFFFFFFFFF

/* Volatile flags */
#define LF_ASYNC_TRUNCATION 0x0000000000000001

/* Hardwired log types known at compilation time (static) */
enum {
	LF_TYPE_FREE  = 0,
	LF_TYPE_VALIDVALUES
};


typedef struct m_logmgr_s     m_logmgr_t;
typedef struct m_log_s        m_log_t;
typedef struct m_log_ops_s    m_log_ops_t;
typedef struct m_log_dsc_s    m_log_dsc_t;
typedef struct m_log_nvmd_s   m_log_nvmd_t;


struct m_log_ops_s {
	m_result_t (*alloc)(m_log_dsc_t *log_dsc);
	m_result_t (*init)(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc);
	m_result_t (*truncation_init)(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
	m_result_t (*truncation_prepare_next)(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
	m_result_t (*truncation_do)(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
	m_result_t (*recovery_init)(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
	m_result_t (*recovery_prepare_next)(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
	m_result_t (*recovery_do)(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
	m_result_t (*report_stats)(m_log_dsc_t *log_dsc);
};


/** 
 * Generic descriptor non-volatile log metadata. 
 * Each log type should define its own struct type.
 */
struct m_log_nvmd_s {
	pcm_word_t generic_flags;
	pcm_word_t reserved2;                     /**< reserved for future use */
	pcm_word_t reserved3;                     /**< reserved for future use */
	pcm_word_t reserved4;                     /**< reserved for future use */
        // char padding[32];
};

struct m_log_dsc_s {
	m_log_ops_t      *ops;             /**< log operations */
	m_log_t          *log;             /**< descriptor structure specific to log type */
	m_log_nvmd_t     *nvmd;            /**< non-volatile log metadata */
	pcm_word_t       *nvphlog;         /**< non-volatile physical log */
	uint64_t         flags;            /**< array of flags */
	uint64_t         logorder;         /**< log order number */
	struct list_head list;
};

extern m_log_ops_t *m_log_ops[];

struct m_logmgr_s {
	pthread_mutex_t  mutex;                 /**< lock synchronizing access to this structure */
	struct list_head free_logs_list;        /**< free logs available for reuse */
	struct list_head pending_logs_list;     /**< logs which are not free but not recovered yet because of unknown type */
	struct list_head active_logs_list;      /**< actively used logs (could be dirty or not) */
	struct list_head known_logtypes_list;   /**< log types known (registered) to the log manager */
	/* log truncation */
	pthread_cond_t   logtrunc_cond;
	pthread_t        logtrunc_thread;
	uint64_t         trunc_time;
	uint64_t         trunc_count;
};


m_result_t m_logmgr_init(pcm_storeset_t *set);
m_result_t m_logmgr_fini(void);
m_result_t m_logmgr_register_logtype(pcm_storeset_t *set, int type, m_log_ops_t *ops);
m_result_t m_logmgr_alloc_log(pcm_storeset_t *set, int type, uint64_t flags, m_log_dsc_t **log_dscp);
m_result_t m_logmgr_free_log(m_log_dsc_t *log_dsc);
m_result_t m_logmgr_do_recovery(pcm_storeset_t *set);
m_result_t m_logtrunc_truncate(pcm_storeset_t *set);
void m_logmgr_stat_print();



#define PHLOG_WRITE(logtype, set, phlog, val)                                 \
do {                                                                          \
    int retries = 0;                                                          \
    while (m_phlog_##logtype##_write(set, (phlog), (val)) != M_R_SUCCESS) {   \
        if (retries++ > 1) {                                                  \
            M_INTERNALERROR("Cannot complete log write successfully.\n");     \
        }                                                                     \
        (phlog)->stat_wait_for_trunc++;                                       \
        m_logtrunc_truncate(set);                                             \
    }                                                                         \
} while (0);


#define PHLOG_FLUSH(logtype, set, phlog)                                      \
do {                                                                          \
    int retries = 0;                                                          \
    while (m_phlog_##logtype##_flush(set, (phlog)) != M_R_SUCCESS) {          \
        if (retries++ > 1) {                                                  \
            M_INTERNALERROR("Cannot complete log write successfully.\n");     \
        }                                                                     \
        (phlog)->stat_wait_for_trunc++;                                       \
        m_logtrunc_truncate(set);                                             \
    }                                                                         \
} while (0);


#define PHLOG_WRITE_ASYNCTRUNC(logtype, set, phlog, val)                       \
do {                                                                           \
	hrtime_t __start;                                                          \
	hrtime_t __end;                                                            \
    if (m_phlog_##logtype##_write(set, (phlog), (val)) != M_R_SUCCESS) {       \
        (phlog)->stat_wait_for_trunc++;                                        \
        __start = hrtime_cycles();                                             \
        while (m_phlog_##logtype##_write(set, (phlog), (val)) != M_R_SUCCESS); \
        __end = hrtime_cycles();                                               \
	    phlog->stat_wait_time_for_trunc += (HRTIME_CYCLE2NS(__end - __start)); \
    }                                                                          \
} while (0);


#define PHLOG_FLUSH_ASYNCTRUNC(logtype, set, phlog)                            \
do {                                                                           \
	hrtime_t __start;                                                          \
	hrtime_t __end;                                                            \
    if (m_phlog_##logtype##_flush(set, (phlog)) != M_R_SUCCESS) {              \
        (phlog)->stat_wait_for_trunc++;                                        \
        __start = hrtime_cycles();                                             \
        while (m_phlog_##logtype##_flush(set, (phlog)) != M_R_SUCCESS);        \
        __end = hrtime_cycles();                                               \
	    phlog->stat_wait_time_for_trunc += (HRTIME_CYCLE2NS(__end - __start)); \
    }                                                                          \
} while (0);


#ifdef __cplusplus
}
#endif

#endif /* _LOG_INTERNAL_H */
