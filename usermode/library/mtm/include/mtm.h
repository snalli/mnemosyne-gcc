/*!
 * \file
 * Defines the public interface to the Mnemosyne Transactional Memory. Having this file,
 * one is able to run durability transactions in their library or application code.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef MTM_H_CFA9SVDY
#define MTM_H_CFA9SVDY

/*!
 * Opens a durability transaction. This should be used as
 *   MNEMOSYNE_ATOMIC {
 *      // do stuff, writing to persistent and nonpersistent memory
 *   }
 *
 * If the code proceeds past the bottom curly brace, the persistent writes are
 * guaranteed to be flushed to persistent memory.
 */
#define MNEMOSYNE_ATOMIC __tm_atomic

#endif /* end of include guard: MTM_H_CFA9SVDY */
