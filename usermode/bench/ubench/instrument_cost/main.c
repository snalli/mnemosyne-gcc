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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
#include "hrtime.h"


#define BLOCK_SIZE 64        /* cacheline size */

typedef struct block_s {
	union {
		uint64_t u64[BLOCK_SIZE/sizeof(uint64_t)];
		char bytes[BLOCK_SIZE];
	};	
} block_t;

block_t *persistent_blocks;
block_t *volatile_blocks;


void measure_instrumentation_cost(block_t *blocks, int n)
{
	int      i;
	hrtime_t start;
	hrtime_t stop;
	hrtime_t start_commit;
	hrtime_t stop_commit;

	
	__tm_atomic {
		__tm_waiver {
			hrtime_barrier();
			start = hrtime_cycles();
		}
		for (i=0; i<n; i++) {
			blocks[i].u64[0] = 1;
		}
		__tm_waiver {
			hrtime_barrier();
			start_commit = stop = hrtime_cycles();
		}
	}
	__tm_waiver {
		hrtime_barrier();
		stop_commit = hrtime_cycles();
	}

	printf("latency                  = %llu (ns)\n", HRTIME_CYCLE2NS(stop-start));
	printf("latency_per_write        = %llu (ns)\n", HRTIME_CYCLE2NS(stop-start)/n);
	printf("latency                  = %llu (ns)\n", HRTIME_CYCLE2NS(stop_commit-start_commit));
	printf("commit_latency_per_write = %llu (ns)\n", HRTIME_CYCLE2NS(stop_commit-start_commit)/n);
}


int
main(int argc, char *argv[])
{
	void     *ptr;
	int      nblocks;
	int      ubench;
	int      c;
	int      i;
	int      n;

	/* Default values */
	nblocks = 16;
	ubench = 0;

	while (1) {
		static struct option long_options[] = {
			{"nblocks",  required_argument, 0, 'n'},
			{"ubench",  required_argument, 0, 'u'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "n:u:",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 'n':
				nblocks = atoi(optarg);
				break;

			case 'u':
				ubench = atoi(optarg); 
				break;

		}
	}

	/* allocate blocks */
	ptr = pmalloc(sizeof(block_t) * 1024 + sizeof(block_t));
	persistent_blocks = (block_t *) (((uintptr_t) ptr + sizeof(block_t)) & ~(BLOCK_SIZE-1));
	ptr = malloc(sizeof(block_t) * 1024 + sizeof(block_t));
	volatile_blocks = (block_t *) (((uintptr_t) ptr + sizeof(block_t)) & ~(BLOCK_SIZE-1));

	/* now bring them in the cache */
	for (i=0; i<1024; i++) {
		memset(&persistent_blocks[i].u64[0], 0, 8);
		memset(&volatile_blocks[i].u64[0], 0, 8);
	}

	printf("persistent_blocks = %p\n", persistent_blocks);
	printf("volatile_blocks = %p\n", volatile_blocks);


	switch(ubench) {
		case 0:
			measure_instrumentation_cost(persistent_blocks, nblocks);
			break;
		case 1:
			measure_instrumentation_cost(volatile_blocks, nblocks);
			break;
	}
	fini_global();
}


