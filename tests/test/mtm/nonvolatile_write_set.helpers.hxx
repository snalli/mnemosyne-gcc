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
