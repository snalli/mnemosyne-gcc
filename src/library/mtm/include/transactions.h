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
 * Defines a collection of helpers for guiding a transactional compiler according with the Intel STM ABI.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef TRANSACTIONS_H_MED8GE8A
#define TRANSACTIONS_H_MED8GE8A

#ifndef TM_CALLABLE
#define TM_CALLABLE __attribute__((transaction_safe))
#endif

#define TM_WAIVER

#ifndef TM_WRAPPING
#define TM_WRAPPING(function) __attribute__((tm_wrapping(function)))
#endif

#endif /* end of include guard: TRANSACTIONS_H_MED8GE8A */
