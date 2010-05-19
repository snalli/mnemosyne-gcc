#ifndef _RAWLOG_TORNBIT_H
#define _RAWLOG_TORNBIT_H

#include <sys/mman.h>
#include <mnemosyne.h>
#include <log.h>
#include <debug.h>

enum {
	LF_TYPE_TM_TORNBIT = 3
};

extern m_log_ops_t rawlog_tornbit_ops;

typedef struct m_rawlog_tornbit_s m_rawlog_tornbit_t;

/* Must ensure that phlog_tornbit is word aligned. */
struct m_rawlog_tornbit_s {
	m_phlog_tornbit_t   phlog_tornbit;
};

static inline
m_result_t
m_rawlog_tornbit_write(pcm_storeset_t *set, m_rawlog_tornbit_t *rawlog, pcm_word_t val)
{
	m_phlog_tornbit_t *phlog_tornbit = &(rawlog->phlog_tornbit);

	PHLOG_WRITE(tornbit, set, phlog_tornbit, val);

	return M_R_SUCCESS;
}


static inline
m_result_t
m_rawlog_tornbit_flush(pcm_storeset_t *set, m_rawlog_tornbit_t *rawlog)
{
	m_phlog_tornbit_t *phlog_tornbit = &(rawlog->phlog_tornbit);

	PHLOG_FLUSH(tornbit, set, phlog_tornbit);

	return M_R_SUCCESS;
}


m_result_t m_rawlog_tornbit_alloc (m_log_dsc_t *log_dsc);
m_result_t m_rawlog_tornbit_init (pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc);

#endif /* _RAWLOG_TORNBIT_H */
