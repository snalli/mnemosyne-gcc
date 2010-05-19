#ifndef _RAWLOG_BASE_H
#define _RAWLOG_BASE_H

#include <sys/mman.h>
#include <mnemosyne.h>
#include <log.h>
#include <debug.h>

enum {
	LF_TYPE_TM_BASE = 2
};

extern m_log_ops_t rawlog_base_ops;

typedef struct m_rawlog_base_s m_rawlog_base_t;

/* Must ensure that phlog_base is word aligned. */
struct m_rawlog_base_s {
	m_phlog_base_t   phlog_base;
};


static inline
m_result_t
m_rawlog_base_write(pcm_storeset_t *set, 
                   m_rawlog_base_t *rawlog, 
                   pcm_word_t val)
{
	m_phlog_base_t *phlog_base = &(rawlog->phlog_base);

	PHLOG_WRITE(base, set, phlog_base, val);

	return M_R_SUCCESS;
}


static inline
m_result_t
m_rawlog_base_flush(pcm_storeset_t *set, m_rawlog_base_t *rawlog)
{
	m_phlog_base_t *phlog_base = &(rawlog->phlog_base);

	PHLOG_FLUSH(base, set, phlog_base);

	return M_R_SUCCESS;
}


m_result_t m_rawlog_base_alloc (m_log_dsc_t *log_dsc);
m_result_t m_rawlog_base_init (pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc);


#endif /* _RAWLOG_BASE_H */
