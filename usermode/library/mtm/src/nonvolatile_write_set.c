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
 * An initialization routine that, given persistent memory already in place, builds a 
 */
void nonvolatile_write_set_initialize ();


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
	if (theWriteSetBlocksAreInitialized) {
		size_t index;
		for (index = 0; index < NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS; ++index)
			the_nonvolatile_write_sets[index].isIdle = true;
		
		theWriteSetBlocksAreInitialized = true;
	}
}


nonvolatile_write_set_t* nonvolatile_write_set_next_available()
{
	// Use this static pointer to iterate through the sets, clock-style.
	static nonvolatile_write_set_block_t* the_next_block = the_nonvolatile_write_sets;
	
	/* Search for the next idle block. */
	pthread_mutex_lock(&the_write_set_blocks_mutex);
	nonvolatile_write_set_block_t* first_block   = the_next_block;  // Watch for a wrap-around.
	nonvolatile_write_set_block_t* current_block = first_block;     // Our iterating pointer.
	nonvolatile_write_set_block_t* found_block;                     // Used to remember an available block.
	do {
		nonvolatile_write_set_block_t* this_block = current_block;
		++current_block;
		
		if (this_block->isIdle) {
			found_block = this_block;
			break;
		}
	} while(current_block != first_block);
	pthread_mutex_unlock(&the_write_set_blocks_mutex);
	
	// Did we find an available block?
	if (found_block != NULL) {
		nonvolatile_write_set_t* next_available_set = found_block;
		
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


MNEMOSYNE_PERSISTENT 
nonvolatile_write_set_t the_nonvolatile_write_sets[NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS];
