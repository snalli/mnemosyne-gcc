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

static inline void
cpu_relax (void)
{
  __asm volatile ("rep; nop" : : : "memory");
}

static inline void
atomic_read_barrier (void)
{
  /* x86 is a strong memory ordering machine.  */
  __asm volatile ("" : : : "memory");
}

static inline void
atomic_write_barrier (void)
{
  /* x86 is a strong memory ordering machine.  */
  __asm volatile ("" : : : "memory");
}


/* Copy a cacheline with the widest available vector type.  */
#if defined(__SSE__) || defined(__AVX__)
# define HAVE_ARCH_MTM_CACHELINE_COPY 1
static inline void
mtm_cacheline_copy (mtm_cacheline * __restrict d,
		    const mtm_cacheline * __restrict s)
{
#ifdef __AVX__
# define CP	m256
# define TYPE	__m256
#else
# define CP	m128
# define TYPE	__m128
#endif

  TYPE w, x, y, z;

  /* ??? Wouldn't it be nice to have a pragma to tell the compiler
     to completely unroll a given loop?  */
  switch (CACHELINE_SIZE / sizeof(s->CP[0]))
    {
    case 1:
      d->CP[0] = s->CP[0];
      break;
    case 2:
      x = s->CP[0];
      y = s->CP[1];
      d->CP[0] = x;
      d->CP[1] = y;
      break;
    case 4:
      w = s->CP[0];
      x = s->CP[1];
      y = s->CP[2];
      z = s->CP[3];
      d->CP[0] = w;
      d->CP[1] = x;
      d->CP[2] = y;
      d->CP[3] = z;
      break;
    default:
      __builtin_trap ();
    }
}
#endif

#if !ALLOW_UNMASKED_STORES && defined(__SSE2__)
# define HAVE_ARCH_MTM_CCM_WRITE_BARRIER 1
/* A write barrier to emit after (a series of) mtm_copy_cacheline_mask.
   Since we'll be emitting non-temporal stores, the normal strong ordering
   of the machine doesn't apply and we have to emit an SFENCE.  */
static inline void
mtm_ccm_write_barrier (void)
{
  _mm_sfence ();
}
#endif
