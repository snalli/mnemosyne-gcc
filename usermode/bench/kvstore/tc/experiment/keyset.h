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

#ifndef _KEYSET_H_9KI112
#define _KEYSET_H_9KI112

#include <stdlib.h>

typedef struct keyset_s {
	int          *keys_array;
	int          keys_num;
	int          keys_num_max;
	unsigned int seed;
} keyset_t;

void init_keyset(keyset_t *keyset, int keys_num_max, int keys_num, unsigned int seed)
{
	int i;

	keyset->keys_array = calloc(keys_num_max, sizeof(int));

	for (i=0; i<keys_num_max; i++) {
		keyset->keys_array[i] = i;
	}
	keyset->keys_num = keys_num;
	keyset->keys_num_max = keys_num_max;
	keyset->seed = seed;
}

void put_key(keyset_t *keyset, int key)
{
	keyset->keys_array[keyset->keys_num++] = key;
}

int out_key_random(keyset_t *keyset)
{
	int index;
	int key;

	index = rand_r(&keyset->seed) % keyset->keys_num;
	key = keyset->keys_array[index];
	if (index != keyset->keys_num-1) {
		keyset->keys_array[index] = keyset->keys_array[keyset->keys_num-1];
	}
	keyset->keys_num--;
	return key;
}

int get_key(keyset_t *keyset, int index)
{
	assert(index < keyset->keys_num);
	return keyset->keys_array[index];
}

void print_keyset(keyset_t *keyset)
{
	int i;

	for (i=0; i<keyset->keys_num; i++) {
		printf("%d\n", keyset->keys_array[i]);
	}
}
#endif
