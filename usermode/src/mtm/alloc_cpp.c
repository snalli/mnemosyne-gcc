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

/* Everything from libstdc++ is weak, to avoid requiring that library
   to be linked into plain C applications using libitm.so.  */

extern void *_Znwm (size_t) __attribute__((weak));
extern void _ZdlPv (void *) __attribute__((weak));
extern void *_Znam (size_t) __attribute__((weak));
extern void _ZdaPv (void *) __attribute__((weak));

typedef const struct nothrow_t { } *c_nothrow_p;

extern void *_ZnwmRKSt9nothrow_t (size_t, c_nothrow_p) __attribute__((weak));
extern void _ZdlPvRKSt9nothrow_t (void *, c_nothrow_p) __attribute__((weak));
extern void *_ZnamRKSt9nothrow_t (size_t, c_nothrow_p) __attribute__((weak));
extern void _ZdaPvRKSt9nothrow_t (void *, c_nothrow_p) __attribute__((weak));

/* Wrap the delete nothrow symbols for usage with a single argument.
   Perhaps should have a configure type check for this, because the
   std::nothrow_t reference argument is unused (empty class), and most
   targets don't actually need that second argument.  So we _could_
   invoke these functions as if they were a single argument free.  */
static void
del_opnt (void *ptr)
{
  _ZdlPvRKSt9nothrow_t (ptr, NULL);
}

static void
del_opvnt (void *ptr)
{
  _ZdaPvRKSt9nothrow_t (ptr, NULL);
}

/* Wrap: operator new (std::size_t sz)  */
void *
_ZGTtnwm (size_t sz)
{
	void *r = _Znwm (sz);
	if (r) {
		mtm_alloc_record_allocation (r, sz, _ZdlPv);
	}	
	return r;
}

/* Wrap: operator new (std::size_t sz, const std::nothrow_t&)  */
void *
_ZGTtnwmRKSt9nothrow_t (size_t sz, c_nothrow_p nt)
{
	void *r = _ZnwmRKSt9nothrow_t (sz, nt);
	if (r) {
		mtm_alloc_record_allocation (r, sz, del_opnt);
	}	
	return r;
}

/* Wrap: operator new[] (std::size_t sz)  */
void *
_ZGTtnam (size_t sz)
{
	void *r = _Znam (sz);
	if (r) {
		mtm_alloc_record_allocation (r, sz, _ZdaPv);
	}
	return r;
}

/* Wrap: operator new[] (std::size_t sz, const std::nothrow_t& nothrow)  */
void *
_ZGTtnamRKSt9nothrow_t (size_t sz, c_nothrow_p nt)
{
	void *r = _ZnamRKSt9nothrow_t (sz, nt);
	if (r) {
		mtm_alloc_record_allocation (r, sz, del_opvnt);
	}	
	return r;
}

/* Wrap: operator delete(void* ptr)  */
void
_ZGTtdlPv (void *ptr)
{
	if (ptr) {
		mtm_alloc_forget_allocation (ptr, _ZdlPv);
	}	
}

/* Wrap: operator delete (void *ptr, const std::nothrow_t&)  */
void
_ZGTtdlPvRKSt9nothrow_t (void *ptr, c_nothrow_p nt UNUSED)
{
	if (ptr) {
		mtm_alloc_forget_allocation (ptr, del_opnt);
	}	
}

/* Wrap: operator delete[] (void *ptr)  */
void
_ZGTtdaPv (void *ptr)
{
	if (ptr) {
		mtm_alloc_forget_allocation (ptr, _ZdaPv);
	}	
}

/* Wrap: operator delete[] (void *ptr, const std::nothrow_t&)  */
void
_ZGTtdaPvRKSt9nothrow_t (void *ptr, c_nothrow_p nt UNUSED)
{
	if (ptr) {
		mtm_alloc_forget_allocation (ptr, del_opvnt);
	}	
}
