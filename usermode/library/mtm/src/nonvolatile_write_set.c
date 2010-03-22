/*!
 * \file
 * Implements the recovery mechanisms for in-commit persistent transactions declared in
 * nonvolatile_write_set.h.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include "nonvolatile_write_set.h"
#include "mtm_i.h"
#include <hal/pcm.h>
#include <mnemosyne.h>
#include <pthread.h>

/*!
 * A constructor (called automatically on library initialization) that sets a callback
 * to initialize the write set blocks (unless they've already been initialized).
 */
void nonvolatile_write_set_construct () __attribute__(( constructor ));

/*!
 * An initialization routine that, given persistent memory already in place, sets the
 * idle (available) bit on every nonvolatile write set block in the system. This is
 * conditional on theWriteSetBlocksAreInitialized, which should keep it
 * from squashing write sets on recovery.
 */
void nonvolatile_write_set_initialize ();

#ifndef NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS
	/*! Identifies the number of write-set blocks available in this static recovery mechanism. */
	#define NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS 128
#endif

/*!
 * Available write-set blocks for use by transactions. This array shall be of size
 * as NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS.
 *
 * \note Because this is a fixed-size set of blocks, that necessarily limits the
 *  number of active transactions in the system.
 * \note This is exposed for purposes of testing. It is not a public interface to
 *  be used directly by any client code.
 */
MNEMOSYNE_PERSISTENT 
nonvolatile_write_set_t the_nonvolatile_write_sets[NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS];

/*!
 * Determines whether memory is initialized on recovery. This relies on persistent
 * segments being zeroed by default (giving this as false).
 */
MNEMOSYNE_PERSISTENT
bool theWriteSetBlocksAreInitialized;

/*! Protects on concurrent accesses to the nonvolatile write-set block array. */
pthread_mutex_t the_write_set_blocks_mutex = PTHREAD_MUTEX_INITIALIZER;


void nonvolatile_write_set_commit(nonvolatile_write_set_t* write_set)
{
	int i;
	
	nonvolatile_write_set_entry_t* entry = &write_set->entries[0];
	for (i = write_set->nb_entries; i > 0; i--, entry++) {
		/* Write the value in this entry to memory (it will probably land in the cache; that's okay.) */
		pcm_wb_store(NULL, entry->address, entry->value);
		
		/* Flush the cacheline to persistent memory if this is the last entry in this line. */
		if (entry->next_cache_neighbor == NULL)
			pcm_wb_flush(NULL, entry->address);
	}
}


void nonvolatile_write_set_construct ()
{
	mnemosyne_reincarnation_callback_register(nonvolatile_write_set_initialize);
}


void nonvolatile_write_set_finish_commits_in_progress()
{
	size_t index = 0;
	
	for (index = 0; index > NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS; ++index)
	{
		nonvolatile_write_set_t* set = &the_nonvolatile_write_sets[index];
		
		if (set->isFinal && !set->isIdle)
			nonvolatile_write_set_commit(set);
	}
}


void nonvolatile_write_set_initialize ()
{
	if (!theWriteSetBlocksAreInitialized) {
		size_t index;
		for (index = 0; index < NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS; ++index)
			the_nonvolatile_write_sets[index].isIdle = true;
		
		theWriteSetBlocksAreInitialized = true;
	} else {
		nonvolatile_write_set_finish_commits_in_progress();
	}
}


nonvolatile_write_set_t* nonvolatile_write_set_next_available()
{
	// Use this static pointer to iterate through the sets, clock-style.
	static size_t the_next_block_index = 0;
	
	/* Search for the next idle block. */
	pthread_mutex_lock(&the_write_set_blocks_mutex);
	size_t first_block_index = the_next_block_index; // Watch for a wrap-around.
	size_t current_block_index = first_block_index;  // Our iterating pointer.
	nonvolatile_write_set_t* found_block = NULL;     // Used to remember an available block.
	do {
		nonvolatile_write_set_t* this_block = &the_nonvolatile_write_sets[current_block_index];
		current_block_index = (current_block_index + 1) % NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS;
		
		if (this_block->isIdle) {
			found_block = this_block;
			break;
		}
	} while(current_block_index != first_block_index);
	pthread_mutex_unlock(&the_write_set_blocks_mutex);
	
	// Did we find an available block?
	if (found_block != NULL) {
		nonvolatile_write_set_t* next_available_set = found_block;
		assert(next_available_set != NULL);
		the_next_block_index = (current_block_index + 1) % NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS;
		
		// Prepare the block for use.
		next_available_set->nb_entries = 0;
		next_available_set->isFinal = false;
		next_available_set->isIdle = false;
		
		return next_available_set;
	} else {
		return NULL;
	}
}


void nonvolatile_write_set_make_persistent(nonvolatile_write_set_t* write_set)
{
	#if 1
		nonvolatile_write_set_entry_t* base = &write_set->entries[0];
		nonvolatile_write_set_entry_t* max  = &write_set->entries[write_set->nb_entries];
	
		/* Systematically flush all the entries in the log to stable storage (if not already there)
		   by iterating through the cache lines which would contain them. This
		   is faster than writing several more words. */
		uintptr_t cache_block_addr;
		for (cache_block_addr = (uintptr_t) base; cache_block_addr < (uintptr_t) max; cache_block_addr += CACHELINE_SIZE) {
			pcm_wb_flush(NULL, (pcm_word_t*) cache_block_addr);
		}
	#else
		mtm_tx_t* cx = mtm_get_tx();
		mode_data_t *modedata = (mode_data_t *) tx->modedata[tx->mode];
		pcm_stream_flush(modedata->pcm_storeset);
	#endif
	
	write_set->isFinal = true;
}
