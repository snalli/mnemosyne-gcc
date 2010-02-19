/*!
 * \file
 * Defines the public interface to the mnemosyne persistent memory allocator.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef _MNEMOSYNE_H

# define MNEMOSYNE_PERSISTENT __attribute__ ((section("PERSISTENT")))
# define MNEMOSYNE_ATOMIC __tm_atomic

# ifdef __cplusplus
extern "C" {
# endif

#include <sys/types.h>

/*! The priority with which the mnemosyne initialization routines will run. */
#ifndef MNEMOSYNE_INITIALIZATION_PRIORITY
#define MNEMOSYNE_INITIALIZATION_PRIORITY 0xffff
#endif

/*!
 * Registers an callback function which is run upon reincarnation of a persistent
 * address space. The client library (or application) may, at this time, be assured
 * that any persistent globals are valid, as well as any dynamic segments which are run.
 *
 * To register a callback successfully, the routine initializing the callback must run
 * in a constructor (the gcc attribute constructor) with priority higher
 * (closer to zero) than the mnemosyne initialization functions. An acceptable priority
 * is given by MNEMOSYNE_INITIALIZATION_PRIORITY - 1.
 *
 * Callbacks registered through this method will be executed in order.
 *
 * \param initialize will be called immediately upon complete reincarnation of the
 *  persistent address space.
 */
void mnemosyne_reincarnation_callback_register(void(*initializer)());

void *mnemosyne_segment_create(void *start, size_t length, int prot, int flags);
int mnemosyne_segment_destroy(void *start, size_t length);
void *mnemosyne_malloc(size_t bytes);
void mnemosyne_free(void *ptr);

# ifdef __cplusplus
}
# endif

#endif /* _MNEMOSYNE_H */
