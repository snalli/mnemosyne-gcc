/*!
 * \file
 * Implements extra helpers that aren't just exposure of private functions.
 */
#include "nonvolatile_write_set.helpers.hxx"

/*!
 * A singleton class that wires the nonvolatile write set structure underlying the nvws interface
 * to a temporary set, owned by this object. This allows tests to proceed
 * independently of actual production or example code.
 */
struct UseTestWriteSets
{
	/*! Connects the global nonvolatile-write-set structure to a temporary structure. */
	UseTestWriteSets ();
	
	/*! Wires the global write sets to the original persistent images. */
	~UseTestWriteSets ();
	
private:
	/*! Recalls the pointer address of the actual (persistent) write sets. */
	nonvolatile_write_set_t* const theOldWriteSets;
	
	/*! Stands in to replace the write sets which are employed by programs. */
	nonvolatile_write_set_t* theNewWriteSets;
};


// Employ the above class using RAII to jump before tests.
UseTestWriteSets theTestsAreReady;


void nonvolatile_write_set_force_initialize ()
{
	theWriteSetBlocksAreInitialized = false;
	nonvolatile_write_set_initialize();
}


UseTestWriteSets::UseTestWriteSets () :
	theNewWriteSets(new nonvolatile_write_set_t[NUMBER_OF_NONVOLATILE_WRITE_SET_BLOCKS]),
	theOldWriteSets(the_nonvolatile_write_sets)
{
	nonvolatile_write_set_t* sets = reinterpret_cast<nonvolatile_write_set_t*>(the_nonvolatile_write_sets);
	sets = theNewWriteSets;
}


UseTestWriteSets::~UseTestWriteSets ()
{
	nonvolatile_write_set_t* sets = reinterpret_cast<nonvolatile_write_set_t*>(the_nonvolatile_write_sets);
	sets = theOldWriteSets;
	delete[] theNewWriteSets;
}
