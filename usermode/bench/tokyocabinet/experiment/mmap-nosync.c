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
	
	/* store records */
	for(i = 0; i < 1000; ++i)
	{
		if (!tcbdbput(bdb, &i, sizeof(size_t), &i, sizeof(size_t)))
		{
			ecode = tcbdbecode(bdb);
			fprintf(stderr, "put error: %s\n", tcbdberrmsg(ecode));
		}
	}
	
	/* close the database */
	if(!tcbdbclose(bdb)){
		ecode = tcbdbecode(bdb);
		fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
	}
	
	/* delete the object */
	tcbdbdel(bdb);
	
	return 0;
}
