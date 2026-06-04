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
 * Implements the fixtures for nonvolatile_write_set tests.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include "nonvolatile_write_set.fixtures.hxx"
#include "nonvolatile_write_set.helpers.hxx"
#include <nonvolatile_write_set.h>
#include <string>
using std::size_t;


AllWriteSetsAvailable::AllWriteSetsAvailable ()
{
	// This test shouldn't be in threads, so we're okay just ham-fisting this.
	nonvolatile_write_set_force_initialize();
}


AllWriteSetsAreBusy::AllWriteSetsAreBusy ()
{
	for (size_t i = 0; i < NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS; ++i)
		nonvolatile_write_set_next_available();
}
