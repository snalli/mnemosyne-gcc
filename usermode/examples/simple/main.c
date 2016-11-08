/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include "pvar.h"


int main (int argc, char const *argv[])
{
	printf("flag: %d", get_flag());
	MNEMOSYNE_ATOMIC {
		if (get_flag() == 0) {
			set_flag(1);
		} else {
			set_flag(0);
		}
	}
	
	printf(" --> %d\n", get_flag());
	printf("&flag = %p\n", addr_flag());

	return 0;
}
