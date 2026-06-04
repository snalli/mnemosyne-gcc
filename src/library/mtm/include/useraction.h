/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

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
