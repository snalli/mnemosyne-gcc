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

#ifndef _ELEMSET_H_9KI112
#define _ELEMSET_H_9KI112

#include <stdlib.h>


typedef struct elemset_s {
	void         **array;
	int          size;
	int          count;
	unsigned int seed;
} elemset_t;


static int 
elemset_init(elemset_t *elemset, int size, unsigned int seed)
{
	int ret = 0; 

	if (!(elemset->array = calloc(size, sizeof(void *)))) {
		ret = -1;
		goto err;
	}
	elemset->size = size;
	elemset->count = 0;
	elemset->seed = seed;
	return 0;
	
err:
	return ret;
}


static int 
elemset_put(elemset_t *elemset, void *obj)
{
	int ret = 0;

	if (elemset->count >= elemset->size) {
		ret = -1;
		goto err;
	}
	elemset->array[elemset->count++] = obj;

err:
	return ret;
}


static int 
elemset_del_random(elemset_t *elemset, void **objp)
{
	int  ret = 0;
	int  index;
	void *obj;

	if (elemset->count == 0) {
		ret = -1;
		goto err;
	}

	index = rand_r(&elemset->seed) % elemset->count;
	obj = elemset->array[index];
	elemset->array[index] = elemset->array[elemset->count-1];
	elemset->count--;
	*objp = obj;

	return 0;
err:
	return ret;
}

static int 
elemset_get_random(elemset_t *elemset, void **objp)
{
	int  ret = 0;
	int  index;
	void *obj;

	if (elemset->count == 0) {
		ret = -1;
		goto err;
	}

	index = rand_r(&elemset->seed) % elemset->count;
	obj = elemset->array[index];
	*objp = obj;

	return 0;
err:
	return ret;
}
static
void
elemset_print(elemset_t *elemset)
{
	int i;

	printf("[");
	for (i=0; i<elemset->count; i++) {
		/* TODO */
	}
	printf("]\n");
}
#endif
