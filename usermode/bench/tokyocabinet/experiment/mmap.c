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

int main(int argc, char **argv)
{
	TCBDB *bdb;
	int ecode;
	size_t i;
	
	/* create the object */
	bdb = tcbdbnew();
	
	/* open the database */
	if(!tcbdbopen(bdb, "casket.tcb", BDBOWRITER | BDBOCREAT)){
		ecode = tcbdbecode(bdb);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
	}
	
	fprintf(stderr, "Filling out a tree of 7,000.\n");
	/* store records */
	for(i = 0; i < 7000; ++i)
	{
    bool result;
    result = tcbdbput(bdb, &i, sizeof(size_t), &i, sizeof(size_t));
	}
  tcbdbsync(bdb);
	
  fprintf(stderr, "Running 1,000,000 updates on the tree.\n");
  struct timeval begin;
  gettimeofday(&begin, NULL);
  size_t x;
  for (x = 0; x < 1000000; ++x) {
    size_t key = x % 7000;
      tcbdbput(bdb, &key, sizeof(size_t), &key, sizeof(size_t)); 
      tcbdbsync(bdb);
      tcbdbout(bdb, &key, sizeof(size_t));
      tcbdbsync(bdb);
  }
  struct timeval end;
  gettimeofday(&end, NULL);
  printf("Time: %lu microseconds\n", 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec));
	
	/* close the database */
	if(!tcbdbclose(bdb)){
		ecode = tcbdbecode(bdb);
		fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
	}
	
	/* delete the object */
	tcbdbdel(bdb);
	
	return 0;
}
