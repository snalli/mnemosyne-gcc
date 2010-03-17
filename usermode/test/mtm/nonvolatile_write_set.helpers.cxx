/*!
 * \file
 * Implements extra helpers that aren't just exposure of private functions.
 */
#include "nonvolatile_write_set.helpers.hxx"


void nonvolatile_write_set_force_initialize ()
{
	theWriteSetBlocksAreInitialized = false;
	nonvolatile_write_set_initialize();
}
