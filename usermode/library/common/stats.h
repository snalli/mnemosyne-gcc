/**
 * \file stats.h 
 *
 * \brief Statistics collector interface and MACROs.
 *
 */

#ifndef _TXC_STATS_H
#define _TXC_STATS_H

#include <misc/generic_types.h>
#include <core/config.h>

/** The statistics collected by the collector. */
#define FOREACH_STAT_XCALL(ACTION)                                           \
  ACTION(x_close)                                                            \
  ACTION(x_create)                                                           \
  ACTION(x_dup)                                                              \
  ACTION(x_fdatasync)                                                        \
  ACTION(x_fsync)                                                            \
  ACTION(x_lseek)                                                            \
  ACTION(x_open)                                                             \
  ACTION(x_pipe)                                                             \
  ACTION(x_printf)                                                           \
  ACTION(x_read)                                                             \
  ACTION(x_read_pipe)                                                        \
  ACTION(x_recvmsg)                                                          \
  ACTION(x_rename)                                                           \
  ACTION(x_sendmsg)                                                          \
  ACTION(x_socket)                                                           \
  ACTION(x_unlink)                                                           \
  ACTION(x_write_ovr)                                                        \
  ACTION(x_write_ovr_ignore)                                                 \
  ACTION(x_write_pipe)                                                       \
  ACTION(x_write_seq)


#ifdef _TXC_STATS_BUILD

/** 
 * Declare under FOREACH_STAT and FOREACH_STATPROBE the statistics you want 
 * to generate probes for and under FOREACH_VOIDSTATPROBE the statistics you
 * do NOT want to generate probes for (i.e. generate void probes).
 *
 * Important: FOREACH_STAT? and FOREACH_VOIDSTATPROBE must be mutually 
 * exclusive. 
 */
# define FOREACH_STAT(ACTION)                                                \
    FOREACH_STAT_XCALL (ACTION)

# define FOREACH_STATPROBE(ACTION)                                           \
    ACTION(XCALL)

# define FOREACH_VOIDSTATPROBE(ACTION)                                           


#else /* !_TXC_STATS_BUILD */

# define FOREACH_STAT(ACTION)          /* do nothing */
# define FOREACH_STATPROBE(ACTION)     /* do nothing */
# define FOREACH_VOIDSTATPROBE(ACTION) /* do nothing */

#endif /* !_TXC_STATS_BUILD */

typedef enum {
#define STATENTRY(name) txc_stats_##name##_stat,
	FOREACH_STAT (STATENTRY)
#undef STATENTRY	
	txc_stats_numofstats
} txc_stats_statentry_t; 

typedef unsigned int txc_stats_statcounter_t;
typedef struct txc_stats_threadstat_s txc_stats_threadstat_t;
typedef struct txc_statsmgr_s txc_statsmgr_t;
typedef struct txc_stats_txstat_s txc_stats_txstat_t;
typedef struct txc_stats_stat_s txc_stats_stat_t;

/* 
 * Break the abstraction and make txc_stats_stat_s and txc_stats_txstat_s
 * visible to others to enable fast inline statistics processing. 
 */


#include <core/tx.h>
#include <core/txdesc.h>

/** Statistic */
struct txc_stats_stat_s
{
	txc_stats_statcounter_t max;   /**< Maximum number of the statistic across all transactions of an atomic block (static transaction). */
	txc_stats_statcounter_t min;   /**< Minimum number of the statistic across all transactions of an atomic block (static transaction). */
	txc_stats_statcounter_t total; /**< Total number of the statistic across all transactions of an atomic block (static transaction). */
}; 


/**Static transaction's (atomic block) statistics */
struct txc_stats_txstat_s {
	const char               *srcloc_str; /**< Stringified source location. */
	txc_tx_srcloc_t          srcloc;      /**< Source location. */
	txc_stats_statcounter_t  count;       /**< Number of dynamic transactions. */
	txc_stats_stat_t         total_stats[txc_stats_numofstats]; /**< Statistics collection. */
}; 


/** \brief Writes the value of a statistic. */
inline
void
txc_stats_txstat_set(txc_tx_t *txd, 
                     txc_stats_statentry_t entry,
                     txc_stats_statcounter_t val)
{
	txd->txstat->total_stats[entry].total = val;	
}


/** \brief Reads the value of a statistic. */
inline
txc_stats_statcounter_t
txc_stats_txstat_get(txc_tx_t *txd, 
                     txc_stats_statentry_t entry)
{
	return txd->txstat->total_stats[entry].total;
}


#define GENERATE_STATPROBES(stat_provider)                                   \
inline                                                                       \
void                                                                         \
txc_stats_txstat_increment_##stat_provider(txc_tx_t *txd,                    \
                                           txc_stats_statentry_t entry,      \
                                           txc_stats_statcounter_t val)      \
{                                                                            \
    if (txc_runtime_settings.statistics == TXC_BOOL_TRUE) {                  \
        txc_stats_txstat_set(txd,                                            \
                             entry,                                          \
                             txc_stats_txstat_get(txd, entry) + val);        \
    }							                                             \
}										                                     \
                                                                             \
inline                                                                       \
void                                                                         \
txc_stats_txstat_decrement_##stat_provider(txc_tx_t *txd,                    \
                                           txc_stats_statentry_t entry,      \
                                           txc_stats_statcounter_t val)      \
{                                                                            \
	if (txc_runtime_settings.statistics == TXC_BOOL_TRUE) {                  \
        txc_stats_txstat_set(txd,                                            \
                             entry,                                          \
                             txc_stats_txstat_get(txd, entry) - val);        \
    }							                                             \
}										                                     \



#define GENERATE_VOIDSTATPROBES(stat_provider)                               \
inline                                                                       \
void                                                                         \
txc_stats_txstat_increment_##stat_provider(txc_tx_t *txd,                    \
                                           txc_stats_statentry_t entry,      \
                                           txc_stats_statcounter_t val)      \
{                                                                            \
	return ;                                                                 \
}										                                     \
                                                                             \
inline                                                                       \
void                                                                         \
txc_stats_txstat_increment_##stat_provider(txc_tx_t *txd,                    \
                                           txc_stats_statentry_t entry,      \
                                           txc_stats_statcounter_t val)      \
{                                                                            \
	return ;                                                                 \
}										                                     \


FOREACH_STATPROBE (GENERATE_STATPROBES)
FOREACH_VOIDSTATPROBE (GENERATE_VOIDSTATPROBES)


#define txc_stats_txstat_increment(txd, stat_provider, stat, val)            \
  txc_stats_txstat_increment_##stat_provider(txd,                            \
                                             txc_stats_##stat##_stat,        \
                                             val);

#define txc_stats_txstat_decrement(txd, stat_provider, stat, val)            \
  txc_stats_txstat_decrement_##stat_provider(txd,                            \
                                             txc_stats_##stat##_stat,        \
                                             val);


txc_result_t txc_statsmgr_create(txc_statsmgr_t **statsmgrp);
txc_result_t txc_statsmgr_destroy(txc_statsmgr_t **statsmgrp);
txc_result_t txc_stats_threadstat_create(txc_statsmgr_t *statsmgr, txc_stats_threadstat_t **threadstatp, txc_tx_t *txd);
txc_result_t txc_stats_txstat_create(txc_stats_txstat_t **txstatp);
txc_result_t txc_stats_txstat_destroy(txc_stats_txstat_t **txstatp);
txc_result_t txc_stats_txstat_init(txc_stats_txstat_t *txstat, const char *srcloc_str, txc_tx_srcloc_t *srcloc);
void txc_stats_register_statsmgr_commit_action(txc_tx_t *txd);
void txc_stats_register_statsmgr_undo_action(txc_tx_t *txd);
void txc_stats_transaction_postbegin(txc_tx_t *txd);
void txc_stats_print(txc_statsmgr_t *statsmgr);

#endif /* _TXC_STATS_H */
