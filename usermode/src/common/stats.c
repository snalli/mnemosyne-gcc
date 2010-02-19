/**
 * \file stats.c
 *
 * \brief Statistics collector implementation.
 *
 */

#include <string.h>
#include <core/tx.h>
#include <core/txdesc.h>
#include <core/stats.h>
#include <core/config.h>
#include <misc/hash_table.h>
#include <misc/generic_types.h>
#include <misc/malloc.h>
#include <misc/mutex.h>
#include <misc/debug.h>

txc_statsmgr_t *txc_g_statsmgr;

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
struct txc_stats_threadstat_s {
	txc_stats_statcounter_t       count;     /**< Number of collected transactions. */
	txc_stats_stat_t              total_stats[txc_stats_numofstats]; /**< Summary of statistics */
	txc_hash_table_t              *tx_stats; /**< Collected statistics */
	struct txc_stats_threadstat_s *next;     /**< Used to implement the list of thread statistics. */
	struct txc_stats_threadstat_s *prev;     /**< Used to implement the list of thread statistics. */
	txc_tx_t                      *txd;      /**< The descriptor of the thread. */
};


/** Statistics manager */
struct txc_statsmgr_s {
	txc_mutex_t            mutex;                       /**< Serializes accesses to this structure */
	unsigned int           alloc_threadstat_num;        /**< Number of threads collecting statistics for */
	txc_stats_threadstat_t *alloc_threadstat_list_head; /**< Head of the thread statistics list */
	txc_stats_threadstat_t *alloc_threadstat_list_tail; /**< Tail of the thread statistics list */
};


txc_result_t
txc_statsmgr_create(txc_statsmgr_t **statsmgrp)
{
	*statsmgrp = (txc_statsmgr_t *) MALLOC(sizeof(txc_statsmgr_t));
	if (*statsmgrp == NULL) {
		return TXC_R_NOMEMORY;
	}

	(*statsmgrp)->alloc_threadstat_num = 0;
	(*statsmgrp)->alloc_threadstat_list_head = (*statsmgrp)->alloc_threadstat_list_tail = NULL;
	TXC_MUTEX_INIT(&(*statsmgrp)->mutex, NULL);

	return TXC_R_SUCCESS;
}


txc_result_t
txc_statsmgr_destroy(txc_statsmgr_t **statsmgrp)
{
	txc_stats_threadstat_t *threadstat;
	txc_stats_threadstat_t *threadstat_next;
	txc_statsmgr_t         *statsmgr = *statsmgrp;


	for (threadstat=statsmgr->alloc_threadstat_list_head;
	     threadstat != NULL;
		 threadstat = threadstat_next)
	{
		threadstat_next = threadstat->next;
		FREE(threadstat);
	}

	FREE(statsmgr);
	*statsmgrp = NULL;

	return TXC_R_SUCCESS;
}


txc_result_t
txc_stats_threadstat_create(txc_statsmgr_t *statsmgr, 
                            txc_stats_threadstat_t **threadstatp,
                            txc_tx_t *txd)
{
	txc_stats_threadstat_t *threadstat;
	int                    i;

	threadstat = (txc_stats_threadstat_t *) MALLOC(sizeof(txc_stats_threadstat_t));

	TXC_MUTEX_LOCK(&(statsmgr->mutex));
	if (statsmgr->alloc_threadstat_list_head == NULL) {
		TXC_ASSERT(statsmgr->alloc_threadstat_list_tail == NULL);
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
	TXC_MUTEX_UNLOCK(&(statsmgr->mutex));

	threadstat->count = 0;					  
	threadstat->txd = txd;
	txc_hash_table_create(&threadstat->tx_stats, 
	                      TXC_STATS_THREADSTAT_HASHTABLE_SIZE, 
						  TXC_BOOL_FALSE);
	for (i=0; i<txc_stats_numofstats; i++) {
		threadstat->total_stats[i].total = 0;
		threadstat->total_stats[i].min       = 0xFFFFFFFF;
		threadstat->total_stats[i].max       = 0;
	}	

	*threadstatp = threadstat;
	return TXC_R_SUCCESS;					  
}


txc_result_t
txc_stats_txstat_create(txc_stats_txstat_t **txstatp)
{
	txc_stats_txstat_t *txstat;

	txstat = (txc_stats_txstat_t *) MALLOC(sizeof(txc_stats_txstat_t));
	txstat->count = 0;
	*txstatp = txstat;

	return TXC_R_SUCCESS;					  
}


txc_result_t
txc_stats_txstat_destroy(txc_stats_txstat_t **txstatp)
{
	FREE(*txstatp);
	*txstatp = NULL;

	return TXC_R_SUCCESS;					  
}


static
void
stats_txstat_reset(txc_stats_txstat_t *txstat)
{
#define RESETSTAT(name)                                                      \
  txstat->total_stats[txc_stats_##name##_stat].total = 0;                    \
  txstat->total_stats[txc_stats_##name##_stat].min = 0xFFFFFFFF;             \
  txstat->total_stats[txc_stats_##name##_stat].max = 0;

	FOREACH_STAT (RESETSTAT)

#undef RESETSTAT	
}


txc_result_t
txc_stats_txstat_init(txc_stats_txstat_t *txstat, 
                      const char *srcloc_str, 
                      txc_tx_srcloc_t *srcloc)
{
	if (txc_runtime_settings.statistics == TXC_BOOL_TRUE) {
		txstat->count = 1;
		txstat->srcloc_str = srcloc_str;
		txstat->srcloc.line = srcloc->line;
		txstat->srcloc.fun = srcloc->fun;
		txstat->srcloc.file = srcloc->file;
		stats_txstat_reset(txstat);
	}
	return TXC_R_SUCCESS;
}


static
void
stats_get_txstat_container(txc_hash_table_t *tx_stats,
                           txc_stats_txstat_t *txstat, 
                           txc_stats_txstat_t **txstat_containerp)
{
	txc_hash_table_key_t   key;
	txc_hash_table_value_t value;
	txc_stats_txstat_t     *txstat_container;

	/* 
	 * Find the txstat container of the static transaction, and if there is
	 * no such container create it and insert it into the hash table.
	 */
	key = (txc_hash_table_key_t) txstat->srcloc_str;
	if (txc_hash_table_lookup(tx_stats, key, &value) 
	    == TXC_R_SUCCESS)
	{
		txstat_container = (txc_stats_txstat_t *) value;	
	} else {
		txstat_container = (txc_stats_txstat_t *) MALLOC(sizeof(txc_stats_txstat_t));
		stats_txstat_reset(txstat_container);
		txstat_container->count = 0;
		txstat_container->srcloc_str = txstat->srcloc_str;
		txstat_container->srcloc.line = txstat->srcloc.line;
		txstat_container->srcloc.fun = txstat->srcloc.fun;
		txstat_container->srcloc.file = txstat->srcloc.file;
		txc_hash_table_add(tx_stats, 
		                   key, 
		                   (txc_hash_table_value_t) (txstat_container));
	}

	*txstat_containerp = txstat_container;
}


static
void 
statsmgr_commit_action(void *args, int *result)
{
	txc_tx_t               *txd = (txc_tx_t *) args;
	txc_stats_txstat_t     *txstat = txd->txstat;
	txc_stats_txstat_t     *txstat_all;
	txc_stats_threadstat_t *threadstat = txd->threadstat;
	int                    i;


	/* 
	 * Find the txstat container of the static transaction, and if there is
	 * no such container create it and insert it into the hash table.
	 */
	stats_get_txstat_container(threadstat->tx_stats, txstat, &txstat_all);

	/* 
	 * Collect the statistics of this transaction instance into its static 
	 * container. 
	 */
	txstat_all->count++;
	for (i=0; i<txc_stats_numofstats; i++) {
		txstat_all->total_stats[i].total += txstat->total_stats[i].total;
		txstat_all->total_stats[i].min = MIN(txstat_all->total_stats[i].min,
		                                     txstat->total_stats[i].total);
		txstat_all->total_stats[i].max = MAX(txstat_all->total_stats[i].max,
		                                     txstat->total_stats[i].total);
	}

	/* 
	 * Collect the statistics of this transaction instance into the thread's 
	 * statistics container. 
	 */
	threadstat->count++;
	for (i=0; i<txc_stats_numofstats; i++) {
		threadstat->total_stats[i].total += txstat->total_stats[i].total;
		threadstat->total_stats[i].min = MIN(threadstat->total_stats[i].min,
		                                     txstat->total_stats[i].total);
		threadstat->total_stats[i].max = MAX(threadstat->total_stats[i].max,
		                                     txstat->total_stats[i].total);
	}
	if (result) {
		*result = 0;
	}
}


static 
void 
statsmgr_undo_action(void *args, int *result)
{
	txc_tx_t *txd = (txc_tx_t *) args;

	/* reset transaction statistics */

	stats_txstat_reset(txd->txstat);

	if (result) {
		*result = 0;
	}
}


static inline
void
register_statsmgr_commit_action(txc_tx_t *txd)
{
	if (txd->statsmgr_commit_action_registered == 0) {
		txc_tx_register_commit_action(txd, 
		                              statsmgr_commit_action, 
		                              (void *) txd,
		                              NULL,
	                                  TXC_STATSMGR_COMMIT_ACTION_ORDER);
		txd->statsmgr_commit_action_registered = 1;							  
	}
}


static inline
void
register_statsmgr_undo_action(txc_tx_t *txd)
{
	if (txd->statsmgr_undo_action_registered == 0) {
		txc_tx_register_undo_action(txd, 
	                                statsmgr_undo_action,
		                            (void *) txd,
		                             NULL,
	                                 TXC_STATSMGR_UNDO_ACTION_ORDER);
		txd->statsmgr_undo_action_registered = 1;							  
	}
}


void
txc_stats_register_statsmgr_commit_action(txc_tx_t *txd)
{
	register_statsmgr_commit_action(txd);
}


void
txc_stats_register_statsmgr_undo_action(txc_tx_t *txd)
{
	register_statsmgr_undo_action(txd);
}


void
txc_stats_transaction_postbegin(txc_tx_t *txd)
{
	if (txc_runtime_settings.statistics == TXC_BOOL_TRUE)
    {
		register_statsmgr_commit_action(txd);
		register_statsmgr_undo_action(txd);
	}	
}


static
void
stats_txstat_print(FILE *fout, 
                   txc_stats_txstat_t *txstat, 
				   int shiftlen,
                   txc_bool_t print_srcloc)
{
	int                     i;
	char                    header[512];
	double                  mean;
	txc_stats_statcounter_t max;
	txc_stats_statcounter_t min;
	txc_stats_statcounter_t total;
	txc_stats_statcounter_t count;

	if (print_srcloc) {
		sprintf(header, "Source is line %d in function %s in %s", 
		        txstat->srcloc.line, 
				txstat->srcloc.fun,
	    	    txstat->srcloc.file);
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

	count = txstat->count;

	fprintf(fout, "%s%s%s:%13s%13s%13s%13u\n", 
	        WHITESPACE(shiftlen+2),
	        "Transactions",
			WHITESPACE(25 - strlen("Transactions")),
			"",
			"",
			"",
			count);
	
	for (i=0; i<txc_stats_numofstats; i++) {
		total = txstat->total_stats[i].total;
		min = txstat->total_stats[i].min;
		max = txstat->total_stats[i].max;
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
                       txc_stats_threadstat_t *threadstat) 
{
	txc_hash_table_iter_t  iter;
	txc_hash_table_key_t   key;
	txc_hash_table_value_t value;
	txc_stats_txstat_t     *txstat;
	txc_stats_txstat_t     txstat_all;
	int                    i;

	txstat_all.count = threadstat->count;
	for (i=0; i<txc_stats_numofstats; i++) {
		txstat_all.total_stats[i] = threadstat->total_stats[i];
	}	
	fprintf(fout, "Thread %u\n", threadstat->txd->tid);
	stats_txstat_print(fout, &txstat_all, 0, TXC_BOOL_FALSE);
	fprintf(fout, "\n");
	fprintf(fout, "  Transactions for thread %u\n\n", threadstat->txd->tid);
	
	txc_hash_table_iter_init(threadstat->tx_stats, &iter);
	while(TXC_R_SUCCESS == txc_hash_table_iter_next(&iter, &key, &value)) {
		txstat = (txc_stats_txstat_t *) value;
		stats_txstat_print(fout, txstat, 4, TXC_BOOL_TRUE);
		fprintf(fout, "\n");
	}
}


static
void
stats_summarize_all(txc_statsmgr_t *statsmgr, txc_stats_threadstat_t *summary)
{
	txc_stats_threadstat_t *threadstat;
	txc_hash_table_iter_t  iter;
	txc_stats_txstat_t     *txstat;
	txc_stats_txstat_t     *txstat_summary;
	txc_hash_table_key_t   key;
	txc_hash_table_value_t value;
	int                    i;

	for (threadstat=statsmgr->alloc_threadstat_list_head;
	     threadstat;
		 threadstat = threadstat->next)
	{
		txc_hash_table_iter_init(threadstat->tx_stats, &iter);
		while(TXC_R_SUCCESS == txc_hash_table_iter_next(&iter, &key, &value)) {
			txstat = (txc_stats_txstat_t *) value;
			stats_get_txstat_container(summary->tx_stats, txstat, &txstat_summary);
			txstat_summary->count += txstat->count;
			summary->count += txstat->count;
			for (i=0; i<txc_stats_numofstats; i++) {
				txstat_summary->total_stats[i].total += txstat->total_stats[i].total;
				txstat_summary->total_stats[i].min = MIN(txstat_summary->total_stats[i].min,
				                                         txstat->total_stats[i].min);
				txstat_summary->total_stats[i].max = MAX(txstat_summary->total_stats[i].max,
				                                         txstat->total_stats[i].max);

				summary->total_stats[i].total += txstat->total_stats[i].total;
				summary->total_stats[i].min = MIN(summary->total_stats[i].min,
				                                  txstat->total_stats[i].min);
				summary->total_stats[i].max = MAX(summary->total_stats[i].max,
				                                  txstat->total_stats[i].max);
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
txc_stats_print(txc_statsmgr_t *statsmgr)
{
	txc_hash_table_iter_t  iter;
	txc_stats_threadstat_t *threadstat;
	txc_stats_threadstat_t summary;
	txc_hash_table_key_t   key;
	txc_hash_table_value_t value;
	txc_stats_txstat_t     *txstat;
	txc_stats_txstat_t     txstat_grand_total;
	int                    i;
	FILE                   *fout;

	if (txc_runtime_settings.statistics == TXC_BOOL_FALSE) {
		return;
	}

	fout = fopen(txc_runtime_settings.statistics_file, "w");

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
	txc_hash_table_create(&(summary.tx_stats),
	                      TXC_STATS_THREADSTAT_HASHTABLE_SIZE, 
						  TXC_BOOL_FALSE);
	for (i=0; i<txc_stats_numofstats; i++) {
		summary.total_stats[i].min = 0;
		summary.total_stats[i].max = 0;
		summary.total_stats[i].total = 0;
		summary.count = 0;
	}	
	stats_summarize_all(statsmgr, &summary);
	txc_hash_table_iter_init(summary.tx_stats, &iter);
	while(TXC_R_SUCCESS == txc_hash_table_iter_next(&iter, &key, &value)) {
		txstat = (txc_stats_txstat_t *) value;
		stats_txstat_print(fout, txstat, 0, TXC_BOOL_TRUE);
		fprintf(fout, "\n");
	}

	/* Print GRAND totals */
	fprintf(fout, "GRAND TOTAL (all transactions, all threads)\n\n");
	txstat_grand_total.count = summary.count;
	for (i=0; i<txc_stats_numofstats; i++) {
		txstat_grand_total.total_stats[i] = summary.total_stats[i];
	}	
	stats_txstat_print(fout, &txstat_grand_total, 0, TXC_BOOL_FALSE);
	
	fclose(fout);
}	
