/**
 * \file
 * 
 * \brief The interface to the log manager.
 *
 */
#ifndef _LOG_H
#define _LOG_H

#include <pthread.h>
#include <stdint.h>
#include <result.h>
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef uint64_t scm_word_t;
#define PCM_STORE(addr, val) do { *(addr) = (val); } while(0);
#define PCM_BARRIER


/* 
 * Physical log size is power of 2 to implement arithmetic efficiently 
 * e.g. modulo using bitwise operations: x % 2^n == x & (2^n - 1) 
 */
#define PHYSICAL_LOG_NUM_ENTRIES_LOG2 5
#define PHYSICAL_LOG_NUM_ENTRIES      (1 << PHYSICAL_LOG_NUM_ENTRIES_LOG2)
#define PHYSICAL_LOG_SIZE             (PHYSICAL_LOG_NUM_ENTRIES * sizeof(scm_word_t)) /* in bytes */


/* Masks for the 64-bit generic_flags field. */

#define LF_FLAGS_MASK     0xFFFFFFFFFFFF0000LLU      /* generic flags mask */
#define LF_TYPE_MASK      0x000000000000FFFFLLU      /* log type mask */

/* Generic flags */
#define LF_DIRTY          0x8000000000000000


/* Log truncation and recovery */
#define INV_LOG_ORDER     0xFFFFFFFFFFFFFFFF

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
	m_result_t (*init)(m_log_t *log, m_log_dsc_t *log_dsc);
	m_result_t (*truncation_init)(m_log_dsc_t *log_dsc);
	m_result_t (*truncation_prepare_next)(m_log_dsc_t *log_dsc);
	m_result_t (*truncation_do)(m_log_dsc_t *log_dsc);
	m_result_t (*recovery_init)(m_log_dsc_t *log_dsc);
	m_result_t (*recovery_prepare_next)(m_log_dsc_t *log_dsc);
	m_result_t (*recovery_do)(m_log_dsc_t *log_dsc);
};


/** 
 * Generic descriptor non-volatile log metadata. 
 * Each log type should define its own struct type.
 */
struct m_log_nvmd_s {
	scm_word_t generic_flags;
	scm_word_t reserved2;                     /**< reserved for future use */
	scm_word_t reserved3;                     /**< reserved for future use */
	scm_word_t reserved4;                     /**< reserved for future use */
};

struct m_log_dsc_s {
	m_log_ops_t      *ops;             /**< log operations */
	m_log_t          *log;             /**< descriptor structure specfic to log type */
	m_log_nvmd_t     *nvmd;            /**< non-volatile log metadata */
	scm_word_t       *nvphlog;         /**< non-volatile physical log */
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
	int              logtrunc_thread;
};


m_result_t m_logmgr_init();
m_result_t m_logmgr_fini();
m_result_t m_logmgr_register_logtype(int type, m_log_ops_t *ops);
m_result_t m_logmgr_alloc_log(int type, m_log_dsc_t **log_dscp);
m_result_t m_logmgr_do_recovery();
m_result_t m_logtrunc_truncate();

#ifdef __cplusplus
}
#endif

#endif /* _LOG_H */
