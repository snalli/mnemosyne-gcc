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
#include <cuckoo_hash/PointerHashInline.h>
#include <debug.h>
#include "rawlog_tornbit.h"

m_log_ops_t rawlog_tornbit_ops = {
	m_rawlog_tornbit_alloc,
	m_rawlog_tornbit_init,
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
m_rawlog_tornbit_quickreset (pcm_storeset_t *set, m_rawlog_tornbit_t *rawlog)
{
	m_phlog_tornbit_t *phlog = &rawlog->phlog_tornbit;

	phlog->nvmd->flags = 0 | TORNBIT_ONE,
	phlog->tornbit = LF_TORNBIT & phlog->nvmd->flags;
	phlog->buffer_count = 0;
	phlog->write_remainder = 0x0;
	phlog->write_remainder_nbits = 0;
	phlog->read_remainder = 0x0;
	phlog->read_remainder_nbits = 0;
	phlog->head = phlog->tail = phlog->stable_tail = phlog->read_index = phlog->nvmd->flags & LF_HEAD_MASK;

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_tornbit_alloc(m_log_dsc_t *log_dsc)
{
	m_rawlog_tornbit_t *rawlog_tornbit;

	if (posix_memalign((void **) &rawlog_tornbit, sizeof(uint64_t), sizeof(m_rawlog_tornbit_t)) != 0) 
	{
		return M_R_FAILURE;
	}
	/* 
	 * The underlying physical log volatile structure requires to be
	 * word aligned.
	 */
	assert((( (uintptr_t) &rawlog_tornbit->phlog_tornbit) & (sizeof(uint64_t)-1)) == 0);
	log_dsc->log = (m_log_t *) rawlog_tornbit;

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_tornbit_init(pcm_storeset_t *set, m_log_t *log, m_log_dsc_t *log_dsc)
{
	m_rawlog_tornbit_t *rawlog_tornbit = (m_rawlog_tornbit_t *) log;
	m_phlog_tornbit_t *phlog_tornbit = &(rawlog_tornbit->phlog_tornbit);

	m_phlog_tornbit_format(set, 
	                       (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                       log_dsc->nvphlog, 
	                       LF_TYPE_TM_TORNBIT);
	m_phlog_tornbit_init(phlog_tornbit, 
	                     (m_phlog_tornbit_nvmd_t *) log_dsc->nvmd, 
	                     log_dsc->nvphlog);

	return M_R_SUCCESS;
}


m_result_t 
m_rawlog_tornbit_register(pcm_storeset_t *pcm_storeset)
{
	m_logmgr_register_logtype(pcm_storeset, LF_TYPE_TM_TORNBIT, &rawlog_tornbit_ops);

	return M_R_SUCCESS;
}
