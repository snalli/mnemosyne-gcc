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
 * A simple experiment where we remove and add keys randomly at steady state,
 * from a single thread.
 * 
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 *         Haris Volos <hvolos@cs.wisc.edu>
 */
#include <assert.h>
#include <tcutil.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <mnemosyne.h>
#include "keyset.h"


MNEMOSYNE_PERSISTENT TCBDB* bdb;

int OBJ_SIZE = 128;
int NUM_KEYS = 10000;
int NUM_UPDATES = 1000000;


typedef struct {
  char itsData[4096];
} BigObject;


void usage(char *name)
{
	fprintf(stderr, "Usage: %s [-s obj_size] [-k num_keys] [-u num_updates] [-h]\n", 
	        name);
}


int main(int argc, char **argv)
{
	int            ecode;
	int            i;
	int            x;
	struct timeval begin;
	struct timeval end;
	keyset_t       put_keyset;
	keyset_t       del_keyset;
	int            key;
	int            opt;

	while ((opt=getopt(argc, argv, "s:k:u:h")) != -1) {
		switch (opt) {
			case 's':
				OBJ_SIZE=atoi(optarg);
				break;
			case 'k':
				NUM_KEYS=atoi(optarg);
				break;
			case 'u':
				NUM_UPDATES=atoi(optarg);
				break;
			case 'h':
			default:
				usage(argv[0]);
				exit(1);
		}
	}

	fprintf(stderr, "Allocator startup... \n");

	/* create the object */
	if (bdb == NULL) {
		bdb = tcbdbnew();
		if(!tcbdbopen(bdb, "casket.tcb", BDBOWRITER | BDBOCREAT)){
			ecode = tcbdbecode(bdb);
			fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
			return 1;
		}
	} else {
		printf("NOT NULL. No experiment.\n");
		return 1;
	}

	fprintf(stderr, "Filling out a tree of %d keys\n", NUM_KEYS);

	init_keyset(&put_keyset, NUM_KEYS, NUM_KEYS, 1);
	init_keyset(&del_keyset, NUM_KEYS, 0, 1);

	/* store records */
	for(i = 0; i < NUM_KEYS; i++)
	{
    	BigObject o;
		{
			tcbdbput(bdb, &i, sizeof(int), &o, OBJ_SIZE);
		}	
	}

	/* Now remove half of the keys to get a random tree to work at steady state */
	fprintf(stderr, "Removing %d keys\n", NUM_KEYS/2);
	for(i = 0; i < NUM_KEYS/2; i++)
	{
    	BigObject o;
		{
			key = out_key_random(&put_keyset);
			put_key(&del_keyset, key);
			tcbdbout(bdb, &key, sizeof(int));
		}	
	}

	/* sanity check */
	for(i = 0; i < put_keyset.keys_num; ++i)
	{
		key = get_key(&put_keyset, i);
		assert(tcbdbvnum(bdb, &key, sizeof(int)) > 0);
	}
	for(i = 0; i < del_keyset.keys_num; ++i)
	{
		key = get_key(&del_keyset, i);
		assert(tcbdbvnum(bdb, &key, sizeof(int)) == 0);
	}

	fprintf(stderr, "Running %d updates on the tree.\n", NUM_UPDATES);
	gettimeofday(&begin, NULL);
	for (x = 0; x < NUM_UPDATES; ++x) {
		BigObject o;
		key = out_key_random(&put_keyset);
		put_key(&del_keyset, key);
		__tm_atomic tcbdbout(bdb, &key, sizeof(int));
		key = out_key_random(&del_keyset);
		put_key(&put_keyset, key);
		__tm_atomic tcbdbput(bdb, &key, sizeof(int), &o, OBJ_SIZE); 
	}
	gettimeofday(&end, NULL);

	/* sanity check */
	for(i = 0; i < put_keyset.keys_num; ++i)
	{
		key = get_key(&put_keyset, i);
		assert(tcbdbvnum(bdb, &key, sizeof(int)) > 0);
	}
	for(i = 0; i < del_keyset.keys_num; ++i)
	{
		key = get_key(&del_keyset, i);
		assert(tcbdbvnum(bdb, &key, sizeof(int)) == 0);
	}

	printf("Time: %lu microseconds\n", 
	       1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec));

	/* close the database */
	// NOPE! :)
	
	/* delete the object */
	// NOPE! :)
	
	return 0;
}
