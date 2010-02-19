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
struct recoverable_write_set_block_s;

/*! Defines a type for the write-set block. */
typedef struct recoverable_write_set_block_s recoverable_write_set_block_t;

/*!
 * Available write-set blocks for use by transactions. This array shall be of size
 * as NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS.
 *
 * \note Because this is a fixed-size set of blocks, that necessarily limits the
 *  number of active transactions in the system.
 */
extern recoverable_write_set_block_t the_nonvolatile_write_set_blocks[];


//
// Here follow details on the above structures.
//

struct nonvolatile_write_set_entry_s
{
	mtm_word_t* address;   /*!< The address which is written. This address should always be word-aligned. */
	mtm_word_t value;      /*!< The value which is written at address. */
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

struct recoverable_write_set_block_s {
	/*! The entries in this write set. The bounds herein are described by size; */
	nonvolatile_write_set_entry_t entries[NONVOLATILE_WRITE_SET_SIZE];

	bool   idle; /*!< If true, states that this block is available to be allocated
	                  to record the write-set of a new transaction or extend an
	                  existing write-set. */
	size_t size; /*!< The number of live entries (entries with actual meaning) in
	                  the write set. Cells with indices >= the size are not valid. */
};

#ifndef NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS
	/*! Identifies the number of write-set blocks available in this static recovery mechanism. */
	#define NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS 128
#endif

#endif /* end of include guard: RECOVERY_H_AUCIP1SR */
