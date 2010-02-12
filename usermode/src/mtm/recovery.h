/*!
 * \file
 * Describes the data structures used for redo-recovery of In-commit Persistent Transactions
 * when the program is reincarnated.
 *
 * \note In-Commit Persistent Transactions needs a better name. It isn't even that informative.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef RECOVERY_H_AUCIP1SR
#define RECOVERY_H_AUCIP1SR

#include "mtm_i.h"

/*!
 * An address and the intended written value of that entry. Unlike other write-set
 * entry structures in this codebase, this structure is independent of the mode of
 * transaction. It serves to record a value written at an address in nonvolatile
 * memory at the word granularity.
 */
struct nonvolatile_write_set_entry_s;

/*! Defines a type for the nonvolatile_write_entry structure. */
typedef struct nonvolatile_write_set_entry_s nonvolatile_write_set_entry_t;

/*!
 * A set of write-set entries, intended to be kept in nonvolatile storage, that
 * reflects the write-set of a transaction in-commit.
 *
 * \todo Write-set blocks should be chainable,
 * if the size of a set exceeds the number of slots available within one block. This
 * allows a pool-like allocation strategy for write set blocks, which avoids
 * fragmentation within persistent memory (where allocations/frees are expensive and
 * often permanent).
 */
struct nonvolatile_write_set_block_s;

/*! Defines a type for the write-set block. */
typedef struct nonvolatile_write_set_block_s nonvolatile_write_set_block_t;

/*!
 * A full write-set for nonvolatile storage.
 * 
 * For the time being, we say that a write set and a single write-set block are
 * equivalent. This is a result of the restriction that our current blocks
 * cannot be extended to include more entries than those for which they have
 * statically allocated space.
 */
typedef struct nonvolatile_write_set_block_s nonvolatile_write_set_t;

/*!
 * Commits the given write set to memory, performing all write operations specified
 * in the given log. For this to be a recoverable commit, the log must already
 * have been made consistent.
 *
 * \param write_set This write set will be committed to memory.
 *
 * \see nonvolatile_write_set_make_persistent()
 */
void nonvolatile_write_set_commit(nonvolatile_write_set_t* write_set);

/*!
 * Flushes the given write set to stable storage. This is not the same as flushing
 * the values in the write set to their targets in persistent storage. Rather,
 * this is a guarantee that the log, which needs to be persistent to
 * guarantee recoverability in case of a crash during write, is in stable storage.
 *
 * This routine will not return until the log is completely flushed to
 * stable storage.
 *
 * \param write_set The write set written out (in log form) to memory. This
 *  cannot be NULL.
 */
void nonvolatile_write_set_make_persistent(nonvolatile_write_set_t* write_set);

/*!
 * Returns a usable write-set block, reserved for the requesting thread and
 * initialized with zero written entries.
 *
 * \todo This function currently returns NULL if all write-set blocks are
 *  in use. A better modus operandi might be either to allocate new
 *  blocks to chain with the old ones or to wait conditionally for a
 *  block to be freed with the end of another transaction.
 */
nonvolatile_write_set_t* nonvolatile_write_set_next_available();

//////////////////////////////////////////////////////////////////
// Here follow details on the structures declared hereinbefore. //
//////////////////////////////////////////////////////////////////

struct nonvolatile_write_set_entry_s
{
	/*! The address which is written. This address should always be word-aligned. */
	volatile mtm_word_t* address;

	mtm_word_t value;   /*!< The value which is written at address. */
	
	/*! The next write-set entry whose address falls within the same cacheline. These
	    entries may be written together with a single cache-line flush. */
	struct nonvolatile_write_set_entry_s* next_cache_neighbor;
};

#ifndef NONVOLATILE_WRITE_SET_SIZE
	/*!
	 * The number of write entries to store in each write set block by default. A
	 * lower number would mean smaller allocations of persistent memory, but potentially
	 * more if the write-sets grow large.
	 *
	 * By the way, <em>real</em> constant propagation (such as generalized constant
	 * expressions) will be really nice to have. Screw C.
	 * http://en.wikipedia.org/wiki/C%2B%2B0x#Generalized_constant_expressions
	 */
	#define NONVOLATILE_WRITE_SET_SIZE 2^14
#endif

struct nonvolatile_write_set_block_s {
	/*! The entries in this write set. The bounds herein are described by size; */
	nonvolatile_write_set_entry_t entries[NONVOLATILE_WRITE_SET_SIZE];
	
	/*!
	 * The number of live entries (entries with actual meaning) in
	 * the write set. Cells with indices >= the size are not valid.
	 *
	 * \todo This is a hot spot for PCM, even so it will probably be cached
	 *  effectively. As we know we have 3 free bits at the bottom of each
	 *  entry's address field, perhaps we could leverage these to divide
	 *  the task of tracking the number of entries.
	 */
	size_t nb_entries;
	
	bool   isFinal;    /*!< If true, indicates that this block is in process of commit. */
	bool   isIdle;     /*!< If true, states that this block is available to be allocated
	                        to record the write-set of a new transaction or extend an
	                        existing write-set. */
};

/*!
 * Available write-set blocks for use by transactions. This array shall be of size
 * as NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS.
 *
 * \note Because this is a fixed-size set of blocks, that necessarily limits the
 *  number of active transactions in the system.
 */
extern nonvolatile_write_set_t the_nonvolatile_write_sets[];

#ifndef NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS
	/*! Identifies the number of write-set blocks available in this static recovery mechanism. */
	#define NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS 128
#endif

#endif /* end of include guard: RECOVERY_H_AUCIP1SR */
