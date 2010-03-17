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
mtm_useraction_addUserCommitAction(mtm_tx_t * tx,
                                   _ITM_userCommitFunction fn,
                                   _ITM_transactionId tid, 
                                   void *arg)
{
  mtm_user_action_t *a;

  a = malloc (sizeof (*a));
  a->next = tx->commit_actions;
  a->fn = fn;
  a->arg = arg;
  tx->commit_actions = a;
}


void
mtm_useraction_addUserUndoAction(mtm_tx_t * tx,
                                 const _ITM_userUndoFunction fn, 
                                 void *arg)
{
  mtm_user_action_t *a;

  a = malloc (sizeof (*a));
  a->next = tx->undo_actions;
  a->fn = fn;
  a->arg = arg;
  tx->undo_actions = a;
}
