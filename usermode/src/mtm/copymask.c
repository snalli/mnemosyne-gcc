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

#if ALLOW_UNMASKED_STORES
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

static void __attribute__((always_inline))
copy_mask_w (mtm_word * __restrict d,
	     const mtm_word * __restrict s,
             mtm_cacheline_mask m)
{
  mtm_cacheline_mask tm = (1 << sizeof (mtm_word)) - 1;

  if (m & tm)
    {
      if ((m & tm) == tm)
	*d = *s;
      else
	{
	  mtm_cacheline_mask bm;

	  switch (sizeof (mtm_word))
	    {
	    case 8:
	      bm = bit_to_byte_mask[(m >> 4) & 15];
	      bm <<= 4 * sizeof (mtm_word);
	      bm |= bit_to_byte_mask[m & 15];
	      break;
	    case 4:
	      bm = bit_to_byte_mask[m & 15];
	      break;
	    default:
	      __builtin_trap ();
	    }
	  *d = (*d & ~bm) | (*s & bm);
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
  if (__builtin_expect (m == 0), 0)
    return;

  for (i = 0; i < CACHELINE_SIZE / n; ++i, m >>= n)
    copy_mask_w (&d->w[i], &s->w[i], m);
}

#else
static inline void __attribute__((always_inline))
copy_mask_1 (mtm_cacheline * __restrict d,
	     const mtm_cacheline * __restrict s,
	     mtm_cacheline_mask m, size_t ofs, size_t idx)
{
  if (m & (1ul << ofs))
    d->b[idx] = s->b[idx];
}

static inline void __attribute__((always_inline))
copy_mask_2 (mtm_cacheline * __restrict d,
	     const mtm_cacheline * __restrict s,
	     mtm_cacheline_mask m, size_t ofs, size_t idx)
{
  mtm_cacheline_mask tm = 3ul << ofs;
  if (m & tm)
    {
      if ((m & tm) == tm)
	d->u16[idx] = s->u16[idx];
      else
        {
	  copy_mask_1 (d, s, m, ofs, idx*2);
	  copy_mask_1 (d, s, m, ofs + 1, idx*2 + 1);
	}
    }
}

static inline void __attribute__((always_inline))
copy_mask_4 (mtm_cacheline * __restrict d,
	     const mtm_cacheline * __restrict s,
	     mtm_cacheline_mask m, size_t ofs, size_t idx)
{
  mtm_cacheline_mask tm = 15ul << ofs;
  if (m & tm)
    {
      if ((m & tm) == tm)
	d->u32[idx] = s->u32[idx];
      else
	{
	  copy_mask_2 (d, s, m, ofs, idx*2);
	  copy_mask_2 (d, s, m, ofs + 2, idx*2 + 1);
	}
    }
}

static inline void __attribute__((always_inline))
copy_mask_8 (mtm_cacheline * __restrict d,
	     const mtm_cacheline * __restrict s,
	     mtm_cacheline_mask m, size_t ofs, size_t idx)
{
  mtm_cacheline_mask tm = 0xfful << ofs;
  if (m & tm)
    {
      if ((m & tm) == tm)
	d->u64[idx] = s->u64[idx];
      else
	{
	  copy_mask_4 (d, s, m, ofs, idx*2);
	  copy_mask_4 (d, s, m, ofs + 4, idx*2 + 1);
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
    switch (n)
      {
      case 8:
	copy_mask_8 (d, s, m, 0, i);
	break;
      case 4:
	copy_mask_4 (d, s, m, 0, i);
	break;
      default:
	__builtin_trap ();
      }
}

#endif /* ALLOW_UNMASKED_STORES */
