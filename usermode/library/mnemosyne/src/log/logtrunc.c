/*!
 * \file 
 *
 * Implements log truncation.
 */

#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "log.h"
#include "logtrunc.h"

static m_logmgr_t *logmgr;

static void log_truncation_main (void *arg);

m_result_t
m_logtrunc_init(m_logmgr_t *mgr)
{
	logmgr = mgr;
	pthread_cond_init(&(logmgr->logtrunc_cond), NULL);
	pthread_create (&(logmgr->logtrunc_thread), NULL, (void *) &log_truncation_main, (void *) 0);

	return M_R_SUCCESS;
}

static 
m_result_t
truncate_logs (int lock)
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
		assert(log_dsc->ops);
		assert(log_dsc->ops->truncation_init);
		log_dsc->ops->truncation_init(log_dsc);
	}
	/* 
	 * Find the next log to truncate, truncate it, update its truncation 
	 * order, and repeat until there are no more logs to truncate.
	 *
	 * TODO: This process can be made more efficient.
	 * TODO: This process should be performed per log type.
	 */
	do {
		log_dsc_to_truncate = NULL; 
		list_for_each_entry(log_dsc, &(logmgr->active_logs_list), list) {
			printf("log_dsc                 = %p\n", log_dsc);
			printf("log_dsc->logorder = 0x%lX\n", log_dsc->logorder);
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
			log_dsc_to_truncate->ops->truncation_do(log_dsc_to_truncate);
			log_dsc_to_truncate->ops->truncation_prepare_next(log_dsc_to_truncate);
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
void 
log_truncation_main (void *arg)
{
	struct timeval    tp;
	struct timespec   ts;
	int               rc;

	pthread_mutex_lock(&(logmgr->mutex));

	while (1) {
		rc =  gettimeofday(&tp, NULL);
		ts.tv_sec = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000; 
		ts.tv_sec += 1;
		pthread_cond_timedwait(&logmgr->logtrunc_cond, &logmgr->mutex, &ts);

		truncate_logs(0);
	}	

	pthread_mutex_unlock(&(logmgr->mutex));
}


/**
 * \brief Force log truncation
 */
m_result_t
m_logtrunc_truncate()
{
	return truncate_logs(1);
}
