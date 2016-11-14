/**
 * \file mtm_i.h
 *
 * \brief Definitions used internally by the STM implementation
 *
 */

/*
 * Source code is partially derived from TinySTM (license is attached)
 *
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * Copyright (c) 2007-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MTM_I_H_819AKI 
#define _MTM_I_H_819AKI 

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>

#define ITM_NORETURN	__attribute__((noreturn))

#include <xmmintrin.h>

typedef uint8_t              _ITM_TYPE_U1;
typedef uint16_t             _ITM_TYPE_U2;
typedef uint32_t             _ITM_TYPE_U4;
typedef uint64_t             _ITM_TYPE_U8;
typedef float                _ITM_TYPE_F;
typedef double               _ITM_TYPE_D;
typedef long double          _ITM_TYPE_E;
typedef __m64                _ITM_TYPE_M64;
typedef __m128               _ITM_TYPE_M128;
typedef float _Complex       _ITM_TYPE_CF;
typedef double _Complex      _ITM_TYPE_CD;
typedef long double _Complex _ITM_TYPE_CE;

typedef struct mtm_tx_s mtm_tx_t;

#include <itm.h>

# ifndef __cplusplus
#  define FOR_ALL_TYPES(ACTION,name)    \
ACTION (name,uint8_t,U1)                \
ACTION (name,uint16_t,U2)               \
ACTION (name,uint32_t,U4)               \
ACTION (name,uint64_t,U8)               \
ACTION (name,float,F)                   \
ACTION (name,double,D)                  \
ACTION (name,long double,E)             \
ACTION (name,__m64,M64)                 \
ACTION (name,__m128,M128)               \
ACTION (name,float _Complex,CF)         \
ACTION (name,double _Complex,CD)        \
ACTION (name,long double _Complex,CE)
# else
#  define FOR_ALL_TYPES(ACTION,name)    \
ACTION (name,uint8_t,U1)                \
ACTION (name,uint16_t,U2)               \
ACTION (name,uint32_t,U4)               \
ACTION (name,uint64_t,U8)               \
ACTION (name,float,F)                   \
ACTION (name,double,D)                  \
ACTION (name,long double,E)             \
ACTION (name,__m64,M64)                 \
ACTION (name,__m128,M128)
# endif

#include <result.h>


/* Persistent log type */
#define TMLOG_TYPE_BASE    0
#define TMLOG_TYPE_TORNBIT 1

#if TMLOG_TYPE == TMLOG_TYPE_BASE
# define M_TMLOG_WRITE          m_tmlog_base_write
# define M_TMLOG_TRUNCATE_SYNC  m_tmlog_base_truncate_sync
# define M_TMLOG_BEGIN          m_tmlog_base_begin
# define M_TMLOG_COMMIT         m_tmlog_base_commit
# define M_TMLOG_ABORT          m_tmlog_base_abort
# define M_TMLOG_T              m_tmlog_base_t
# define M_TMLOG_LF_TYPE        LF_TYPE_TM_BASE
# define M_TMLOG_OPS            tmlog_base_ops
#elif TMLOG_TYPE == TMLOG_TYPE_TORNBIT
# define M_TMLOG_WRITE          m_tmlog_tornbit_write
# define M_TMLOG_TRUNCATE_SYNC  m_tmlog_tornbit_truncate_sync
# define M_TMLOG_BEGIN          m_tmlog_tornbit_begin
# define M_TMLOG_COMMIT         m_tmlog_tornbit_commit
# define M_TMLOG_ABORT          m_tmlog_tornbit_abort
# define M_TMLOG_T              m_tmlog_tornbit_t
# define M_TMLOG_LF_TYPE        LF_TYPE_TM_TORNBIT
# define M_TMLOG_OPS            tmlog_tornbit_ops
#else
# error "Unknown persistent log type."
#endif

/*
 * The library does not require to pass the current transaction as a
 * parameter to the functions (the current transaction is stored in a
 * thread-local variable).  One can, however, compile the library with
 * explicit transaction parameters.  This is useful, for instance, for
 * performance on architectures that do not support TLS or for easier
 * compiler integration.
 */
#define EXPLICIT_TX_PARAMETER

# ifdef EXPLICIT_TX_PARAMETER
#  define TXTYPE                        mtm_tx_t *
#  define TXPARAM                       mtm_tx_t *tx
#  define TXPARAMS                      mtm_tx_t *tx,
#  define TXARG                         (mtm_tx_t *)tx
#  define TXARGS                        (mtm_tx_t *)tx,
struct mtm_tx *mtm_current_tx();
# else /* ! EXPLICIT_TX_PARAMETER */
#  define TXTYPE                        void
#  define TXPARAM                       /* Nothing */
#  define TXPARAMS                      /* Nothing */
#  define TXARG                         /* Nothing */
#  define TXARGS                        /* Nothing */
#endif /* ! EXPLICIT_TX_PARAMETER */

#ifdef EXPLICIT_TX_PARAMETER
# define TX_RETURN                      return tx
# define TX_GET                         /* Nothing */
#else /* ! EXPLICIT_TX_PARAMETER */
# define TX_RETURN                      return
# define TX_GET                         mtm_tx_t *tx = mtm_get_tx()
#endif /* ! EXPLICIT_TX_PARAMETER */

#define WRITE_BACK_ETL                  0
#define WRITE_THROUGH                   1

#define CM_SUICIDE                      0
#define CM_DELAY                        1
#define CM_BACKOFF                      2
#define CM_PRIORITY                     3

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unwind.h>
#include <pthread.h>

#include <atomic.h>

#include <pcm.h>

#include "mode/dtable.h"

#if defined(CONFLICT_TRACKING) && ! defined(EPOCH_GC)
# error "CONFLICT_TRACKING requires EPOCH_GC"
#endif /* defined(CONFLICT_TRACKING) && ! defined(EPOCH_GC) */

#if defined(READ_LOCKED_DATA) && ! defined(EPOCH_GC)
# error "READ_LOCKED_DATA requires EPOCH_GC"
#endif /* defined(READ_LOCKED_DATA) && ! defined(EPOCH_GC) */

#define TLS

# define MTM_DEBUG_PRINT(...)
//# define MTM_DEBUG_PRINT(...)               printf(__VA_ARGS__); fflush(NULL)

#ifdef DEBUG
/* Note: stdio is thread-safe */
# define IO_FLUSH                       fflush(NULL)
# define PRINT_DEBUG(...)               printf(__VA_ARGS__); fflush(NULL)
#else /* ! DEBUG */
# define IO_FLUSH
# define PRINT_DEBUG(...)
#endif /* ! DEBUG */

#ifdef DEBUG2
# define PRINT_DEBUG2(...)              PRINT_DEBUG(__VA_ARGS__)
#else /* ! DEBUG2 */
# define PRINT_DEBUG2(...)
#endif /* ! DEBUG2 */

#ifndef LOCK_SHIFT_EXTRA
# define LOCK_SHIFT_EXTRA               2                   /* 2 extra shift */
#endif /* LOCK_SHIFT_EXTRA */

#if CM == CM_PRIORITY
# define VR_THRESHOLD                   "VR_THRESHOLD"
# define CM_THRESHOLD                   "CM_THRESHOLD"
#endif

extern int vr_threshold;
extern int cm_threshold;



#define STR2(str1, str2)                str1##str2
#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define COMPILE_TIME_ASSERT(pred)       switch (0) { case 0: case pred: ; }

#define UNUSED		__attribute__((unused))

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility push(hidden)
#endif

#include "mode/mode.h"
#include "sysdeps/x86/target.h"
#include "rwlock.h"
#include "useraction.h"
#include "locks.h"
#include "local.h"
#include "stats.h"

/**
 * Size of a word (accessible atomically) on the target architecture.
 * The library supports 32-bit and 64-bit architectures.
 */
typedef uintptr_t mtm_word_t;

enum {                                  /* Transaction status */
  TX_IDLE = 0,
  TX_ACTIVE = 1,
  TX_COMMITTED = 2,
  TX_ABORTED = 3,
  TX_IRREVOCABLE = 4,
  TX_SERIAL = 8,
};


/* 
 * These values are given to mtm_restart_transaction and indicate the
 * reason for the restart.  The reason is used to decide what STM 
 * implementation should be used during the next iteration.  
 */
typedef enum mtm_restart_reason
{
	RESTART_REALLOCATE,
	RESTART_LOCKED_READ,
	RESTART_LOCKED_WRITE,
	RESTART_VALIDATE_READ,
	RESTART_VALIDATE_WRITE,
	RESTART_VALIDATE_COMMIT,
	RESTART_NOT_READONLY,
	RESTART_USER_RETRY,
	NUM_RESTARTS
} mtm_restart_reason;


/* This type is private to local.c.  */
struct mtm_local_undo;

/* Transaction descriptor */
struct mtm_tx_s {
	uintptr_t              dummy1;           /* ICC expects to find the dtable pointer at offset 2*WORD_SIZE. */
	uintptr_t              dummy2;
	mtm_dtable_t           *dtable;          /* The dispatch table for the active transaction mode. */

	jmp_buf                *tmp_jb_ptr;      /* Pointer to the temporary register checkpoint. This simplifies the assembly checkpoint code. */
	jmp_buf                tmp_jb;           /* Temporary register checkpoint; this is where the assebly register checkpoint code will save the registers.  */
	jmp_buf                jb;               /* Register checkpoint of this transaction. Keeping this separate from tmp_jb allows a nested transaction to transparently execute the assembly checkpoint code without destroying the active register checkpoint.  */

	mtm_mode_data_t        *modedata[MTM_NUM_MODES];
	mtm_mode_t             mode;
	mtm_word_t             status;           /* Transaction status (not read by other threads). */

	uint32_t               prop;             /* The _ITM_codeProperties of this transaction as given by the compiler.  */
	int                    nesting;          /* Nesting level. */
	int                    can_extend;       /* Can this transaction be extended? */
	_ITM_transactionId     id;               /* Instance number of the transaction */
	int                    thread_num;
#ifdef CONFLICT_TRACKING
	pthread_t              thread_id;        /* Thread identifier (immutable) */
#endif /* CONFLICT_TRACKING */
#if CM == CM_DELAY || CM == CM_PRIORITY
	volatile mtm_word_t    *c_lock;          /* Pointer to contented lock (cause of abort). */
#endif /* CM == CM_DELAY || CM == CM_PRIORITY */
#if CM == CM_BACKOFF
	unsigned long          backoff;          /* Maximum backoff duration. */
	unsigned long          seed;             /* RNG seed. */
#endif /* CM == CM_BACKOFF */
#if CM == CM_PRIORITY
	int                    priority;         /* Transaction priority */
	int                    visible_reads;    /* Should we use visible reads? */
#endif /* CM == CM_PRIORITY */
	unsigned long          retries;          /* Number of consecutive aborts (retries) */

	uintptr_t              stack_base;       /* Stack base address */
	uintptr_t              stack_size;       /* Stack size */
	mtm_local_undo_t       local_undo;       /* Data used by local.c for the local memory undo log.  */
	mtm_word_t             *wb_table;        /* Private write-back table for use when isolation is off. */
	pcm_storeset_t         *pcm_storeset;    /* PCM emulation bookkeeping structure */
	m_stats_threadstat_t   *threadstat;      /* Thread statistics */
	m_stats_statset_t      *statset;         /* Per transaction instance statistics */
	mtm_user_action_list_t *commit_action_list;
	mtm_user_action_list_t *undo_action_list;
};


/*
 * Transaction nesting is supported in a minimalist way (flat nesting):
 * - When a transaction is started in the context of another
 *   transaction, we simply increment a nesting counter but do not
 *   actually start a new transaction.
 * - The environment to be used for setjmp/longjmp is only returned when
 *   no transaction is active so that it is not overwritten by nested
 *   transactions. This allows for composability as the caller does not
 *   need to know whether it executes inside another transaction.
 * - The commit of a nested transaction simply decrements the nesting
 *   counter. Only the commit of the top-level transaction will actually
 *   carry through updates to shared memory.
 * - An abort of a nested transaction will rollback the top-level
 *   transaction and reset the nesting counter. The call to longjmp will
 *   restart execution before the top-level transaction.
 * Using nested transactions without setjmp/longjmp is not recommended
 * as one would need to explicitly jump back outside of the top-level
 * transaction upon abort of a nested transaction. This breaks
 * composability.
 */

/*
 * Reading from the previous version of locked addresses is implemented
 * by peeking into the write set of the transaction that owns the
 * lock. Each transaction has a unique identifier, updated even upon
 * retry. A special "commit" bit of this identifier is set upon commit,
 * right before writing the values from the redo log to shared memory. A
 * transaction can read a locked address if the identifier of the owner
 * does not change between before and after reading the value and
 * version, and it does not have the commit bit set.
 */

/*!
 * The set of locks and write-set pointers (together!).
 * 
 * A lock is a unsigned int of the size of a pointer (mtm_word_t).
 * The LSB is the lock bit. If it is set, this means:
 * - At least some covered memory addresses is being written.
 * - Write-back (ETL): all bits of the lock apart from the lock bit form
 *   a pointer that points to the write log entry holding the new
 *   value. Multiple values covered by the same log entry and orginized
 *   in a linked list in the write log.
 * - Write-through and write-back (CTL): all bits of the lock apart from
 *   the lock bit form a pointer that points to the transaction
 *   descriptor containing the write-set.
 * If the lock bit is not set, then:
 * - All covered memory addresses contain consistent values.
 * - Write-back (ETL and CTL): all bits of the lock besides the lock bit
 *   contain a version number (timestamp).
 * - Write-through: all bits of the lock besides the lock bit contain a
 *   version number.
 *   - The high order bits contain the commit time.
 *   - The low order bits contain an incarnation number (incremented
 *     upon abort while writing the covered memory addresses).
 * When using the PRIORITY contention manager, the format of locks is
 * slightly different. It is documented elsewhere.
 *
 * \see locks.h
 */
extern volatile mtm_word_t locks[];

#ifdef CLOCK_IN_CACHE_LINE
extern volatile mtm_word_t gclock[];
# define CLOCK                          (gclock[512 / sizeof(mtm_word_t)])
#else /* ! CLOCK_IN_CACHE_LINE */
extern volatile mtm_word_t gclock;
# define CLOCK                          (gclock)
#endif /* ! CLOCK_IN_CACHE_LINE */

#ifdef _M_STATS_BUILD	
extern m_statsmgr_t *mtm_statsmgr;
#endif

/* 
 * The lock that provides access to serial mode.  Non-serialized transactions
 * acquire read locks; the serialized transaction aquires a write lock.
 */
extern mtm_rwlock_t mtm_serial_lock;

extern uint32_t mtm_begin_transaction(uint32_t, const mtm_jmpbuf_t *);
extern uint32_t mtm_longjmp (const mtm_jmpbuf_t *, uint32_t)
	ITM_NORETURN;

extern void mtm_commit_local (TXPARAM);
extern void mtm_rollback_local (TXPARAM);
extern void mtm_LB (mtm_tx_t *tx, const void *, size_t) _ITM_CALL_CONVENTION;

extern void mtm_serialmode (bool, bool);
extern void mtm_restart_transaction (mtm_restart_reason) ITM_NORETURN;

extern void mtm_alloc_record_allocation (void *, size_t, void (*)(void *));
extern void mtm_alloc_forget_allocation (void *, void (*)(void *));
extern size_t mtm_alloc_get_allocation_size (void *);
extern void mtm_alloc_commit_allocations (bool);

extern void mtm_revert_cpp_exceptions (void);

//extern mtm_cacheline_page *mtm_page_alloc (void);
//extern void mtm_page_release (mtm_cacheline_page *, mtm_cacheline_page *);

//extern mtm_cacheline *mtm_null_read_lock (mtm_tx_t *td, uintptr_t);
//extern mtm_cacheline_mask_pair mtm_null_write_lock (mtm_tx_t *td, uintptr_t);
extern void *mtm_null_read_lock (mtm_tx_t *td, uintptr_t);
extern void *mtm_null_write_lock (mtm_tx_t *td, uintptr_t);


/* ################################################################### *
 * CLOCK
 * ################################################################### */

#define GET_CLOCK                       (ATOMIC_LOAD_ACQ(&CLOCK))
#define FETCH_INC_CLOCK                 (ATOMIC_FETCH_INC_FULL(&CLOCK))


/* ################################################################### *
 * STATIC
 * ################################################################### */

/* Don't access this variable directly; use the functions below.  */
#ifdef TLS
extern __thread mtm_tx_t *_mtm_thread_tx;
#else /* !TLS */
extern pthread_key_t _mtm_thread_tx;
#endif /* !TLS */

/*
 * Returns the transaction descriptor for the CURRENT thread.
 */
static inline mtm_tx_t *mtm_get_tx()
{
#ifdef TLS
  return _mtm_thread_tx;
#else /* ! TLS */
  return (mtm_tx_t *)pthread_getspecific(_mtm_thread_tx);
#endif /* ! TLS */
}

#ifdef LOCK_IDX_SWAP
/*
 * Compute index in lock table (swap bytes to avoid consecutive addresses to have neighboring locks).
 */
static inline unsigned int lock_idx_swap(unsigned int idx) {
  return (idx & ~(unsigned int)0xFFFF) | ((idx & 0x00FF) << 8) | ((idx & 0xFF00) >> 8);
}
#endif /* LOCK_IDX_SWAP */

#ifdef ROLLOVER_CLOCK
/*
 * We use a simple approach for clock roll-over:
 * - We maintain the count of (active) transactions using a counter
 *   protected by a mutex. This approach is not very efficient but the
 *   cost is quickly amortized because we only modify the counter when
 *   creating and deleting a transaction descriptor, which typically
 *   happens much less often than starting and committing a transaction.
 * - We detect overflows when reading the clock or when incrementing it.
 *   Upon overflow, we wait until all threads have blocked on a barrier.
 * - Threads can block on the barrier upon overflow when they (1) start
 *   a transaction, or (2) delete a transaction. This means that threads
 *   must ensure that they properly delete their transaction descriptor
 *   before performing any blocking operation outside of a transaction
 *   in order to guarantee liveness (our model prohibits blocking
 *   inside a transaction).
 */

pthread_mutex_t tx_count_mutex;
pthread_cond_t tx_reset;
int tx_count;
int tx_overflow;

/*
 * Enter new transactional thread.
 */
static inline void mtm_rollover_enter(mtm_tx_t *tx)
{
  PRINT_DEBUG("==> mtm_rollover_enter(%p)\n", tx);

  pthread_mutex_lock(&tx_count_mutex);
  while (tx_overflow != 0)
    pthread_cond_wait(&tx_reset, &tx_count_mutex);
  /* One more (active) transaction */
  tx_count++;
  pthread_mutex_unlock(&tx_count_mutex);
}

/*
 * Exit transactional thread.
 */
static inline void mtm_rollover_exit(mtm_tx_t *tx)
{
  PRINT_DEBUG("==> mtm_rollover_exit(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

  pthread_mutex_lock(&tx_count_mutex);
  /* One less (active) transaction */
  tx_count--;
  assert(tx_count >= 0);
  /* Are all transactions stopped? */
  if (tx_overflow != 0 && tx_count == 0) {
    /* Yes: reset clock */
    PM_MEMSET((void *)locks, 0, LOCK_ARRAY_SIZE * sizeof(mtm_word_t));
    CLOCK = 0;
    tx_overflow = 0;
# ifdef EPOCH_GC
    /* Reset GC */
    gc_reset();
# endif /* EPOCH_GC */
    /* Wake up all thread */
    pthread_cond_broadcast(&tx_reset);
  }
  pthread_mutex_unlock(&tx_count_mutex);
}

/*!
 * Cleans all transactions in the current thread, wipes away all write sets, and
 * sets the version clock back to zero.
 *
 * \param tx the active transaction in this thread. This has no bearing on the
 *  actions performed unless debugging output is enabled, in which case the
 *  transaction here is described on the console.
 */
static inline void mtm_overflow(mtm_tx_t *tx)
{
  PRINT_DEBUG("==> mtm_overflow(%p[%lu-%lu])\n", tx, (unsigned long)tx->start, (unsigned long)tx->end);

  pthread_mutex_lock(&tx_count_mutex);
  /* Set overflow flag (might already be set) */
  tx_overflow = 1;
  /* One less (active) transaction */
  tx_count--;
  assert(tx_count >= 0);
  /* Are all transactions stopped? */
  if (tx_count == 0) {
    /* Yes: reset clock */
    PM_MEMSET((void *)locks, 0, LOCK_ARRAY_SIZE * sizeof(mtm_word_t));
    CLOCK = 0;
    tx_overflow = 0;
# ifdef EPOCH_GC
    /* Reset GC */
    gc_reset();
# endif /* EPOCH_GC */
    /* Wake up all thread */
    pthread_cond_broadcast(&tx_reset);
  } else {
    /* No: wait for other transactions to stop */
    pthread_cond_wait(&tx_reset, &tx_count_mutex);
  }
  /* One more (active) transaction */
  tx_count++;
  pthread_mutex_unlock(&tx_count_mutex);
}
#endif /* ROLLOVER_CLOCK */

/*
 * Get curent value of global clock.
 */
static inline mtm_word_t mtm_get_clock()
{
  return GET_CLOCK;
}

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility pop
#endif

#endif /* _MTM_I_H_819AKI */
