#ifndef _USERACTION_H
#define _USERACTION_H

typedef struct mtm_user_action_s mtm_user_action_t;

void mtm_useraction_runActions (mtm_user_action_t **list);
void mtm_useraction_freeActions (mtm_user_action_t **list);
void mtm_useraction_addUserCommitAction(mtm_tx_t * __td, _ITM_userCommitFunction fn, _ITM_transactionId tid, void *arg);
void mtm_useraction_addUserUndoAction(mtm_tx_t * __td, const _ITM_userUndoFunction fn, void *arg);

#endif
