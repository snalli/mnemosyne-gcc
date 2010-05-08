/*!
 * \file
 * A simple experiment where all operations are writes, performed 1,000,000 times from
 * a single thread.
 * 
 * Based on tcbdbex.c, from TokyoCabinet v1.4.43.
 * 
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include <tcutil.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <mnemosyne.h>

MNEMOSYNE_PERSISTENT TCBDB* bdb;

typedef struct {
  char itsData[128];
} BigObject;

int main(int argc, char **argv)
{
	int ecode;
	size_t i;
	fprintf(stderr, "Allocator startup... \n");
	
	/* create the object */
	if (bdb == NULL) {
		bdb = tcbdbnew();
		fprintf(stderr, "BDB = %p\n", bdb);
		if(!tcbdbopen(bdb, "casket.tcb", BDBOWRITER | BDBOCREAT)){
			ecode = tcbdbecode(bdb);
			fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
			return 1;
		}
	} else {
		printf("NOT NULL. No experiment.\n");
		return 1;
	}
	
	fprintf(stderr, "Filling out a tree of 7,000.\n");
	/* store records */
	for(i = 0; i < 7000; ++i)
	{
    BigObject o;
    __tm_atomic tcbdbput(bdb, &i, sizeof(size_t), &o, sizeof(BigObject));
	}
	
  fprintf(stderr, "Running 1,000,000 updates on the tree.\n");
  struct timeval begin;
  gettimeofday(&begin, NULL);
  size_t x;
  for (x = 0; x < 1000000; ++x) {
    size_t key = x % 7000;
    BigObject o;
      __tm_atomic tcbdbput(bdb, &key, sizeof(size_t), &o, sizeof(BigObject)); 
      __tm_atomic tcbdbout(bdb, &key, sizeof(size_t));
  }
  struct timeval end;
  gettimeofday(&end, NULL);
  printf("Time: %lu microseconds\n", 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec));
  
	/* close the database */
	// NOPE! :)
	
	/* delete the object */
	// NOPE! :)
	
	return 0;
}
