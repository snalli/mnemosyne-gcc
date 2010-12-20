#ifndef _LOG_TORNBIT_H
#define _LOG_TORNBIT_H

#include <string.h>
#include <sys/mman.h>
#include <mnemosyne.h>
#include <log.h>

#define COMMIT_MARKER 0x0010000000000000

enum {
	LF_TYPE_PAXOS_TORNBIT = 4
};

extern m_log_ops_t log_tornbit_ops;

typedef struct m_log_tornbit_s m_log_tornbit_t;

typedef void tornbit_flush_set_t;

/* Must ensure that phlog_tornbit is word aligned. */
struct m_log_tornbit_s {
	m_phlog_tornbit_t   phlog_tornbit;
	tornbit_flush_set_t *flush_set;
};

static inline
m_result_t
m_log_tornbit_begin(pcm_storeset_t *set, m_log_tornbit_t *log, pcm_word_t recid)
{
	m_phlog_tornbit_t *phlog_tornbit = &(log->phlog_tornbit);
# ifdef	SYNC_TRUNCATION
# error "No support for SYNC_TRUNCATION"
# else
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, recid);
# endif
	return M_R_SUCCESS;
}



static inline
m_result_t
m_log_tornbit_write(pcm_storeset_t *set, m_log_tornbit_t *log, pcm_word_t val)
{
	m_phlog_tornbit_t *phlog_tornbit = &(log->phlog_tornbit);
# ifdef	SYNC_TRUNCATION
# error "No support for SYNC_TRUNCATION"
# else
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, val);
# endif
	return M_R_SUCCESS;
}


static inline
m_result_t
m_log_tornbit_write_char(pcm_storeset_t *set, m_log_tornbit_t *log, const char *buf, unsigned int buflen)
{
	m_phlog_tornbit_t *phlog_tornbit = &(log->phlog_tornbit);
	pcm_word_t v;
    unsigned int i;
# ifdef	SYNC_TRUNCATION
# error "No support for SYNC_TRUNCATION"
# else
	for (i=0; i<buflen; i+=sizeof(pcm_word_t)) {
    	v = *((pcm_word_t *) &buf[i]);
		PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, v);
	}	
# endif
	return M_R_SUCCESS;
}


static inline
m_result_t
m_log_tornbit_commit(pcm_storeset_t *set, m_log_tornbit_t *log)
{
	m_phlog_tornbit_t *phlog_tornbit = &(log->phlog_tornbit);

# ifdef	SYNC_TRUNCATION
# error "No support for SYNC_TRUNCATION"
# else
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) COMMIT_MARKER);
	PHLOG_FLUSH_ASYNCTRUNC(tornbit, set, phlog_tornbit);
# endif
	return M_R_SUCCESS;
}


static inline
m_result_t
m_log_tornbit_truncate_sync(pcm_storeset_t *set, m_log_tornbit_t *log)
{
	m_phlog_tornbit_truncate_sync(set, &log->phlog_tornbit);

	return M_R_SUCCESS;
}



m_result_t m_log_tornbit_alloc (m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_init (pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_truncation_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_truncation_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_truncation_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_recovery_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_recovery_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_log_tornbit_report_stats(m_log_dsc_t *log_dsc);


#endif /* _LOG_TORNBIT_H */
