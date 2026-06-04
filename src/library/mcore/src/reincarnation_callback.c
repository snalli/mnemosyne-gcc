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

/*!
 * \file
 * Implements the reincarnation_callback mechanism.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include "reincarnation_callback.h"
#include "init.h"
#include <list.h>
#include <stdlib.h>


/*! The list of registered callbacks. */
LIST_HEAD(theRegisteredCallbacks);

/*! A registered callback, part of a list of these. */
struct reincarnation_callback
{
	void (*routine)();      /*!< The callback to be executed. */
	struct list_head list;  /*!< Links this callback in the context of theRegisteredCallbacks. */
};
typedef struct reincarnation_callback reincarnation_callback_t;


void mnemosyne_reincarnation_callback_register(void(*initializer)())
{
	if (!mnemosyne_initialized) {
		reincarnation_callback_t* callback = (reincarnation_callback_t*) malloc(sizeof(struct reincarnation_callback));
		callback->routine = initializer;
		list_add_tail(&callback->list, &theRegisteredCallbacks);
	} else {
		initializer();  // We're already ready already!
	}
}


void mnemosyne_reincarnation_callback_execute_all()
{
	struct list_head* callback_node;
	struct list_head* next;
	list_for_each_safe(callback_node, next, &theRegisteredCallbacks) {
		list_del(callback_node);
		reincarnation_callback_t* callback = list_entry(callback_node, reincarnation_callback_t, list);
		callback->routine();
		free(callback);
	}
}
