/*!
 * \file
 * Implements the fixtures for nonvolatile_write_set tests.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include "nonvolatile_write_set.fixtures.hxx"
#include <nonvolatile_write_set.h>
#include <string>
using std::size_t;


AllWriteSetsAvailable::AllWriteSetsAvailable ()
{
	// This test shouldn't be in threads, so we're okay just ham-fisting this.
	for (size_t setNumber = 0; setNumber < NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS; ++setNumber)
		the_nonvolatile_write_sets[setNumber].isIdle = true;
}
