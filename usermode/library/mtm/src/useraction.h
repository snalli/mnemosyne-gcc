#ifndef _USERACTION_H
#define _USERACTION_H

typedef struct mtm_user_action_list_s mtm_user_action_list_t;

int mtm_useraction_list_alloc(mtm_user_action_list_t **listp);
int mtm_useraction_list_free(mtm_user_action_list_t **listp);
int mtm_useraction_clear(mtm_user_action_list_t *list);
void mtm_useraction_list_run(mtm_user_action_list_t *list, int reverse);
void mtm_useraction_addUserCommitAction(mtm_tx_t * __td, _ITM_userCommitFunction fn, _ITM_transactionId tid, void *arg);
void mtm_useraction_addUserUndoAction(mtm_tx_t * __td, const _ITM_userUndoFunction fn, void *arg);

#endif
