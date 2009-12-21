/* Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

# include <stdint.h>
# include <xmmintrin.h>

typedef struct mtm_jmpbuf_s mtm_jmpbuf_t;

#ifdef __x86_64__
struct mtm_jmpbuf_s
{
/*
  unsigned long cfa;
  unsigned long rip;
  unsigned long rbx;
  unsigned long rbp;
  unsigned long r12;
  unsigned long r13;
  unsigned long r14;
  unsigned long r15;
*/ 
	uint64_t spSave;
	uint64_t rbxSave;
	uint64_t rbpSave;
	uint64_t r12Save;
	uint64_t r13Save;
	uint64_t r14Save;
	uint64_t r15Save;
	uint64_t abendPCSave;
	uint32_t mxcsrSave;
	uint32_t txnFlagsSave;      /*<< Transaction flags */
	uint16_t fpcsrSave;
};
#else
struct mtm_jmpbuf_s
{
/*
  unsigned long cfa;
  unsigned long ebx;
  unsigned long esi;
  unsigned long edi;
  unsigned long ebp;
  unsigned long eip;
*/  
	uint32_t spSave;
	uint32_t ebxSave;
	uint32_t esiSave;
	uint32_t ediSave;
	uint32_t ebpSave;
	uint32_t abendPCSave;
	uint32_t mxcsrSave;
	uint32_t txnFlagsSave;      /*<< Transaction flags */
	uint16_t fpcsrSave;
};
#endif

/* The "cacheline" as defined by the STM need not be the same as the
   cacheline defined by the processor.  It ought to be big enough for
   any of the basic types to be stored (aligned) in one line.  It ought
   to be small enough for efficient manipulation of the modification
   mask.  The cacheline copy routines assume that if SSE is present
   that we can use it, which implies a minimum cacheline size of 16.  */
#ifdef __x86_64__
# define CACHELINE_SIZE 64
#else
# define CACHELINE_SIZE 32
#endif

/* x86 doesn't require strict alignment for the basic types.  */
#define STRICT_ALIGNMENT 0

/* x86 uses a fixed page size of 4K.  */
#define PAGE_SIZE       4096
#define FIXED_PAGE_SIZE 1

/* We'll be using some of the cpu builtins, and their associated types.  */
//#include <x86intrin.h>
