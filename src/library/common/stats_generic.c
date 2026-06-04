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

/**
 * \file stats.c
 *
 * \brief Statistics collector implementation.
 *
 * FIXME: This statistics collector was meant to be as generic as possible
 * but it seems that it didn't live up to its promise. This is because the 
 * logic is based on the logic of the statistics collector used in the 
 * transaction statistics collector used by MTM. For the future, we would 
 * like to implement a more extensible statistics collector similar to the
 * one used by Solaris kstat provider.
 *
 */

#include <stdbool.h>
#include <string.h>
#include "stats_generic.h"
#include "chhash.h"
#include "util.h"
#include "debug.h"

m_statsmgr_t *m_g_statsmgr;

#define M_STATS_THREADSTAT_HASHTABLE_SIZE 128

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#define ACTION(name) #name,
char *stats_strings[] = {
	FOREACH_STAT(ACTION)
};	
#undef ACTION	

static const char __whitespaces[] = "                                                              ";

#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]

/** Thread statistics */
struct m_stats_threadstat_s {
    unsigned int tid;
	m_stats_statset_t           summary_statset; /**< Statistics summary */
	m_chhash_t                  *stats_table; /**< Collected statistics */
	struct m_stats_threadstat_s *next;     /**< Used to implement the list of thread statistics. */
	struct m_stats_threadstat_s *prev;     /**< Used to implement the list of thread statistics. */
};


/** Statistics manager */
struct m_statsmgr_s {
	m_mutex_t            mutex;                       /**< Serializes accesses to this structure */
	char                 *output_file;
	unsigned int         alloc_threadstat_num;        /**< Number of threads collecting statistics for */
	m_stats_threadstat_t *alloc_threadstat_list_head; /**< Head of the thread statistics list */
	m_stats_threadstat_t *alloc_threadstat_list_tail; /**< Tail of the thread statistics list */
};


m_result_t
m_statsmgr_create(m_statsmgr_t **statsmgrp, char *output_file)
{
	*statsmgrp = (m_statsmgr_t *) MALLOC(sizeof(m_statsmgr_t));
	if (*statsmgrp == NULL) {
		return M_R_NOMEMORY;
	}

	(*statsmgrp)->output_file = output_file;
	(*statsmgrp)->alloc_threadstat_num = 0;
	(*statsmgrp)->alloc_threadstat_list_head = (*statsmgrp)->alloc_threadstat_list_tail = NULL;
	M_MUTEX_INIT(&(*statsmgrp)->mutex, NULL);

	return M_R_SUCCESS;
}


m_result_t
m_statsmgr_destroy(m_statsmgr_t **statsmgrp)
{
	m_stats_threadstat_t *threadstat;
	m_stats_threadstat_t *threadstat_next;
	m_statsmgr_t         *statsmgr = *statsmgrp;


	for (threadstat=statsmgr->alloc_threadstat_list_head;
	     threadstat != NULL;
		 threadstat = threadstat_next)
	{
		threadstat_next = threadstat->next;
		FREE(threadstat);
	}

	FREE(statsmgr);
	*statsmgrp = NULL;

	return M_R_SUCCESS;
}


m_result_t
m_stats_threadstat_create(m_statsmgr_t *statsmgr, 
                          unsigned int tid, 
                          m_stats_threadstat_t **threadstatp)
{
	m_stats_threadstat_t *threadstat;
	int                  i;

	threadstat = (m_stats_threadstat_t *) MALLOC(sizeof(m_stats_threadstat_t));

	M_MUTEX_LOCK(&(statsmgr->mutex));
	if (statsmgr->alloc_threadstat_list_head == NULL) {
		M_ASSERT(statsmgr->alloc_threadstat_list_tail == NULL);
		statsmgr->alloc_threadstat_list_head = threadstat;
		statsmgr->alloc_threadstat_list_tail = threadstat;
		threadstat->next = NULL;
		threadstat->prev = NULL;
	} else {
		statsmgr->alloc_threadstat_list_tail->next = threadstat;
		threadstat->prev = statsmgr->alloc_threadstat_list_tail;
		statsmgr->alloc_threadstat_list_tail = threadstat;
	}
	statsmgr->alloc_threadstat_num++;
	M_MUTEX_UNLOCK(&(statsmgr->mutex));

	m_stats_statset_init(&(threadstat->summary_statset), NULL);
	threadstat->tid = tid;
	m_chhash_create(&threadstat->stats_table, 
	                M_STATS_THREADSTAT_HASHTABLE_SIZE, 
					false);
	*threadstatp = threadstat;
	return M_R_SUCCESS;					  
}


m_result_t
m_stats_statset_create(m_stats_statset_t **statsetp)
{
	m_stats_statset_t *statset;

	statset = (m_stats_statset_t *) MALLOC(sizeof(m_stats_statset_t));
	statset->count = 0;
	*statsetp = statset;

	return M_R_SUCCESS;					  
}


m_result_t
m_stats_statset_destroy(m_stats_statset_t **statsetp)
{
	FREE(*statsetp);
	*statsetp = NULL;

	return M_R_SUCCESS;					  
}


static
void
stats_statset_reset(m_stats_statset_t *statset)
{
#define RESETSTAT(name)                                                      \
  statset->stats[m_stats_##name##_stat].total = 0;                    \
  statset->stats[m_stats_##name##_stat].min = 0xFFFFFFFF;             \
  statset->stats[m_stats_##name##_stat].max = 0;

	FOREACH_STAT (RESETSTAT)

#undef RESETSTAT	
}


m_result_t
m_stats_statset_init(m_stats_statset_t *statset, 
                     const char *name)
{
	statset->count = 0;
	statset->name = name;
	stats_statset_reset(statset);
	return M_R_SUCCESS;
}


static
m_result_t
stats_get_statset(m_chhash_t *stats_table,
                  char *name, 
                  m_stats_statset_t **statsetp)
{
	m_chhash_key_t    key;
	m_chhash_value_t  value;
	m_stats_statset_t *statset;

	key = (m_chhash_key_t) name;
	if (m_chhash_lookup(stats_table, key, &value) == M_R_SUCCESS)
	{
		statset = (m_stats_statset_t *) value;	
		*statsetp = statset;
		return M_R_SUCCESS;
	}
	return M_R_FAILURE;
}


/* 
 * Collect the statistics of source_statset into dest_statset.
 */
void 
stats_aggregate(m_stats_statset_t *dest_statset, 
                m_stats_statset_t *source_statset)
{
	int i;

	dest_statset->count++;
	for (i=0; i<m_stats_numofstats; i++) {
		dest_statset->stats[i].total += source_statset->stats[i].total;
		dest_statset->stats[i].min = MIN(dest_statset->stats[i].min,
		                                 source_statset->stats[i].total);
		dest_statset->stats[i].max = MAX(dest_statset->stats[i].max,
		                                 source_statset->stats[i].total);
	}
}


void 
m_stats_threadstat_aggregate(m_stats_threadstat_t *threadstat, m_stats_statset_t *source_statset)
{
	m_stats_statset_t    *statset_all;
	m_result_t           result;


	result = stats_get_statset(threadstat->stats_table, source_statset->name, &statset_all);
	if (result != M_R_SUCCESS) {
		m_stats_statset_create(&statset_all);
		m_stats_statset_init(statset_all, source_statset->name);
		m_chhash_add(threadstat->stats_table, 
		             source_statset->name, 
		             (m_chhash_value_t) (statset_all));
	}

	
	stats_aggregate(statset_all, source_statset);
	stats_aggregate(&threadstat->summary_statset, source_statset);
}



static
void
m_stats_statset_print(FILE *fout, 
                      m_stats_statset_t *statset, 
                      int shiftlen,
                      int print_header)
{
	int                     i;
	char                    header[512];
	double                  mean;
	m_stats_statcounter_t max;
	m_stats_statcounter_t min;
	m_stats_statcounter_t total;
	m_stats_statcounter_t count;

	if (print_header) {
		sprintf(header, "Transaction: %s", statset->name);
		fprintf(fout, "%s%s\n\n", WHITESPACE(shiftlen), header);
	}

	fprintf(fout, "%s%s%s:%13s%13s%13s%13s\n", 
	        WHITESPACE(shiftlen+2),
	        "",
			WHITESPACE(25),
			"Min",
			"Mean",
			"Max",
			"Total");

	count = statset->count;

	fprintf(fout, "%s%s%s:%13s%13s%13s%13u\n", 
	        WHITESPACE(shiftlen+2),
	        "Transactions",
			WHITESPACE(25 - strlen("Transactions")),
			"",
			"",
			"",
			count);
	
	for (i=0; i<m_stats_numofstats; i++) {
		total = statset->stats[i].total;
		min = statset->stats[i].min;
		max = statset->stats[i].max;
		mean = (double) total / (double) count;
		fprintf(fout, "%s%s%s:%13u%13.2f%13u%13u\n", 
		        WHITESPACE(shiftlen+2),
		        stats_strings[i],
				WHITESPACE(25 - strlen(stats_strings[i])),
				min,
				mean,
				max,
				total);
	}
}


static
void
stats_threadstat_print(FILE *fout, 
                       m_stats_threadstat_t *threadstat) 
{
	m_chhash_iter_t   iter;
	m_chhash_key_t    key;
	m_chhash_value_t  value;
	m_stats_statset_t *statset;
	m_stats_statset_t statset_all;
	int               i;

	fprintf(fout, "Thread %u\n", threadstat->tid);
	m_stats_statset_print(fout, &threadstat->summary_statset, 0, false);
	fprintf(fout, "\n");
	fprintf(fout, "  Transactions for thread %u\n\n", threadstat->tid);
	
	m_chhash_iter_init(threadstat->stats_table, &iter);
	while(M_R_SUCCESS == m_chhash_iter_next(&iter, &key, &value)) {
		statset = (m_stats_statset_t *) value;
		m_stats_statset_print(fout, statset, 4, true);
		fprintf(fout, "\n");
	}
}


static
void
stats_summarize_all(m_statsmgr_t *statsmgr, m_stats_threadstat_t *summary)
{
	m_stats_threadstat_t *threadstat;
	m_chhash_iter_t      iter;
	m_stats_statset_t    *statset;
	m_stats_statset_t    *statset_summary;
	m_chhash_key_t       key;
	m_chhash_value_t     value;
	int                  i;
	m_result_t           result;


	for (threadstat=statsmgr->alloc_threadstat_list_head;
	     threadstat;
		 threadstat = threadstat->next)
	{
		m_chhash_iter_init(threadstat->stats_table, &iter);
		while(M_R_SUCCESS == m_chhash_iter_next(&iter, &key, &value)) {
			statset = (m_stats_statset_t *) value;
			result = stats_get_statset(summary->stats_table, 
			                           statset->name, &statset_summary);
			if (result != M_R_SUCCESS) {
				m_stats_statset_create(&statset_summary);
				m_stats_statset_init(statset_summary, statset->name);
				m_chhash_add(summary->stats_table, 
			                 statset->name, 
			                 (m_chhash_value_t) (statset_summary));
			}
			statset_summary->count += statset->count;
			summary->summary_statset.count += statset->count;
			for (i=0; i<m_stats_numofstats; i++) {
				statset_summary->stats[i].total += statset->stats[i].total;
				statset_summary->stats[i].min = MIN(statset_summary->stats[i].min,
				                                         statset->stats[i].min);
				statset_summary->stats[i].max = MAX(statset_summary->stats[i].max,
				                                         statset->stats[i].max);

				summary->summary_statset.stats[i].total += statset->stats[i].total;
				summary->summary_statset.stats[i].min = MIN(summary->summary_statset.stats[i].min,
				                                            statset->stats[i].min);
				summary->summary_statset.stats[i].max = MAX(summary->summary_statset.stats[i].max,
				                                            statset->stats[i].max);
			}
		}
	}	
}


/**
 * \brief Prints a statistics report.
 *
 * Summarizes statistics and prints a report into the statistics file.
 * Print the statistics report in the same format as the Intel STM's one 
 * to avoid duplicating post-processing scripts.
 *
 * \param[in] statsmgr Pointer to the statistics manager collecting the 
 *            statistics.
 * \return None
 */
void
m_stats_print(m_statsmgr_t *statsmgr)
{
	m_chhash_iter_t      iter;
	m_stats_threadstat_t *threadstat;
	m_stats_threadstat_t summary;
	m_chhash_key_t       key;
	m_chhash_value_t     value;
	m_stats_statset_t    *statset;
	m_stats_statset_t    statset_grand_total;
	int                  i;
	FILE                 *fout;

	if (statsmgr->output_file) {
		fout = fopen(statsmgr->output_file, "w");
	} else {	
		fout = stderr;
	}	
	/* Print per THREAD per TRANSACTION totals */
	fprintf(fout, "STATS REPORT\n");
	fprintf(fout, "THREAD TOTALS\n\n");

	for (threadstat=statsmgr->alloc_threadstat_list_head;
	     threadstat;
		 threadstat = threadstat->next)
	{	
		stats_threadstat_print(fout, threadstat);
		fprintf(fout, "\n");
	}

	/* Print per TRANSACTION totals */
	fprintf(fout, "TRANSACTION TOTALS\n\n");
	m_chhash_create(&(summary.stats_table),
	                M_STATS_THREADSTAT_HASHTABLE_SIZE, 
                    false);
	m_stats_statset_init(&summary.summary_statset, NULL);					  
	stats_summarize_all(statsmgr, &summary);
	m_chhash_iter_init(summary.stats_table, &iter);
	while(M_R_SUCCESS == m_chhash_iter_next(&iter, &key, &value)) {
		statset = (m_stats_statset_t *) value;
		m_stats_statset_print(fout, statset, 0, true);
		fprintf(fout, "\n");
	}

	/* Print GRAND totals */
	fprintf(fout, "GRAND TOTAL (all transactions, all threads)\n\n");
	statset_grand_total.count = summary.summary_statset.count;
	for (i=0; i<m_stats_numofstats; i++) {
		statset_grand_total.stats[i] = summary.summary_statset.stats[i];
	}	
	m_stats_statset_print(fout, &statset_grand_total, 0, false);
	if (statsmgr->output_file) {
		fclose(fout);
	}	
}	
