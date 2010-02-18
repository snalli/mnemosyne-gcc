/*!
 * \file
 * Routines concerning the checkpointing and restoration of an address space with persistent segments.
 *
 * \auther Haris Volos <hvolos@cs.wisc.edu>
 */
#ifndef ADDRESS_SPACE_H_XBFT9DH2
#define ADDRESS_SPACE_H_XBFT9DH2

#include <common/result.h>

/*!
 * Maps persistent memory segments back into the live address space of the program. This is
 * done by reading the persistent segments roster file.
 *
 * \todo More details would be nice.
 */
mnemosyne_result_t mnemosyne_segment_address_space_checkpoint();

/*!
 * Loads all persistent segments into memory. This includes persistent segments static to
 * the running program, those required by dynamic modules (e.g. libraries) and persistent
 * segments which were created dynamically with the last instance of the program.
 *
 * On completion of this function (and not before its completion), persistent data
 * structures in any component of the application are available for use.
 */
mnemosyne_result_t mnemosyne_segment_address_space_reincarnate();

#endif /* end of include guard: ADDRESS_SPACE_H_XBFT9DH2 */
