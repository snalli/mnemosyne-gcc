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
 * Defines interfaces that allow one to determine whether the library has been initialized or not.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef INIT_H_FY8AG1WS
#define INIT_H_FY8AG1WS

#include <stdint.h>

/*! Greater than zero if mnemosyne has been initialized (or more importantly: reincarnated) */
extern volatile uint32_t mnemosyne_initialized;

#endif /* end of include guard: INIT_H_FY8AG1WS */
