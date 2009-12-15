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

/* Provide access to the futex system call.  */

#include "mtm_i.h"
#include "futex.h"
#include <errno.h>


#define FUTEX_WAIT          0
#define FUTEX_WAKE          1
#define FUTEX_PRIVATE_FLAG  128L


static long int mtm_futex_wait = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
static long int mtm_futex_wake = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;


void
futex_wait (int *addr, int val)
{
	unsigned long long i, count = mtm_spin_count_var;
	long res;

	for (i = 0; i < count; i++) {
		if (__builtin_expect (*addr != val, 0)) {
			return;
		} else {
			cpu_relax ();
		}
	}

	res = sys_futex0 (addr, mtm_futex_wait, val);
	if (__builtin_expect (res == -ENOSYS, 0)) {
		mtm_futex_wait = FUTEX_WAIT;
		mtm_futex_wake = FUTEX_WAKE;
		sys_futex0 (addr, FUTEX_WAIT, val);
	}
}


void
futex_wake (int *addr, int count)
{
	long res = sys_futex0 (addr, mtm_futex_wake, count);
	if (__builtin_expect (res == -ENOSYS, 0)) {
		mtm_futex_wait = FUTEX_WAIT;
		mtm_futex_wake = FUTEX_WAKE;
		sys_futex0 (addr, FUTEX_WAKE, count);
	}
}
