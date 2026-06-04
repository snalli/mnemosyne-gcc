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

#ifndef _TMLOG_TORNBIT_H
#define _TMLOG_TORNBIT_H

#include <sys/mman.h>
#include <mnemosyne.h>
#include <log.h>
#include <debug.h>
#include "mtm_i.h"

#define XACT_COMMIT_MARKER 0x0010000000000000
#define XACT_ABORT_MARKER  0x0100000000000000

enum {
	LF_TYPE_TM_TORNBIT = 3
};

extern m_log_ops_t tmlog_tornbit_ops;

typedef struct m_tmlog_tornbit_s m_tmlog_tornbit_t;

typedef void tornbit_flush_set_t;

/* Must ensure that phlog_tornbit is word aligned. */
struct m_tmlog_tornbit_s {
	m_phlog_tornbit_t   phlog_tornbit;
	tornbit_flush_set_t *flush_set;
};

static inline
m_result_t
m_tmlog_tornbit_write(pcm_storeset_t *set, m_tmlog_tornbit_t *tmlog, uintptr_t addr, pcm_word_t val, pcm_word_t mask)
{
	m_phlog_tornbit_t *phlog_tornbit = &(tmlog->phlog_tornbit);

# ifdef	SYNC_TRUNCATION
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) addr);
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) val);
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) mask);
# else
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) addr);
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) val);
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) mask);
# endif

	return M_R_SUCCESS;
}


static inline
m_result_t
m_tmlog_tornbit_begin(m_tmlog_tornbit_t *tmlog)
{
	return M_R_SUCCESS;
}


static inline
m_result_t
m_tmlog_tornbit_commit(pcm_storeset_t *set, m_tmlog_tornbit_t *tmlog, uint64_t sqn)
{
	m_phlog_tornbit_t *phlog_tornbit = &(tmlog->phlog_tornbit);

# ifdef	SYNC_TRUNCATION
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) XACT_COMMIT_MARKER);
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) sqn);
	PHLOG_FLUSH(tornbit, set, phlog_tornbit);
# else
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) XACT_COMMIT_MARKER);
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) sqn);
	PHLOG_FLUSH_ASYNCTRUNC(tornbit, set, phlog_tornbit);
# endif
	return M_R_SUCCESS;
}


static inline
m_result_t
m_tmlog_tornbit_abort(pcm_storeset_t *set, m_tmlog_tornbit_t *tmlog, uint64_t sqn)
{
	m_phlog_tornbit_t *phlog_tornbit = &(tmlog->phlog_tornbit);

# ifdef	SYNC_TRUNCATION
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) XACT_ABORT_MARKER);
	PHLOG_WRITE(tornbit, set, phlog_tornbit, (pcm_word_t) sqn);
	PHLOG_FLUSH(tornbit, set, phlog_tornbit);
# else
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) XACT_ABORT_MARKER);
	PHLOG_WRITE_ASYNCTRUNC(tornbit, set, phlog_tornbit, (pcm_word_t) sqn);
	PHLOG_FLUSH_ASYNCTRUNC(tornbit, set, phlog_tornbit);
# endif

	return M_R_SUCCESS;
}


static inline
m_result_t
m_tmlog_tornbit_truncate_sync(pcm_storeset_t *set, m_tmlog_tornbit_t *tmlog)
{
	m_phlog_tornbit_truncate_sync(set, &tmlog->phlog_tornbit);

	return M_R_SUCCESS;
}



m_result_t m_tmlog_tornbit_alloc (m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_init (pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_truncation_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_truncation_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_truncation_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_recovery_init(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_recovery_prepare_next(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_recovery_do(pcm_storeset_t *set, m_log_dsc_t *log_dsc);
m_result_t m_tmlog_tornbit_report_stats(m_log_dsc_t *log_dsc);


#endif /* _TMLOG_TORNBIT_H */
