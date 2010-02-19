/*!
 * \file
 * Implements the reincarnation_callback mechanism.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include "reincarnation_callback.h"
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
	reincarnation_callback_t* callback = (reincarnation_callback_t*) malloc(sizeof(struct reincarnation_callback));
	callback->routine = initializer;
	list_add_tail(&callback->list, &theRegisteredCallbacks);
}


void mnemosyne_reincarnation_callback_execute_all()
{
	struct list_head* callback_node;
	list_for_each(callback_node, &theRegisteredCallbacks) {
		reincarnation_callback_t* callback = list_entry(callback_node, reincarnation_callback_t, list);
		callback->routine();
	}
}
