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

#ifndef _PLIST_H
#define _PLIST_H

#include <stddef.h>

#define PCM_STORE(addr, val)                               \
	*(addr) = val; 

#define PCM_BARRIER


struct plist_head {
	struct plist_head *next;
	struct plist_head *prev;
	struct plist_head *next_shadow;
};


/**
 * \brief Given two nodes, it properly links them together to form
 * a double-linked list. This operation is idempotent.
 *
 * \param[in]  Pointer to the node that will become the head of the list.
 * \param[out] Pointer to the node that will become the tail of the list.
 */
static inline void plist_init(struct plist_head *head, struct plist_head *tail)
{
	PCM_STORE(&(head->next), tail);
	PCM_STORE(&(head->prev), tail);
	PCM_STORE(&(tail->next), head);
	PCM_STORE(&(tail->prev), head);
	PCM_BARRIER
}


static inline void plist_move(struct plist_head *listA, struct plist_head *listB)
{
	struct plist_head *listA_next_old = listA->next;
	struct plist_head *listA_prev_old = listA->prev;

	PCM_STORE(&(listA->prev->next), listA->next);
	PCM_BARRIER;
	PCM_STORE(&(listA->next), listB);
	PCM_BARRIER;
	PCM_STORE(&(listB->prev->next), listA);
	PCM_BARRIER;
	PCM_STORE(&(listA_next_old->prev), listA_prev_old);
	PCM_BARRIER;
	PCM_STORE(&(listA->prev), listB->prev);
	PCM_BARRIER;
	PCM_STORE(&(listA->next->prev), listA);
	PCM_BARRIER;
}

/**
 * \brief Recover a doubly linked list.
 *
 * Any of the nodes could be linked to another doubly or singly linked list.
 * That's why we need to need to ensure any PREV pointer of a node that could 
 * be located in a single linked list is non-NULL before following it.
 */
static inline void plist_recover(struct plist_head *list)
{
	struct plist_head *node;
	struct plist_head *nodeX;
	int               count = 0;

	for (node = list; prefetch(node->next), node != list || count == 0;
	     node = node->next, count++)
	{		
		if (node->next == node && node->prev == node) {
			/* Empty list -- nothing to recover*/
			return;
		}
recover:
		if (node->next->prev->prev == node /* CYCLE_I: 1F-2B cycle */) {
			nodeX = node->next->prev;
		    if (nodeX->next->prev != NULL &&
			    nodeX->next->prev->next == nodeX /* CYCLE_II: 1F-1B-1F cycle */) {
				PCM_STORE(&(nodeX->prev->next->prev), nodeX->prev);
				PCM_BARRIER
				goto recover;
			} else {
				PCM_STORE(&(nodeX->next), node->next);
				PCM_BARRIER;
				PCM_STORE(&(node->next), nodeX);
				PCM_BARRIER;
				goto recover;
			}	
		} else if (node->next->next->prev == node /* CYCLE_II: 2F-1B cycle */) {
			nodeX = node->next;
		    if (nodeX->prev != NULL &&
			    nodeX->prev->next->prev == nodeX /* CYCLE_I: 1B-1F-1B cycle */) {
				PCM_STORE(&(node->next), node->next->next);
				PCM_BARRIER;
				goto recover;
			} else {
				PCM_STORE(&(nodeX->prev), node);
				PCM_BARRIER;
				PCM_STORE(&(nodeX->next->prev), nodeX);
				PCM_BARRIER;
				goto recover;
			}	
		}
	}
}

static inline void plist_init_S(struct plist_head *head, struct plist_head *tail)
{
	PCM_STORE(&(head->next), tail);
	PCM_BARRIER
}

/**
 * \brief Adds a node in a persistent list 
 *
 * \param[in] The node to add.
 * \param[in] The list to add the node to.
 */
static inline void plist_add_S(struct plist_head *node, struct plist_head *head)
{

	PCM_STORE(&(node->next), head->next);
	PCM_STORE(&(head->next), node);
	PCM_BARRIER

}

/**
 * \brief Move a node from a double linked list into a single linked list.
 */
static inline void plist_move_DtoS(struct plist_head *listA, struct plist_head *listB)
{
	struct plist_head *listA_next_old = listA->next;
	struct plist_head *listA_prev_old = listA->prev;

	PCM_STORE(&(listA->prev->next), listA->next);
	PCM_BARRIER;
	PCM_STORE(&(listA->next), listB);
	PCM_BARRIER;
	PCM_STORE(&(listB->prev->next), listA);
	PCM_BARRIER;
	PCM_STORE(&(listA_next_old->prev), listA_prev_old);
	PCM_BARRIER;
	PCM_STORE(&(listA->prev), NULL);
	PCM_BARRIER;
}


/**
 * \brief Move a node from a single linked list into a double linked list.
 */
static inline void plist_move_StoD(struct plist_head *listA, struct plist_head *listB)
{
	struct plist_head *listA_next_old = listA->next;
	struct plist_head *listA_prev_old = listA->prev;

	PCM_STORE(&(listA->prev->next), listA->next);
	PCM_BARRIER;
	PCM_STORE(&(listA->next), listB);
	PCM_BARRIER;
	PCM_STORE(&(listB->prev->next), listA);
	PCM_BARRIER;
	PCM_STORE(&(listA_next_old->prev), listA_prev_old);
	PCM_BARRIER;
	PCM_STORE(&(listA->prev), listB->prev);
	PCM_BARRIER;
	PCM_STORE(&(listA->next->prev), listA);
	PCM_BARRIER;
}



#endif /* _PLIST_H */
