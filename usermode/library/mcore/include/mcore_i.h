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
 * Defines types and function atttributes common throughout libmcore sources.
 * 
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#ifndef MNEMOSYNE_I_H_IBT0Y37D
#define MNEMOSYNE_I_H_IBT0Y37D

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <debug.h>

#ifdef __i386__
# define ITM_REGPARM	__attribute__((regparm(2)))
#else
# define ITM_REGPARM
#endif

#define ITM_NORETURN	__attribute__((noreturn))

#include <xmmintrin.h>

typedef uint8_t  _ITM_TYPE_U1;
typedef uint16_t _ITM_TYPE_U2;
typedef uint32_t _ITM_TYPE_U4;
typedef uint64_t _ITM_TYPE_U8;
typedef float    _ITM_TYPE_F;
typedef double   _ITM_TYPE_D;
typedef long double _ITM_TYPE_E;
typedef __m64  _ITM_TYPE_M64;
typedef __m128  _ITM_TYPE_M128;
typedef float _Complex _ITM_TYPE_CF;
typedef double _Complex _ITM_TYPE_CD;
typedef long double _Complex _ITM_TYPE_CE;

#include "thrdesc.h"

#endif /* end of include guard: MNEMOSYNE_I_H_IBT0Y37D */
