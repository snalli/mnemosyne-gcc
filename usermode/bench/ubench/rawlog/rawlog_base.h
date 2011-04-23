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
