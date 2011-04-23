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
