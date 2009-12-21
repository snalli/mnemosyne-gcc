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


#define M64_READ(LOCK) \
_ITM_TYPE_M64 _ITM_CALL_CONVENTION _ITM_##LOCK##M64(const _ITM_TYPE_M64 *ptr)	\
{									\
  uintptr_t iptr = (uintptr_t) ptr;					\
  uintptr_t iline = iptr & -CACHELINE_SIZE;				\
  uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);				\
  mtm_cacheline *line = mtm_disp()->LOCK (iline);			\
  _ITM_TYPE_M64 ret;							\
									\
  if (iofs + sizeof(ret) <= CACHELINE_SIZE)				\
    {									\
      return *(_ITM_TYPE_M64 *)&line->b[iofs];				\
    }									\
  else									\
    {									\
      uintptr_t ileft = CACHELINE_SIZE - iofs;				\
      memcpy (&ret, &line->b[iofs], ileft);				\
      line = mtm_disp()->LOCK (iline + CACHELINE_SIZE);			\
      memcpy ((char *)&ret + ileft, line, sizeof(ret) - ileft);		\
    }									\
  return ret;								\
}

#define M64_WRITE(LOCK) \
void _ITM_CALL_CONVENTION _ITM_##LOCK##M64(_ITM_TYPE_M64 *ptr, _ITM_TYPE_M64 val) \
{									\
  uintptr_t iptr = (uintptr_t) ptr;					\
  uintptr_t iline = iptr & -CACHELINE_SIZE;				\
  uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);				\
  mtm_cacheline_mask m = ((mtm_cacheline_mask)1 << sizeof(val)) - 1;	\
  mtm_cacheline_mask_pair pair = mtm_disp()->LOCK (iline);		\
									\
  if (iofs + sizeof(val) <= CACHELINE_SIZE)				\
    {									\
      *(_ITM_TYPE_M64 *)&pair.line->b[iofs] = val;			\
      *pair.mask |= m << iofs;						\
    }									\
  else									\
    {									\
      _ITM_TYPE_M64 sval = val;						\
      uintptr_t ileft = CACHELINE_SIZE - iofs;				\
      memcpy (&pair.line->b[iofs], &sval, ileft);			\
      *pair.mask |= m << iofs;						\
      pair = mtm_disp()->LOCK (iline + CACHELINE_SIZE);			\
      memcpy (pair.line, (char *)&sval + ileft, sizeof(sval) - ileft);	\
      *pair.mask |= m >> ileft;						\
    }									\
}

/* Note that unlike all other X86 types, the SSE vector types do
   require strict alignment by default.  Use the unaligned load
   and store builtins when necessary.  */

#define M128_READ(LOCK) \
_ITM_TYPE_M128 _ITM_CALL_CONVENTION _ITM_##LOCK##M128(const _ITM_TYPE_M128 *ptr)	\
{									\
  uintptr_t iptr = (uintptr_t) ptr;					\
  uintptr_t iline = iptr & -CACHELINE_SIZE;				\
  uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);				\
  mtm_cacheline *line = mtm_disp()->LOCK (iline);			\
  _ITM_TYPE_M128 ret;							\
									\
  if ((iofs & (sizeof (ret) - 1)) == 0)					\
    {									\
      return *(_ITM_TYPE_M128 *)&line->b[iofs];				\
    }									\
  else if (iofs + sizeof(ret) <= CACHELINE_SIZE)			\
    {									\
      return _mm_loadu_ps ((const float *) &line->b[iofs]);		\
    }									\
  else									\
    {									\
      uintptr_t ileft = CACHELINE_SIZE - iofs;				\
      memcpy (&ret, &line->b[iofs], ileft);				\
      line = mtm_disp()->LOCK (iline + CACHELINE_SIZE);			\
      memcpy ((char *)&ret + ileft, line, sizeof(ret) - ileft);		\
    }									\
  return ret;								\
}

#define M128_WRITE(LOCK) \
void _ITM_CALL_CONVENTION _ITM_##LOCK##M128(_ITM_TYPE_M128 *ptr, _ITM_TYPE_M128 val) \
{									\
  uintptr_t iptr = (uintptr_t) ptr;					\
  uintptr_t iline = iptr & -CACHELINE_SIZE;				\
  uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);				\
  mtm_cacheline_mask m = ((mtm_cacheline_mask)1 << sizeof(val)) - 1;	\
  mtm_cacheline_mask_pair pair = mtm_disp()->LOCK (iline);		\
									\
  if ((iofs & (sizeof (val) - 1)) == 0)					\
    {									\
      *(_ITM_TYPE_M128 *)&pair.line->b[iofs] = val;			\
      *pair.mask |= m << iofs;						\
    }									\
  else if (iofs + sizeof(val) <= CACHELINE_SIZE)			\
    {									\
      _mm_storeu_ps ((float *) &pair.line->b[iofs], val);		\
      *pair.mask |= m << iofs;						\
    }									\
  else									\
    {									\
      _ITM_TYPE_M128 sval = val;					\
      uintptr_t ileft = CACHELINE_SIZE - iofs;				\
      memcpy (&pair.line->b[iofs], &sval, ileft);			\
      *pair.mask |= m << iofs;						\
      pair = mtm_disp()->LOCK (iline + CACHELINE_SIZE);			\
      memcpy (pair.line, (char *)&sval + ileft, sizeof(sval) - ileft);	\
      *pair.mask |= m >> ileft;						\
    }									\
}

M64_READ(R)
M64_READ(RaR)
M64_READ(RaW)
M64_READ(RfW)
M64_WRITE(W)
M64_WRITE(WaR)
M64_WRITE(WaW)

M128_READ(R)
M128_READ(RaR)
M128_READ(RaW)
M128_READ(RfW)
M128_WRITE(W)
M128_WRITE(WaR)
M128_WRITE(WaW)


void _ITM_CALL_CONVENTION
_ITM_LM64 (const _ITM_TYPE_M64 *ptr)
{
  MTM_LB (ptr, sizeof (*ptr));
}

void _ITM_CALL_CONVENTION
_ITM_LM128 (const _ITM_TYPE_M128 *ptr)
{
  MTM_LB (ptr, sizeof (*ptr));
}
