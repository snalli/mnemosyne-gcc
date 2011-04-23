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
