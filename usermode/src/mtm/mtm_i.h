/* Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* The following are internal implementation functions and definitions.
   To distinguish them from those defined by the Intel ABI, they all
   begin with mtm.  */

#ifndef LIBITM_I_H
#define LIBITM_I_H 1

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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

#include "../inc/itm.h"

//typedef struct mtm_thread_s mtm_thread_t;
typedef struct mtm_transaction_s mtm_transaction_t;

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unwind.h>

#include "vtable.h"

#define UNUSED		__attribute__((unused))

/* Control how mtm_copy_cacheline_mask operates.  If set, we use byte masking
   to update D, which *does* write to bytes not affected by the mask.  It's
   unclear if this optimization is correct.  */
#define ALLOW_UNMASKED_STORES 0

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility push(hidden)
#endif

#include "target.h"
#include "rwlock.h"
#include "aatree.h"


/* A mtm_cacheline_mask stores a modified bit for every modified byte
   in the cacheline with which it is associated.  */
#if CACHELINE_SIZE == 8
typedef uint8_t mtm_cacheline_mask;
#elif CACHELINE_SIZE == 16
typedef uint16_t mtm_cacheline_mask;
#elif CACHELINE_SIZE == 32
typedef uint32_t mtm_cacheline_mask;
#elif CACHELINE_SIZE == 64
typedef uint64_t mtm_cacheline_mask;
#else
#error "Unsupported cacheline size"
#endif

typedef unsigned int mtm_word __attribute__((mode (word)));

/* A cacheline.  The smallest unit with which locks are associated.  */
typedef union mtm_cacheline
{
	/* Byte access to the cacheline.  */
	unsigned char b[CACHELINE_SIZE] __attribute__((aligned(CACHELINE_SIZE)));

	/* Larger sized access to the cacheline.  */
	uint16_t u16[CACHELINE_SIZE / sizeof(uint16_t)];
	uint32_t u32[CACHELINE_SIZE / sizeof(uint32_t)];
	uint64_t u64[CACHELINE_SIZE / sizeof(uint64_t)];
	mtm_word w[CACHELINE_SIZE / sizeof(mtm_word)];

#if defined(__i386__) || defined(__x86_64__)
	/* ??? The definitions of mtm_cacheline_copy{,_mask} require all three
	 * of these types depending on the implementation, making it difficult
	 * to hide these inside the target header file.  */
# ifdef __SSE__
	__m128 m128[CACHELINE_SIZE / sizeof(__m128)];
# endif
# ifdef __SSE2__
	__m128i m128i[CACHELINE_SIZE / sizeof(__m128i)];
# endif
# ifdef __AVX__
	__m256 m256[CACHELINE_SIZE / sizeof(__m256)];
# endif
#endif
} mtm_cacheline;

/* A "page" worth of saved cachelines plus modification masks.  This
   arrangement is intended to minimize the overhead of alignment.  The
   PAGE_SIZE defined by the target must be a constant for this to work,
   which means that this definition may not be the same as the real
   system page size.  */

#define CACHELINES_PER_PAGE \
	((PAGE_SIZE - sizeof(void *)) \
	 / (CACHELINE_SIZE + sizeof(mtm_cacheline_mask)))

typedef struct mtm_cacheline_page
{
	mtm_cacheline lines[CACHELINES_PER_PAGE] __attribute__((aligned(PAGE_SIZE)));
	mtm_cacheline_mask masks[CACHELINES_PER_PAGE];
	struct mtm_cacheline_page *prev;
} mtm_cacheline_page;

static inline mtm_cacheline_page *
mtm_page_for_line (mtm_cacheline *c)
{
	return (mtm_cacheline_page *)((uintptr_t)c & -PAGE_SIZE);
}

static inline mtm_cacheline_mask *
mtm_mask_for_line (mtm_cacheline *c)
{
	mtm_cacheline_page *p = mtm_page_for_line (c);
	size_t index = c - &p->lines[0];
	return &p->masks[index];
}

/* A read lock function locks a cacheline.  PTR must be cacheline aligned.
 * The return value is the cacheline address (equal to PTR for a write-through
 * implementation, and something else for a write-back implementation).  */
typedef mtm_cacheline *(*mtm_read_lock_fn)(mtm_thread_t *td, uintptr_t cacheline);


/* A write lock function locks a cacheline.  PTR must be cacheline aligned.
 * The return value is a pair of the cacheline address and a mask that must
 * be updated with the bytes that are subsequently modified.  We hope that
 * the target implements small structure return efficiently so that this
 * comes back in a pair of registers.  If not, we're not really worse off
 * than returning the second value via a second argument to the function.  */
typedef struct mtm_cacheline_mask_pair
{
	mtm_cacheline *line;
	mtm_cacheline_mask *mask;
} mtm_cacheline_mask_pair;

typedef mtm_cacheline_mask_pair (*mtm_write_lock_fn)(mtm_thread_t *td, uintptr_t cacheline);

/* A versioned write lock on a cacheline.  This must be wide enough to 
   store a pointer, and preferably wide enough to avoid overflowing the
   version counter.  Thus we use a "word", which should be 64-bits on
   64-bit systems even when their pointer size is forced smaller.  */
typedef mtm_word mtm_stmlock;

/* This has to be the same size as mtm_stmlock, we just use this name
   for documentation purposes.  */
typedef mtm_word mtm_version;

/* The maximum value a version number can have.  This is a consequence
   of having the low bit of mtm_stmlock reserved for the owned bit.  */
#define mtm_VERSION_MAX		(~(mtm_version)0 >> 1)

/* A value that may be used to indicate "uninitialized" for a version.  */
#define mtm_VERSION_INVALID	(~(mtm_version)0)

/* This bit is set when the write lock is held.  When set, the balance of
   the bits in the lock is a pointer that references STM backend specific
   data; it is up to the STM backend to determine if this thread holds the
   lock.  If this bit is clear, the balance of the bits are the last 
   version number committed to the cacheline.  */
static inline bool
mtm_stmlock_owned_p (mtm_stmlock lock)
{
	return lock & 1;
}

static inline mtm_stmlock
mtm_stmlock_set_owned (void *data)
{
	return (mtm_stmlock)(uintptr_t)data | 1;
}

static inline void *
mtm_stmlock_get_addr (mtm_stmlock lock)
{
	return (void *)((uintptr_t)lock & ~(uintptr_t)1);
}

static inline mtm_version
mtm_stmlock_get_version (mtm_stmlock lock)
{
	return lock >> 1;
}

static inline mtm_stmlock
mtm_stmlock_set_version (mtm_version ver)
{
	return ver << 1;
}

/* We use a fixed set of locks for all memory, hashed into the
   following table.  */
#define LOCK_ARRAY_SIZE  (1024 * 1024)
extern mtm_stmlock mtm_stmlock_array[LOCK_ARRAY_SIZE];

static inline mtm_stmlock *
mtm_get_stmlock (uintptr_t addr)
{
	size_t idx = (addr / CACHELINE_SIZE) % LOCK_ARRAY_SIZE;
	return mtm_stmlock_array + idx;
}

/* The current global version number.  */
extern mtm_version mtm_clock;

/* These values define a mask used in mtm_transaction_t.state.  */
#define STATE_READONLY		0x0001
#define STATE_SERIAL		0x0002
#define STATE_IRREVOCABLE	0x0004
#define STATE_ABORTING		0x0008

/* These values are given to mtm_restart_transaction and indicate the
   reason for the restart.  The reason is used to decide what STM 
   implementation should be used during the next iteration.  */
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

/* This type is private to useraction.c.  */
struct mtm_user_action;

/* This type is private to the STM implementation.  */
struct mtm_method;


/* All data relevant to a single transaction.  */
struct mtm_transaction_s
{
	/* The jump buffer by which mtm_longjmp restarts the transaction.
	   This field *must* be at the beginning of the transaction.  */
	mtm_jmpbuf_t jb;

	/* Data used by local.c for the local memory undo log.  */
	struct mtm_local_undo **local_undo;
	size_t n_local_undo;
	size_t size_local_undo;

	/* Data used by alloc.c for the malloc/free undo log.  */
	aa_tree alloc_actions;

	/* Data used by useraction.c for the user defined undo log.  */
	struct mtm_user_action *commit_actions;
	struct mtm_user_action *undo_actions;

	/* Data used by the STM implementation.  */
	struct mtm_method *m;

	/* A pointer to the "outer" transaction.  */
	mtm_transaction_t *prev;

	/* A numerical identifier for this transaction.  */
	_ITM_transactionId id;

	/* The _ITM_codeProperties of this transaction as given by the compiler.  */
	uint32_t prop;

	/* The nesting depth of this transaction.  */
	uint32_t nesting;

	/* A mask of bits indicating the current status of the transaction.  */
	uint32_t state;

	/* Data used by eh_cpp.c for managing exceptions within the transaction.  */
	uint32_t cxa_catch_count;
	void *cxa_unthrown;
	void *eh_in_flight;

	/* Data used by retry.c for deciding what STM implementation should
	   be used for the next iteration of the transaction.  */
	uint32_t restart_reason[NUM_RESTARTS];
	uint32_t restart_total;
};

/* The maximum number of free mtm_transaction_t structs to be kept.
   This number must be greater than 1 in order for transaction abort
   to be handled properly.  */
#define MAX_FREE_TX	8


/* All thread-local data required by the entire library.  */
struct mtm_thread_s
{
	uintptr_t dummy1;
	uintptr_t dummy2;
	/* The dispatch table for the STM implementation currently in use.  */
	txninterface_t *vtable;
	/* The currently active transaction. */
	mtm_transaction_t *tx;

	mtm_jmpbuf_t *tmp_jb_ptr;
	mtm_jmpbuf_t tmp_jb;

	/* A queue of free mtm_transaction_t structs.  */
	mtm_transaction_t *free_tx[MAX_FREE_TX];
	unsigned free_tx_idx, free_tx_count;

	/* The value returned by _ITM_getThreadnum to identify this thread.  */
	/* ??? At present, this is densely allocated beginning with 1 and
	   we don't bother filling in this value until it is requested.
	   Which means that the value returned is, as far as the user is
	   concerned, essentially arbitrary.  We wouldn't need this at all
	   if we knew that pthread_t is integral and fits into an int.  */
	/* ??? Consider using gettid on Linux w/ NPTL.  At least that would
	   be a value meaningful to the user.  */
	int thread_num;
};

/* Don't access this variable directly; use the functions below.  */
extern __thread mtm_thread_t *_mtm_thr;

#include "target_i.h"

static inline void setup_mtm_thr(mtm_thread_t *thr) { _mtm_thr = thr; }
static inline mtm_thread_t *mtm_thr(void) { return _mtm_thr; }
static inline mtm_transaction_t * mtm_tx(void) { return _mtm_thr->tx; }
static inline void set_mtm_tx(mtm_transaction_t *x) { _mtm_thr->tx = x; }


#ifndef HAVE_ARCH_MTM_CACHELINE_COPY
/* Copy S to D, with S and D both aligned no overlap.  */
static inline void
mtm_cacheline_copy (mtm_cacheline * __restrict d,
                    const mtm_cacheline * __restrict s)
{
	*d = *s;
}
#endif

/* Similarly, but only modify bytes with bits set in M.  */
extern void mtm_cacheline_copy_mask (mtm_cacheline * __restrict d,
                                     const mtm_cacheline * __restrict s,
                                     mtm_cacheline_mask m);

#ifndef HAVE_ARCH_MTM_CCM_WRITE_BARRIER
/* A write barrier to emit after mtm_copy_cacheline_mask.  */
static inline void
mtm_ccm_write_barrier (void)
{
  atomic_write_barrier ();
}
#endif

/* The lock that provides access to serial mode.  Non-serialized transactions
   acquire read locks; the serialized transaction aquires a write lock.  */
extern mtm_rwlock mtm_serial_lock;

/* An unscaled count of the number of times we should spin attempting to 
   acquire locks before we block the current thread and defer to the OS.
   This variable isn't used when the standard POSIX lock implementations
   are used.  */
extern uint64_t mtm_spin_count_var;

extern uint32_t mtm_begin_transaction(uint32_t, const mtm_jmpbuf_t *);
extern uint32_t mtm_longjmp (const mtm_jmpbuf_t *, uint32_t)
	ITM_NORETURN;

extern void mtm_commit_local (void);
extern void mtm_rollback_local (void);
extern void mtm_LB (mtm_thread_t *td, const void *, size_t) _ITM_CALL_CONVENTION;

extern void mtm_serialmode (bool, bool);
extern void mtm_decide_retry_strategy (mtm_restart_reason);
extern void mtm_restart_transaction (mtm_restart_reason) ITM_NORETURN;

extern void mtm_alloc_record_allocation (void *, size_t, void (*)(void *));
extern void mtm_alloc_forget_allocation (void *, void (*)(void *));
extern size_t mtm_alloc_get_allocation_size (void *);
extern void mtm_alloc_commit_allocations (bool);

extern void mtm_revert_cpp_exceptions (void);

extern mtm_cacheline_page *mtm_page_alloc (void);
extern void mtm_page_release (mtm_cacheline_page *, mtm_cacheline_page *);

extern mtm_cacheline *mtm_null_read_lock (mtm_thread_t *td, uintptr_t);
extern mtm_cacheline_mask_pair mtm_null_write_lock (mtm_thread_t *td, uintptr_t);

static inline mtm_version
mtm_get_clock (void)
{
	mtm_version r;

	__sync_synchronize ();
	r = mtm_clock;
	atomic_read_barrier ();

	return r;
}

static inline mtm_version
mtm_inc_clock (void)
{
	mtm_version r = __sync_add_and_fetch (&mtm_clock, 1);

	/* ??? Ought to handle wraparound for 32-bit.  */
	if (sizeof(r) < 8 && r > mtm_VERSION_MAX) {
		abort ();
	}

	return r;
}


#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility pop
#endif

#endif /* LIBITM_I_H */
