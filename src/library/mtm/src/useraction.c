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

#include "mtm_i.h"
#include "useraction.h"

#define USERACTION_LIST_SIZE 8

typedef struct mtm_user_action_s      mtm_user_action_t;

struct mtm_user_action_s
{
	_ITM_userCommitFunction  fn;
	void                     *arg;
};

struct mtm_user_action_list_s
{
	mtm_user_action_t *array;
	int               nb_entries;      /* Number of entries */
	int               size;            /* Size of array */
};



int
mtm_useraction_list_alloc(mtm_user_action_list_t **listp)
{
	mtm_user_action_list_t *list;

	list = (mtm_user_action_list_t *) malloc (sizeof(mtm_user_action_list_t));
	
	if (list) {
		list->array = (mtm_user_action_t *) realloc(NULL, sizeof(mtm_user_action_t) * USERACTION_LIST_SIZE);
		list->size = USERACTION_LIST_SIZE;
		*listp = list;
		return 0;
	}

	listp = NULL;
	return -1;
}


int
mtm_useraction_list_free(mtm_user_action_list_t **listp)
{
	assert(listp != NULL);

	mtm_user_action_list_t *list = *listp;
	
	if (list) {
		free (list->array);
		free (list);
	}
	*listp = NULL;

	return 0;
}



int
mtm_useraction_clear(mtm_user_action_list_t *list)
{
	assert(list != NULL);

	list->nb_entries = 0;

	return 0;
}


void
mtm_useraction_list_run (mtm_user_action_list_t *list, int reverse)
{
	mtm_user_action_t *action;
	int               i;

	for (i=0; i<list->nb_entries; i++) {
		if (reverse) {
			action = &list->array[list->nb_entries - i - 1];
		} else {	
			action = &list->array[i];
		}
		action->fn (action->arg);
	}
}


void
mtm_useraction_addUserCommitAction(mtm_tx_t * tx,
                                   _ITM_userCommitFunction fn,
                                   _ITM_transactionId tid, 
                                   void *arg)
{
	/* tid is ignored */
	mtm_user_action_list_t *list = tx->commit_action_list;
	mtm_user_action_t      *action;

	if (list->nb_entries == list->size) {
		list->size *= 2;
		list->array = (mtm_user_action_t *) realloc(list->array, sizeof(mtm_user_action_t) * list->size);
	}

	action = &list->array[list->nb_entries++];
	action->fn = fn;
	action->arg = arg;
}


void
mtm_useraction_addUserUndoAction(mtm_tx_t * tx,
                                 const _ITM_userUndoFunction fn, 
                                 void *arg)
{
	mtm_user_action_list_t *list = tx->undo_action_list;
	mtm_user_action_t      *action;

	if (list->nb_entries == list->size) {
		list->size *= 2;
		list->array = (mtm_user_action_t *) realloc(list->array, sizeof(mtm_user_action_t) * list->size);
	}

	action = &list->array[list->nb_entries++];
	action->fn = fn;
	action->arg = arg;
}
