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
