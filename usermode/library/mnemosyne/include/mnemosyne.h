/*!
 * \file
 * Defines the public interface to the mnemosyne persistent memory allocator. Having this
 * header, one is able to map persistent segments and allocate memory from them safely. One
 * may use persistent globals to point to these dynamic segments. All persistent segments will
 * be automatically reincarnated on every process startup without intervention by the
 * application programmer.
 *
 * Library implementors making use of Mnemosyne transactional memory will appreciate the 
 * reincarnation callback functionality. This functionality allows the implementor to
 * attach his own initializations to be run after persistent segments are guaranteed
 * to have been mapped back into the process.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef MNEMOSYNE_H_4EOVRWJH
#define MNEMOSYNE_H_4EOVRWJH

#include <sys/types.h>

/*!
 * Allows the declaration of a persistent global variable. A programmer making use
 * of dynamic mnemosyne segments is advised to use these variables to point into
 * the dynamic segments (which, practically, could be mapped anywhere in the
 * virtual address space). Without a global persistent pointer, the programmer
 * risks losing a portion of his address space to memory he will not recover.
 */
#define MNEMOSYNE_PERSISTENT __attribute__ ((section("PERSISTENT")))

# ifdef __cplusplus
extern "C" {
# endif

/*!
 * Registers an callback function which is run upon reincarnation of a persistent
 * address space. The client library (or application) may, at this time, be assured
 * that any persistent globals are valid, as well as any dynamic segments which are run.
 *
 * To register a callback successfully, the routine initializing the callback must run
 * in a constructor (the gcc attribute constructor). It does not matter that this 
 * constructor run before or after mnemosyne initializes; in one case, the
 * callback will be queued. In the other, it will be run.
 *
 * Callbacks registered through this method will be executed in order.
 *
 * \param initialize will be called immediately upon complete reincarnation of the
 *  persistent address space.
 */
void mnemosyne_reincarnation_callback_register(void(*initializer)());

void *m_pmap(void *start, size_t length, int prot, int flags);
int m_punmap(void *start, size_t length);

void mnemosyne_init_global(void);

# ifdef __cplusplus
}
# endif

#endif /* end of include guard: MNEMOSYNE_H_4EOVRWJH */
