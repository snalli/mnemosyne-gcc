/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include "pvar.h"


int main (int argc, char const *argv[])
{
	printf("flag: %d", PGET(flag));

	PTx {
		if (PGET(flag) == 0) {
			PSET(flag, 1);
		} else {
			PSET(flag, 0);
		}
	}
	
	printf(" --> %d\n", PGET(flag));
	printf("&flag = %p\n", PADDR(flag));

	return 0;
}
