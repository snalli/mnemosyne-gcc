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

/*!
 * \file 
 *
 * Implements asynchronous log truncation.
 */

#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sched.h>
#include "log_i.h"
#include "logtrunc.h"
#include "hal/pcm_i.h"
#include "phlog_tornbit.h"

static m_logmgr_t *logmgr;

static void *log_truncation_main (void *arg);

m_result_t
m_logtrunc_init(m_logmgr_t *mgr)
{
	logmgr = mgr;
	pthread_cond_init(&(logmgr->logtrunc_cond), NULL);
	//FIXME: Don't create asynchronous trunc thread when doing synchronous truncations
	//FIXME: SYNC_TRUNCATION preprocessor flag is not passed here
#ifndef SYNC_TRUNCATION
	pthread_create (&(logmgr->logtrunc_thread), NULL, &log_truncation_main, (void *) 0);
#endif
	return M_R_SUCCESS;
}

static 
m_result_t
truncate_logs (pcm_storeset_t *set, int lock)
{
	m_log_dsc_t       *log_dsc;
	m_log_dsc_t       *log_dsc_to_truncate;

	if (lock) {
		pthread_mutex_lock(&(logmgr->mutex));
	}	

	/* 
	 * First prepare each log for truncation.
	 * A log might then pass back a log truncation order number if it cares about 
	 * the order the truncation is performed with respect to other logs.
	 */
	list_for_each_entry(log_dsc, &(logmgr->active_logs_list), list) {
		if (log_dsc->flags & LF_ASYNC_TRUNCATION) {
			assert(log_dsc->ops);
			assert(log_dsc->ops->truncation_init);
			log_dsc->ops->truncation_init(set, log_dsc);
		}
	}
	/* 
	 * Find the next log to truncate, truncate it, update its truncation 
	 * order, and repeat until there are no more logs to truncate.
	 *
	 * TODO: This process can be made more efficient.
	 * TODO: This process should be performed per log type to allow coexistence 
	 *       of logs of different types
	 */
	do {
		log_dsc_to_truncate = NULL; 
		list_for_each_entry(log_dsc, &(logmgr->active_logs_list), list) {
			if (!(log_dsc->flags & LF_ASYNC_TRUNCATION)) {
				continue;
			}
			if (log_dsc->logorder == INV_LOG_ORDER) {
				continue;
			}
			if (log_dsc_to_truncate == NULL) {
				log_dsc_to_truncate = log_dsc;
			} else {
				if (log_dsc_to_truncate->logorder > log_dsc->logorder) {
					log_dsc_to_truncate = log_dsc;
				}
			}
		}
		if (log_dsc_to_truncate) {
			assert(log_dsc_to_truncate->ops);
			assert(log_dsc_to_truncate->ops->truncation_do);
			assert(log_dsc_to_truncate->ops->truncation_prepare_next);
			log_dsc_to_truncate->ops->truncation_do(set, log_dsc_to_truncate);
			log_dsc_to_truncate->ops->truncation_prepare_next(set, log_dsc_to_truncate);
		}	
	} while(log_dsc_to_truncate);

	if (lock) {
		pthread_mutex_unlock(&(logmgr->mutex));
	}	

	return M_R_SUCCESS;
}


/**
 * \brief Routine executed by the log truncation thread to truncate log
 * in the background.
 */
static
void *
log_truncation_main (void *arg)
{
	struct timeval     start_time;
	struct timeval     stop_time;
	struct timeval     tp;
	struct timespec    ts;
	pcm_storeset_t     *set;
	unsigned long long measured_time;

	set = pcm_storeset_get();

	cpu_set_t          cpu_set;

	CPU_ZERO(&cpu_set);
	CPU_SET(1 % 4, &cpu_set);
	sched_setaffinity(0, 4, &cpu_set);


/*
 * In the past, we tried to periodically wake-up and truncate the logs
 * but for the workloads we tried this didn't work well. Instead, we
 * now continuously loop and check for logs to truncate 
 */

	/* reset trunc statistics */
	logmgr->trunc_count=0;									 
	logmgr->trunc_time = 0;									 
#if 1
	pthread_mutex_lock(&(logmgr->mutex));

	while (1) {
		gettimeofday(&tp, NULL);
		ts.tv_sec = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000; 
		ts.tv_sec += 10; // sleep time 
		pthread_cond_timedwait(&logmgr->logtrunc_cond, &logmgr->mutex, &ts);
		//pthread_mutex_unlock(&(logmgr->mutex));
		//pthread_mutex_lock(&(logmgr->mutex));

		gettimeofday(&start_time, NULL);
		truncate_logs(set, 0);
		gettimeofday(&stop_time, NULL);
		measured_time = 1000000 * (stop_time.tv_sec - start_time.tv_sec) +
		                                     stop_time.tv_usec - start_time.tv_usec;
		logmgr->trunc_count++;									 
		logmgr->trunc_time += measured_time;									 
	}	

	pthread_mutex_unlock(&(logmgr->mutex));
#endif

#if 0
	while (1) {
		pthread_mutex_lock(&(logmgr->mutex));
		truncate_logs(set, 0);
		pthread_mutex_unlock(&(logmgr->mutex));
	}	
#endif

	return 0;
}


/**
 * \brief Force log truncation
 */
m_result_t
m_logtrunc_truncate(pcm_storeset_t *set)
{
	assert(0 && "m_logtrunc_truncate no longer used");
	//return truncate_logs(set, 1);
}


m_result_t
m_logtrunc_signal()
{
	// We don't worry about lost signals, as if the signal is lost, then 
	// the async trunc thread was already truncating the log 
	pthread_cond_signal(&logmgr->logtrunc_cond);
}
