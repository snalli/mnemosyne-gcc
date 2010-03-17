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
	nonvolatile_write_set_initialize();
}


AllWriteSetsAreBusy::AllWriteSetsAreBusy ()
{
	nonvolatile_write_set_initialize();
	for (size_t i = 0; i < NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS; ++i)
		nonvolatile_write_set_next_available();
}
