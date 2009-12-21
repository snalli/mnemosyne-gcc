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

struct mtm_method
{
  mtm_version start;
};

/* Acquire a read lock on ADDR.  */

static mtm_cacheline *
readonly_R (uintptr_t addr)
{
  return (mtm_cacheline *) addr;
}

static mtm_cacheline *
readonly_RaW (uintptr_t addr)
{
  abort ();
}

static mtm_cacheline *
readonly_RfW (uintptr_t addr)
{
  MTM_restart_transaction (RESTART_NOT_READONLY);
}

static mtm_cacheline_mask_pair
readonly_W (uintptr_t addr)
{
  MTM_restart_transaction (RESTART_NOT_READONLY);
}

static mtm_cacheline_mask_pair
readonly_WaW (uintptr_t addr)
{
  abort ();
}

/* Commit the transaction.  */

static bool
readonly_trycommit (void)
{
  struct mtm_method *m = mtm_tx()->m;
  return mtm_get_clock () == m->start;
}

static void
readonly_rollback (void)
{
  /* Nothing to do.  */
}

static void
readonly_init (bool first)
{
  struct mtm_method *m;

  if (first)
    mtm_tx()->m = m = calloc (1, sizeof (*m));
  else
    m = mtm_tx()->m;

  m->start = mtm_get_clock ();
}

static void
readonly_fini (void)
{
  struct mtm_method *m = mtm_tx()->m;
  free (m);
}

const struct mtm_dispatch dispatch_readonly = {
  .R = readonly_R,
  .RaR = readonly_R,
  .RaW = readonly_RaW,
  .RfW = readonly_RfW,

  .W = readonly_W,
  .WaR = readonly_W,
  .WaW = readonly_WaW,

  .trycommit = readonly_trycommit,
  .rollback = readonly_rollback,
  .init = readonly_init,
  .fini = readonly_fini,

  .write_through = true
};
