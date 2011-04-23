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
 * Specifies the interfaces by which reincarnation callbacks may be initiated from
 * within initialization routines.
 *
 * The concept here is that callback registration is initiated by 
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef REINCARNATION_CALLBACK_H_WTK12KU8
#define REINCARNATION_CALLBACK_H_WTK12KU8

/*! \see mnemosyne.h */
void mnemosyne_reincarnation_callback_register(void(*initializer)());

/*!
 * Runs all currently-registered callbacks given by clients of the mnemosyne library.
 * These callbacks will execute under the assumption that persistent memory has been
 * mapped for all global objects and that dynamically-allocated persistent segments
 * have been re-mapped.
 */
void mnemosyne_reincarnation_callback_execute_all();

#endif /* end of include guard: REINCARNATION_CALLBACK_H_WTK12KU8 */
