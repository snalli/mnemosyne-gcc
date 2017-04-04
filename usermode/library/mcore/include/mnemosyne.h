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

#ifndef PSEGMENT_RESERVED_REGION_START
# define PSEGMENT_RESERVED_REGION_START   0x0000100000000000
#endif
#ifndef PSEGMENT_RESERVED_REGION_SIZE
# define PSEGMENT_RESERVED_REGION_SIZE    0x0000010000000000 /* 1 TB */
#endif

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

void *m_pmap(void *start, unsigned long long length, int prot, int flags);
void *m_pmap2(void *start, unsigned long long  length, int prot, int flags);
int  m_punmap(void *start, unsigned long long length);

void mnemosyne_init_global(void);

# ifdef __cplusplus
}
# endif

#endif /* end of include guard: MNEMOSYNE_H_4EOVRWJH */
