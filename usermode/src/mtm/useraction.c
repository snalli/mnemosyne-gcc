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

#include "mtm_i.h"
#include "useraction.h"

struct mtm_user_action_s
{
  struct mtm_user_action_s *next;
  _ITM_userCommitFunction  fn;
  void                     *arg;
};




void
mtm_useraction_runActions (mtm_user_action_t **list)
{
  mtm_user_action_t *a = *list;

  if (a == NULL)
    return;
  *list = NULL;

  do
    {
      mtm_user_action_t *n = a->next;
      a->fn (a->arg);
      free (a);
      a = n;
    }
  while (a);
}


void
mtm_useraction_freeActions (mtm_user_action_t **list)
{
  mtm_user_action_t *a = *list;

  if (a == NULL)
    return;
  *list = NULL;

  do
    {
      mtm_user_action_t *n = a->next;
      free (a);
      a = n;
    }
  while (a);
}


void
mtm_useraction_addUserCommitAction(mtm_tx_t * __td,
                                   _ITM_userCommitFunction fn,
                                   _ITM_transactionId tid, 
                                   void *arg)
{
//FIXME
  mtm_tx_t *tx;
  mtm_user_action_t *a;

  for (tx = mtm_tx(); tx->id != tid; tx = tx->prev)
    continue;

  a = malloc (sizeof (*a));
  a->next = tx->commit_actions;
  a->fn = fn;
  a->arg = arg;
  tx->commit_actions = a;
}


void
mtm_useraction_addUserUndoAction(mtm_tx_t * __td,
                                 const _ITM_userUndoFunction fn, 
                                 void *arg)
{
//FIXME
  mtm_tx_t *tx = mtm_tx();
  mtm_user_action_t *a;

  a = malloc (sizeof (*a));
  a->next = tx->undo_actions;
  a->fn = fn;
  a->arg = arg;
  tx->undo_actions = a;
}
