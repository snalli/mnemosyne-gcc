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

#define MNEMOSYNE_ATOMIC __transaction_relaxed

# ifdef __cplusplus
extern "C" {
# endif

void mtm_fini_global();
extern int mtm_enable_trace;

/* GCC specific. For function pointers */
struct clone_entry
{
  void *orig, *clone;
};


# ifdef __cplusplus
}
# endif
#endif /* end of include guard: MTM_H_CFA9SVDY */
