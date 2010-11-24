/*!
 * \file
 * A simple experiment where we remove and add keys randomly at steady state,
 * from a single thread.
 * 
 * Based on tcbdbex.c, from TokyoCabinet v1.4.43.
 * 
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 *         Haris Volos <hvolos@cs.wisc.edu>
 */
#include <tcutil.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "keyset.h"

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
	TCBDB          *bdb;
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
	
	/* create the object */
	bdb = tcbdbnew();
	
	/* open the database */
	//if(!tcbdbopen(bdb, "/mnt/pcmfs/tc/casket.tcb", BDBOWRITER | BDBOCREAT)){
	if(!tcbdbopen(bdb, "casket.tcb", BDBOWRITER | BDBOCREAT)){
		ecode = tcbdbecode(bdb);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
	}
	
	fprintf(stderr, "Filling out a tree of %d keys\n", NUM_KEYS);

	init_keyset(&put_keyset, NUM_KEYS, NUM_KEYS, 1);
	init_keyset(&del_keyset, NUM_KEYS, 0, 1);

	/* store records */
	for(i = 0; i < NUM_KEYS; ++i)
	{
    	bool result;
		BigObject o;
		result = tcbdbput(bdb, &i, sizeof(int), &o, OBJ_SIZE);
	}
	tcbdbsync(bdb);

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
	tcbdbsync(bdb);

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
		tcbdbout(bdb, &key, sizeof(int));
		tcbdbsync(bdb);
		key = out_key_random(&del_keyset);
		put_key(&put_keyset, key);
		tcbdbput(bdb, &key, sizeof(int), &o, OBJ_SIZE); 
		tcbdbsync(bdb);
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
	if(!tcbdbclose(bdb)){
		ecode = tcbdbecode(bdb);
		fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
	}
	
	/* delete the object */
	tcbdbdel(bdb);
	
	return 0;
}
