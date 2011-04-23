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

#include <stdio.h>
#include <assert.h>
#include <mnemosyne.h>
#include <pcm.h>
#include <debug.h>
#include "rawlog_base.h"

m_log_ops_t rawlog_base_ops = {
	m_rawlog_base_alloc,
	m_rawlog_base_init,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

/* 
 * This is just a hack for use by this nanobenchmark. 
 * To be able to measure throughput of writing to the log we need a whole bunch
 * of write operations. But since memory is FAST we wrap around the log quickly. 
 * So we quickly reset the log to deal with wraps quickly since we don't want 
 * to measure the truncation overhead.
 */
m_result_t 
m_rawlog_base_quickreset (pcm_storeset_t *set, m_rawlog_base_t *rawlog)
{
	m_phlog_base_t *phlog = &rawlog->phlog_base;

	phlog->nvmd->head = 0;
	phlog->nvmd->tail = 0;
	phlog->buffer_count = 0;
	phlog->head = phlog->nvmd->head;
	phlog->tail = phlog->nvmd->tail;
	phlog->read_index = phlog->nvmd->head;

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_base_alloc(m_log_dsc_t *log_dsc)
{
	m_rawlog_base_t *rawlog_base;

	if (posix_memalign((void **) &rawlog_base, sizeof(uint64_t), sizeof(m_rawlog_base_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	/* 
	 * The underlying physical log volatile structure requires to be
	 * word aligned.
	 */
	assert((( (uintptr_t) &rawlog_base->phlog_base) & (sizeof(uint64_t)-1)) == 0);
	log_dsc->log = (m_log_t *) rawlog_base;

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_base_init(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc)
{
	m_rawlog_base_t *rawlog_base = (m_rawlog_base_t *) log;
	m_phlog_base_t *phlog_base = &(rawlog_base->phlog_base);

	m_phlog_base_format(set, 
	                    (m_phlog_base_nvmd_t *) log_dsc->nvmd, 
	                    log_dsc->nvphlog, 
	                    LF_TYPE_TM_BASE);
	m_phlog_base_init(phlog_base, 
	                  (m_phlog_base_nvmd_t *) log_dsc->nvmd, 
	                  log_dsc->nvphlog);

	return M_R_SUCCESS;
}

m_result_t 
m_rawlog_base_register(pcm_storeset_t *pcm_storeset)
{
	m_logmgr_register_logtype(pcm_storeset, LF_TYPE_TM_BASE, &rawlog_base_ops);
	return M_R_SUCCESS;
}	
