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

#ifndef MTM_RWLOCK_H
#define MTM_RWLOCK_H

/* The read-write summary definition.  */

#define RWLOCK_S_LOCK		1
#define RWLOCK_A_WRITER		2
#define RWLOCK_W_WRITER		4
#define RWLOCK_A_READER		8
#define RWLOCK_W_READER		16
#define RWLOCK_RW_UPGRADE	32

typedef struct {
  int summary;
  int a_readers;
  int w_readers;
  int w_writers;
} mtm_rwlock;

extern void mtm_rwlock_read_lock (mtm_rwlock *);
extern void mtm_rwlock_write_lock (mtm_rwlock *);
extern bool mtm_rwlock_write_upgrade (mtm_rwlock *);
extern void mtm_rwlock_read_unlock (mtm_rwlock *);
extern void mtm_rwlock_write_unlock (mtm_rwlock *);

#endif /* MTM_RWLOCK_H */
