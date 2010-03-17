/*!
 * \file
 * Fixture structures to aid in the elegant execution of unit tests.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef NONVOLATILE_WRITE_SET_FIXTURES_HXX_JAC4VL8B
#define NONVOLATILE_WRITE_SET_FIXTURES_HXX_JAC4VL8B

/*!
 * Supports the condition, as it is the case on program startup, that all
 * write sets defined in the system are available for use.
 */
struct AllWriteSetsAvailable
{
	AllWriteSetsAvailable ();
};

/*!
 * Establishes the condition that all write sets have been taken.
 */
struct AllWriteSetsAreBusy
{
	AllWriteSetsAreBusy ();
};

#endif /* end of include guard: NONVOLATILE_WRITE_SET_FIXTURES_HXX_JAC4VL8B */
