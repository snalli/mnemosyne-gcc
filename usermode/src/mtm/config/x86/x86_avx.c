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

#define M256_READ(LOCK) \
_ITM_TYPE_M256 _ITM_CALL_CONVENTION _ITM_##LOCK##M256(const _ITM_TYPE_M256 *ptr)	\
{									\
  uintptr_t iptr = (uintptr_t) ptr;					\
  uintptr_t iline = iptr & -CACHELINE_SIZE;				\
  uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);				\
  mtm_cacheline *line = mtm_disp()->LOCK (iline);			\
  _ITM_TYPE_M256 ret;							\
									\
  if (iofs + sizeof(ret) <= CACHELINE_SIZE)				\
    {									\
      return *(_ITM_TYPE_M256 *)&line->b[iofs];				\
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

#define M256_WRITE(LOCK) \
void _ITM_CALL_CONVENTION _ITM_##LOCK##M256(_ITM_TYPE_M256 *ptr, _ITM_TYPE_M256 val) \
{									\
  uintptr_t iptr = (uintptr_t) ptr;					\
  uintptr_t iline = iptr & -CACHELINE_SIZE;				\
  uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);				\
  mtm_cacheline_mask m = ((mtm_cacheline_mask)2 << (sizeof(val)-1))-1;	\
  mtm_cacheline_mask_pair pair = mtm_disp()->LOCK (iline);		\
									\
  if (iofs + sizeof(val) <= CACHELINE_SIZE)				\
    {									\
      *(_ITM_TYPE_M256 *)&pair.line->b[iofs] = val;			\
      *pair.mask |= m << iofs;						\
    }									\
  else									\
    {									\
      _ITM_TYPE_M256 sval = val;					\
      uintptr_t ileft = CACHELINE_SIZE - iofs;				\
      memcpy (&pair.line->b[iofs], &sval, ileft);			\
      *pair.mask |= m << iofs;						\
      pair = mtm_disp()->LOCK (iline + CACHELINE_SIZE);			\
      memcpy (pair.line, (char *)&sval + ileft, sizeof(sval) - ileft);	\
      *pair.mask |= m >> ileft;						\
    }									\
}

M256_READ(R)
M256_READ(RaR)
M256_READ(RaW)
M256_READ(RfW)
M256_WRITE(W)
M256_WRITE(WaR)
M256_WRITE(WaW)

void _ITM_CALL_CONVENTION
_ITM_LM256 (const _ITM_TYPE_M256 *ptr)
{
  MTM_LB (ptr, sizeof (*ptr));
}
