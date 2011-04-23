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
 * Implements tests for the nonvolatile_write_set component of libMTM.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include <unittest++/UnitTest++.h>
#include <nonvolatile_write_set.h>
#include "nonvolatile_write_set.fixtures.hxx"
#include "nonvolatile_write_set.helpers.hxx"

SUITE(NonvolatileWriteSet) {
	TEST_FIXTURE(AllWriteSetsAvailable, FirstAllocationDoesNotFail) {
		nonvolatile_write_set_t* first_set = nonvolatile_write_set_next_available();
		CHECK(first_set != NULL);
	}
	
	TEST_FIXTURE(AllWriteSetsAvailable, SecondAllocationDoesNotFail) {
		nonvolatile_write_set_t* first_set  = nonvolatile_write_set_next_available();
		nonvolatile_write_set_t* second_set = nonvolatile_write_set_next_available();
		CHECK(second_set != NULL);
		CHECK(second_set != first_set);
	}
	
	TEST(InitializationSucceeds) {
		nonvolatile_write_set_force_initialize();
	}
	
	TEST_FIXTURE(AllWriteSetsAreBusy, NextAvailableRespectsLimitsOnWriteSetBodySize) {
		CHECK(nonvolatile_write_set_next_available() == NULL);
	}
	
	TEST_FIXTURE(AllWriteSetsAreBusy, InitializationRespectsPreviousInitialization) {
		nonvolatile_write_set_initialize();
		CHECK(nonvolatile_write_set_next_available() == NULL);
	}
}
