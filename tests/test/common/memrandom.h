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

#ifndef _MEMRANDOM_H
#define _MEMRANDOM_H

static inline void writeMemRandom(void *ptr, size_t sz, unsigned int seed)
{
	size_t       i;
	uintptr_t    *uptr;
	uintptr_t   val;
	unsigned int _seed = seed;

	for (i=0; i<sz; i+=sizeof(uintptr_t)) {
		val = rand_r(&_seed);
		uptr = (uintptr_t *) ((size_t) ptr + i);
		*uptr = val;
	}
}

static inline void assertMemRandom(void *ptr, size_t sz, unsigned int seed)
{
	size_t       i;
	uintptr_t    *uptr;
	uintptr_t   correct_val;
	uintptr_t   read_val;
	unsigned int _seed = seed;

	for (i=0; i<sz; i+=sizeof(uintptr_t)) {
		correct_val = rand_r(&_seed);
		uptr = (uintptr_t *) ((size_t) ptr + i);
		read_val = *uptr;
		CHECK(read_val == correct_val);
	}
}

#endif /* _MEMRANDOM_H */
