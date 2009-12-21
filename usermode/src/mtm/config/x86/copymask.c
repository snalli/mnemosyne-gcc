/* Copyright (C) 2009 Free Software Foundation, Inc.
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

#include "mtm_i.h"


static uint32_t const bit_to_byte_mask[16] =
{
  0x00000000,
  0x000000ff,
  0x0000ff00,
  0x0000ffff,
  0x00ff0000,
  0x00ff00ff,
  0x00ffff00,
  0x00ffffff,
  0xff000000,
  0xff0000ff,
  0xff00ff00,
  0xff00ffff,
  0xffff0000,
  0xffff00ff,
  0xffffff00,
  0xffffffff
};

#ifdef __SSE2__

void
mtm_cacheline_copy_mask (mtm_cacheline * __restrict d,
			 const mtm_cacheline * __restrict s,
			 mtm_cacheline_mask m)
{
	int i;

	if (m == (mtm_cacheline_mask)-1) {
		mtm_cacheline_copy (d, s);
		return;
    }
	if (__builtin_expect (m == 0, 0)) {
		return;
	}	

	for (i = 0; i < CACHELINE_SIZE / 16; ++i) {
		mtm_cacheline_mask m16 = m & 0xffff;
		if (__builtin_expect (m16 == 0, 0)) {
			goto skip16;
		}	
		if (__builtin_expect (m16 == 0xffff, 1)) {
			d->m128i[i] = s->m128i[i];
		skip16:
			m >>= 16;
		} else {
			__m128i bm0;
			__m128i bm1;
			__m128i bm2;
			__m128i bm3;
			bm0 = _mm_set_epi32 (0, 0, 0, bit_to_byte_mask[m & 15]); m >>= 4;
			bm1 = _mm_set_epi32 (0, 0, 0, bit_to_byte_mask[m & 15]); m >>= 4;
			bm2 = _mm_set_epi32 (0, 0, 0, bit_to_byte_mask[m & 15]); m >>= 4;
			bm3 = _mm_set_epi32 (0, 0, 0, bit_to_byte_mask[m & 15]); m >>= 4;
			bm0 = _mm_unpacklo_epi32 (bm0, bm1);
			bm2 = _mm_unpacklo_epi32 (bm2, bm3);
			bm0 = _mm_unpacklo_epi64 (bm0, bm2);

			if (ALLOW_UNMASKED_STORES) {
				/* 
				 * Implementing the following expression using intrinsics:
				 * d->m128i[i] = (d->m128i[i] & ~bm0) | (s->m128i[i] & bm0);
				 */
				d->m128i[i] = _mm_or_si128(_mm_andnot_si128(bm0, d->m128i[i]),
				                           _mm_and_si128(bm0, s->m128i[i]));
			} else {
				_mm_maskmoveu_si128 (s->m128i[i], bm0, (char *)&d->m128i[i]);
			}
    	}
	}	
}

#else
/* ??? If we don't have SSE2, I believe we can honor !ALLOW_UNMASKED_STORES
   more efficiently with an unlocked cmpxchg insn.  My reasoning is that 
   while we write to locations that we do not wish to modify, we do it in
   an uninterruptable insn, and so we either truely write back the original
   data or the insn fails -- unlike with a load/and/or/write sequence which
   can be interrupted either by a kernel task switch or an unlucky cacheline
   steal by another processor.  Avoiding the LOCK prefix improves performance
   by a factor of 10, and we don't need the memory barrier semantics
   implied by that prefix.  */

static void __attribute__((always_inline))
copy_mask_w (mtm_word * __restrict d,
	     const mtm_word * __restrict s,
	     mtm_cacheline_mask m)
{
  mtm_cacheline_mask tm = (1 << sizeof (mtm_word)) - 1;

  //if (__builtin_expect (m & tm, tm))
  if (m & tm == tm)
    {
      if (__builtin_expect ((m & tm) == tm, 1))
	*d = *s;
      else if (sizeof (mtm_word) == 4)
	{
	  mtm_cacheline_mask bm = bit_to_byte_mask[m & 15];

	  if (ALLOW_UNMASKED_STORES)
	    *d = (*d & ~bm) | (*s & bm);
	  else
	    {
	      mtm_word n, o = *d;
	      asm ("\n0:\t"
		"mov	%[o], %[n]\n\t"
		"and	%[m], %[n]\n\t"
		"or	%[s], %[n]\n\t"
		"cmpxchg %[n], %[d]\n\t"
		"jnz,pn	0b"
		: [d] "+m"(*d), [n] "=&r" (n), [o] "+a"(o)
		: [s] "r" (*s & bm), [m] "r" (~bm));
	    }
	}
      else if (sizeof (mtm_word) == 8)
	{
	  uint32_t bl = bit_to_byte_mask[m & 15];
	  uint32_t bh = bit_to_byte_mask[(m >> 4) & 15];
	  mtm_cacheline_mask bm = bl | ((mtm_cacheline_mask)bh << 31 << 1);

	  if (ALLOW_UNMASKED_STORES)
	    *d = (*d & ~bm) | (*s & bm);
	  else
	    {
#ifdef __x86_64__
	      mtm_word n, o = *d;
	      asm ("\n0:\t"
		"mov	%[o], %[n]\n\t"
		"and	%[m], %[n]\n\t"
		"or	%[s], %[n]\n\t"
		"cmpxchg %[n], %[d]\n\t"
		"jnz,pn	0b"
		: [d] "+m"(*d), [n] "=&r" (n), [o] "+a"(o)
		: [s] "r" (*s & bm), [m] "r" (~bm));
#else
	      /* ??? While it's possible to perform this operation with
		 cmpxchg8b, the sequence requires all 7 general registers
		 and thus cannot be performed with -fPIC.  Don't even try.  */
	      __builtin_trap ();
#endif
	    }
	}
    }
}

void
mtm_cacheline_copy_mask (mtm_cacheline * __restrict d,
			 const mtm_cacheline * __restrict s,
			 mtm_cacheline_mask m)
{
  const size_t n = sizeof (mtm_word);
  size_t i;

  if (m == (mtm_cacheline_mask)-1)
    {
      mtm_cacheline_copy (d, s);
      return;
    }
  if (__builtin_expect (m == 0, 0))
    return;

  for (i = 0; i < CACHELINE_SIZE / n; ++i, m >>= n)
    copy_mask_w (&d->w[i], &s->w[i], m);
}

#endif /* SSE2 */
