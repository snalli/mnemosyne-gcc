/*!
 * \file
 * Leverages the unified namespace in C programs to expose underlying structures and functions to
 * tests. This allows deeper fixtures.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef NONVOLATILE_WRITE_SET_HELPERS_HXX_3XYS5TH
#define NONVOLATILE_WRITE_SET_HELPERS_HXX_3XYS5TH

#include <nonvolatile_write_set.h>

extern "C" {
	/*!
	 * An initialization routine that, given persistent memory already in place, sets the
	 * idle (available) bit on every nonvolatile write set block in the system. This is
	 * conditional on theWriteSetBlocksAreInitialized, which should keep it
	 * from squashing write sets on recovery.
	 */
	void nonvolatile_write_set_initialize ();
	
	/*!
	 * Runs initialization code without any care for whether initialization
	 * had happened already or not.
	 */
	void nonvolatile_write_set_force_initialize ();
	
	/*!
	 * Determines whether memory is initialized on recovery. This relies on persistent
	 * segments being zeroed by default (giving this as false).
	 */
	extern bool theWriteSetBlocksAreInitialized;
	
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
	extern nonvolatile_write_set_t the_nonvolatile_write_sets[NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS];
}

#endif /* end of include guard: NONVOLATILE_WRITE_SET_HELPERS_HXX_3XYS5TH */
